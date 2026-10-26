// SPDX-License-Identifier: Apache-2.0
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hb_config.h"
#include "hb_enrollment_flow.h"
#include "hb_face.h"
#include "hb_household_voice.h"
#include "hb_identity.h"
#include "hb_identity_lifecycle.h"
#include "hb_profile_flash.h"
#include "hb_profile_store_v2.h"
#include "hb_voice.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <new>
extern "C"
{
#include "hb_camera.h"
#include "hb_camera_preview.h"
#include "hb_compute_budget.h"
#include "hb_raw_frame.h"
    esp_err_t hb_identity_model_storage(void);
}
#if CONFIG_HB_IDENTITY_EXPERIMENTAL
#include "human_face_detect.hpp"
#include "human_face_recognition.hpp"
#endif

static QueueHandle_t jobs;
static SemaphoreHandle_t guard;
static hb_id_status published{};
static std::atomic<uint32_t> epoch{1};
static std::atomic<bool> stop_requested{false};
static std::atomic<bool> disable_requested{false};
static std::atomic<uint32_t> command_rejections{0};
static std::atomic<uint64_t> session_deadline{0};
static hb_id_database *database;
static hb_id_enrollment *enrollment;
static uint16_t *published_preview;
static uint32_t preview_sequence;
static portMUX_TYPE event_lock = portMUX_INITIALIZER_UNLOCKED;
static hb_event_bus event_bus;
static uint64_t boot_id;
static hb_profile_io profile_io{};
static hb_profile_revision profile_revision{0, -1};
static hb_household_snapshot *household;
static hb_household_snapshot *storage_scratch;
static bool storage_dirty, cleanup_pending;
static uint64_t pending_saved_id;
static uint8_t durable_digest[32];
static bool digest_valid;
static hb_voice_database *voice_database;
static hb_household_voice_enrollment *voice_enrollment;
// No accepted microphone/model/resource/speech/calibration evidence yet.
// There is intentionally no UI, NVS or network bypass of these acceptance gates.
static const hb_voice_acceptance voice_acceptance{};
static const hb_voice_policy voice_policy{};
static uint32_t voice_epoch = 1;
static bool voice_verifying;
// Initial engineering thresholds, deliberately not described as calibrated probabilities.
static const hb_id_policy policy{CONFIG_HB_IDENTITY_THRESHOLD_MILLI / 1000.0f,
                                 CONFIG_HB_IDENTITY_MARGIN_MILLI / 1000.0f,
                                 CONFIG_HB_IDENTITY_AGREEMENT_MILLI / 1000.0f, 3};
static hb_id_mode resting_mode(bool auto_enabled)
{
    unsigned usable = 0;
    for (const auto &p : database->people)
        usable += p.id && p.enabled && p.count;
    return hb_id_auto_ready(auto_enabled, usable) ? HB_ID_AUTO : HB_ID_STOP;
}
static uint64_t now_ms()
{
    return esp_timer_get_time() / 1000;
}
static void message(hb_id_status &s, const char *text)
{
    snprintf(s.message, sizeof(s.message), "%s", text);
}
static void publish(hb_id_status &s)
{
    s.profile_count = 0;
    s.usable_profiles = 0;
    s.command_rejections = command_rejections.load();
    s.stack_min = uxTaskGetStackHighWaterMark(nullptr);
    for (const auto &p : database->people)
        if (p.id)
        {
            if (p.enabled && p.count)
                ++s.usable_profiles;
            auto &d = s.profiles[s.profile_count++];
            d.id = p.id;
            d.samples = p.count;
            d.enabled = p.enabled;
            d.voice_samples = hb_household_voice_count(voice_database, p.id);
            snprintf(d.name, sizeof(d.name), "%s", p.name);
        }
    s.psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    s.voice_available = hb_voice_accepted(&voice_acceptance, &voice_policy);
    s.voice_enrollment_state = voice_enrollment ? voice_enrollment->state : HB_VEN_IDLE;
    s.voice_enrollment_samples = voice_enrollment ? voice_enrollment->staging.count : 0;
    s.psram_largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    s.internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s.internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    xSemaphoreTake(guard, portMAX_DELAY);
    published = s;
    xSemaphoreGive(guard);
}

static void identity_events(const hb_id_status &s)
{
    static hb_presence_tracker presence{};
    static hb_presence_value previous = HB_PRESENCE_UNKNOWN;
    static uint32_t session;
    if (!hb_id_background(s.mode) || s.session != epoch.load() || !s.detection_valid ||
        !s.frame_ms || s.frame_ms > now_ms() || now_ms() - s.frame_ms > 5000)
        return;
    if (session != s.session)
    {
        session = s.session;
        previous = HB_PRESENCE_UNKNOWN;
        hb_presence_reset(&presence);
    }
    hb_presence_observe(&presence, s.sequence, s.frame_ms, s.completed_ms, s.faces > 0, now_ms());
    auto occupancy = hb_presence_read(&presence, true, s.ready, now_ms());
    hb_id_event e{};
    e.boot_id = boot_id;
    e.timestamp_ms = s.completed_ms;
    e.frame_ms = s.frame_ms;
    e.session = s.session;
    e.sequence = s.sequence;
    e.state = s.match.state;
    e.similarity = s.match.best;
    e.second = s.match.second;
    e.source = HB_ID_SOURCE_FACE;
    time_t wall = time(nullptr);
    if (wall > 1700000000)
        e.unix_ms = (int64_t)wall * 1000;
    bool entered = occupancy.fresh && occupancy.value == HB_PRESENCE_PRESENT &&
                   previous != HB_PRESENCE_PRESENT;
    bool left =
        occupancy.fresh && occupancy.value == HB_PRESENCE_ABSENT && previous == HB_PRESENCE_PRESENT;
    if (occupancy.fresh)
        previous = occupancy.value;
    portENTER_CRITICAL(&event_lock);
    if (s.session == epoch.load() && (entered || left))
    {
        e.kind = entered ? HB_ID_EVENT_ENTERED : HB_ID_EVENT_LEFT;
        hb_event_push(&event_bus, e);
    }
    portEXIT_CRITICAL(&event_lock);
    if (s.match.state == HB_ID_UNAVAILABLE || (s.match.state == HB_ID_KNOWN && !s.confirmed))
        return;
    e.kind = s.confirmed                        ? HB_ID_EVENT_RECOGNIZED
             : s.match.state == HB_ID_AMBIGUOUS ? HB_ID_EVENT_AMBIGUOUS
                                                : HB_ID_EVENT_UNKNOWN;
    e.confirmed = s.confirmed;
    e.person_id = s.confirmed ? s.confirmed_id : 0;
    e.persistent_id = s.storage_saved;
    if (e.person_id)
        for (unsigned i = 0; i < s.profile_count; ++i)
            if (s.profiles[i].id == e.person_id)
                snprintf(e.person_name, sizeof(e.person_name), "%s", s.profiles[i].name);
    portENTER_CRITICAL(&event_lock);
    if (s.session == epoch.load())
        hb_event_push(&event_bus, e);
    portEXIT_CRITICAL(&event_lock);
}

// Hash only durable fields; recognition counters and cooldowns remain RAM-only.
static bool persistence_fingerprint(uint8_t digest[32])
{
    memcpy(storage_scratch, household, sizeof(*household));
    for (auto &p : storage_scratch->face.people)
    {
        p.last_greeting_ms = p.last_sample_ms = p.last_sample_sequence = 0;
    }
    for (auto &p : storage_scratch->voice.people)
        p.last_end = 0;
    bool ok = profile_io.sha256(storage_scratch, sizeof(*storage_scratch), digest);
    memset(storage_scratch, 0, sizeof(*storage_scratch));
    return ok;
}
static void persistence_metrics(unsigned stage, uint64_t started)
{
    ESP_LOGI("identity",
             "PROFILE_STORAGE stage=%u storage_us=%llu workspace=%u internal_low=%u psram_low=%u",
             stage, (unsigned long long)(esp_timer_get_time() - started),
             (unsigned)(2 * sizeof(hb_household_snapshot)),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));
}
static void persistence_start(hb_id_status &s)
{
    s.storage_result = HB_STORE_LOCKED;
    snprintf(s.persistence_message, sizeof(s.persistence_message),
             "RAM-only: profile znikają po restarcie");
    if (!hb_profile_flash_open(&profile_io))
        return;
    s.storage_mode = profile_io.storage_mode;
    storage_scratch = (hb_household_snapshot *)heap_caps_calloc(
        1, sizeof(*storage_scratch), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!storage_scratch)
    {
        snprintf(s.persistence_message, sizeof(s.persistence_message),
                 "Brak pamięci bazy; zapis zablokowany");
        return;
    }
    uint64_t storage_started = esp_timer_get_time();
    hb_store_result r = hb_profile_v2_boot(&profile_io, household, storage_scratch,
                                           &profile_revision, esp_random());
    persistence_metrics(0, storage_started);
    s.storage_result = r;
    ESP_LOGI("identity", "PROFILE_BOOT result=%u bank=%d generation=%llu", (unsigned)r,
             profile_revision.bank, (unsigned long long)profile_revision.generation);
    s.persistence_ready = r == HB_STORE_OK || r == HB_STORE_EMPTY;
    if (!s.persistence_ready)
    {
        // Restore may have inspected invalid bytes. Never expose them as live records.
        memset(household, 0, sizeof(*household));
        hb_id_database_init(database, esp_random());
        hb_voice_database_init(voice_database);
    }
    s.storage_saved = r == HB_STORE_OK;
    s.storage_generation = profile_revision.generation;
    s.storage_bank = profile_revision.bank;
    digest_valid = s.storage_saved && persistence_fingerprint(durable_digest);
    if (s.storage_saved)
        snprintf(s.persistence_message, sizeof(s.persistence_message),
                 "Profile zapisane na urządzeniu");
    else
        snprintf(s.persistence_message, sizeof(s.persistence_message), "%s%s",
                 hb_profile_result_text(r), s.persistence_ready ? "" : ". Zapis zablokowany");
}

static bool persistence_commit(hb_id_status &s, bool deletion)
{
    storage_dirty = true;
    cleanup_pending |= deletion;
    s.storage_saved = false;
    if (!s.persistence_ready)
    {
        message(s, "Zmiany w RAM, niezapisane trwale. Sprawdź status bazy i zgodę.");
        return false;
    }
    // Serialize storage hashing/Flash work with existing AI/voice resource owners.
    if (!hb_compute_enter(1000))
    {
        snprintf(s.persistence_message, sizeof(s.persistence_message),
                 "Zasoby zajęte. Zmiany w RAM; ponów zapis");
        message(s, s.persistence_message);
        return false;
    }
    struct ComputeLease
    {
        ~ComputeLease()
        {
            hb_compute_leave();
        }
    } lease;
    uint8_t digest[32];
    bool same = digest_valid && persistence_fingerprint(digest) &&
                !memcmp(digest, durable_digest, sizeof(digest));
    s.storage_saving = true;
    snprintf(s.persistence_message, sizeof(s.persistence_message), "Zapisywanie profili...");
    publish(s);
    uint64_t storage_started = esp_timer_get_time();
    hb_store_result r = same ? HB_STORE_OK
                             : hb_profile_v2_commit(&profile_io, household, storage_scratch,
                                                    &profile_revision, true);
    persistence_metrics(1, storage_started);
    s.storage_result = r;
    s.storage_saving = false;
    if (r != HB_STORE_OK)
    {
        snprintf(s.persistence_message, sizeof(s.persistence_message),
                 "Błąd zapisu (%u). Zmiany w RAM; ponów zapis", (unsigned)r);
        message(s, s.persistence_message);
        return false;
    }
    storage_dirty = false;
    s.storage_saved = true;
    s.storage_generation = profile_revision.generation;
    s.storage_bank = profile_revision.bank;
    s.storage_last_save_ms = now_ms();
    digest_valid = persistence_fingerprint(durable_digest);
    if (cleanup_pending)
    {
        r = hb_profile_v2_scrub(&profile_io, &profile_revision, true);
        cleanup_pending = r != HB_STORE_OK;
    }
    snprintf(s.persistence_message, sizeof(s.persistence_message),
             cleanup_pending ? "Zapisano. Błąd czyszczenia starego banku; ponów"
                             : "Profile zapisane na urządzeniu");
    message(s, s.persistence_message);
    return true;
}

#if CONFIG_HB_IDENTITY_EXPERIMENTAL
class Models
{
  public:
    HumanFaceDetect *detector = nullptr, *official = nullptr;
    human_face_detect::MNP *mnp = nullptr;
    HumanFaceFeat *mbf = nullptr;
    uint8_t *raw = nullptr, *roi_pixels = nullptr;
    uint16_t *thumbnail = nullptr;
    static constexpr size_t raw_capacity = 1288 * 728;
    void unload()
    {
        if (!mbf && !official && !mnp && !detector && !raw && !roi_pixels && !thumbnail)
            return;
        ESP_LOGI("identity", "MODEL_RELEASE begin heap_ok=%d stack_min=%u",
                 heap_caps_check_integrity_all(true),
                 (unsigned)uxTaskGetStackHighWaterMark(nullptr));
        delete mbf;
        mbf = nullptr;
        ESP_LOGI("identity", "MODEL_RELEASE mbf_done");
        delete official;
        official = nullptr;
        delete mnp;
        mnp = nullptr;
        delete detector;
        detector = nullptr;
        free(raw);
        raw = nullptr;
        free(roi_pixels);
        roi_pixels = nullptr;
        free(thumbnail);
        thumbnail = nullptr;
        ESP_LOGI("identity", "MODEL_RELEASE done heap_ok=%d internal=%u largest=%u psram=%u",
                 heap_caps_check_integrity_all(true),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    }
    bool load(hb_id_status &s)
    {
        if (mbf)
            return true;
        int64_t load_started = esp_timer_get_time();
        static uint64_t last_load_log;
        if (!last_load_log || now_ms() - last_load_log >= 10000)
        {
            last_load_log = now_ms();
            ESP_LOGI(
                "identity", "MODEL_LOAD begin internal=%u largest=%u psram=%u largest=%u",
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
        }
        esp_err_t model_error = hb_identity_model_storage();
        if (model_error != ESP_OK)
        {
            snprintf(s.message, sizeof(s.message), "Model MBF we Flash: %s",
                     esp_err_to_name(model_error));
            return false;
        }
        ESP_LOGI("identity", "MODEL_VALIDATE mapped_sha256_ok us=%u",
                 (unsigned)(esp_timer_get_time() - load_started));
        uint32_t p = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        uint32_t i = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        // Provisional admission budgets, to be replaced by measured board peaks.
        if (p < 20 * 1024 * 1024 ||
            heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) < 12 * 1024 * 1024 ||
            i < 180 * 1024 ||
            heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) < 64 * 1024)
        {
            message(s, "Za mało pamięci na bezpieczny start MBF");
            return false;
        }
        raw = (uint8_t *)heap_caps_malloc(raw_capacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        roi_pixels =
            (uint8_t *)heap_caps_malloc(640 * 640 * 3, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        thumbnail =
            (uint16_t *)heap_caps_malloc(HB_FACE_INPUT_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!raw || !roi_pixels || !thumbnail)
        {
            unload();
            return false;
        }
        detector =
            new (std::nothrow) HumanFaceDetect(HumanFaceDetect::ESPDET_PICO_416_416_FACE, false);
        ESP_LOGI("identity", "MODEL_LOAD detector_done");
        mnp = new (std::nothrow)
            human_face_detect::MNP("human_face_detect_mnp_s8_v1.espdl", 0.7f, 0.5f);
        ESP_LOGI("identity", "MODEL_LOAD mnp_done");
        mbf = new (std::nothrow) HumanFaceFeat(HumanFaceFeat::MBF_S8_V1, false);
        if (!detector || !mnp || !mbf || mbf->get_feat_len() != HB_ID_DIM)
        {
            unload();
            return false;
        }
        auto *input = mbf->get_raw_model()->get_input();
        if (!input || input->shape != std::vector<int>({1, 112, 112, 3}))
        {
            unload();
            message(s, "Nieprawidłowy format wejścia MBF");
            return false;
        }
        size_t after_p = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        size_t after_i = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        s.psram_load_delta = p > after_p ? p - after_p : 0;
        s.internal_load_delta = i > after_i ? i - after_i : 0;
        ESP_LOGI("identity", "load psram_delta=%u internal_delta=%u", (unsigned)s.psram_load_delta,
                 (unsigned)s.internal_load_delta);
        if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) < 6 * 1024 * 1024 ||
            heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) < 96 * 1024)
        {
            unload();
            message(s, "Przekroczono budżet pamięci modeli");
            return false;
        }
        s.model_load_us = esp_timer_get_time() - load_started;
        return true;
    }
    bool infer(hb_id_status &s, hb_id_embedding &embedding)
    {
        hb_camera_status cs{};
        hb_camera_get_status(&cs);
        hb_camera_frame f{};
        if (!cs.running || cs.test_pattern || !hb_camera_acquire(&f))
        {
            message(s, "Brak aktualnej klatki kamery");
            return false;
        }
        unsigned rw = f.width, rh = f.height;
        uint64_t begin = esp_timer_get_time();
        bool valid = f.rotation_ccw == 90 && rw >= 32 && rh >= 32 && !(rw & 1) && !(rh & 1) &&
                     (size_t)rw * rh <= raw_capacity && f.sequence > s.sequence && f.timestamp_ms &&
                     f.timestamp_ms <= now_ms() && now_ms() - f.timestamp_ms <= 500;
        if (valid)
        {
            // One reusable RAW8 snapshot preserves exact-frame ROI pixels after releasing DMA.
            memcpy(raw, f.pixels, (size_t)rw * rh);
            s.sequence = f.sequence;
            s.frame_ms = f.timestamp_ms;
        }
        hb_camera_release(&f);
        if (!valid)
        {
            message(s, "Nieaktualna lub nieobsługiwana klatka");
            return false;
        }
        hb_raw_thumbnail_upright(raw, rw, rh, thumbnail, HB_FACE_INPUT_WIDTH, HB_FACE_INPUT_HEIGHT);
        hb_camera_make_preview(thumbnail, HB_FACE_INPUT_WIDTH, HB_FACE_INPUT_HEIGHT, thumbnail,
                               HB_FACE_INPUT_WIDTH, HB_FACE_INPUT_HEIGHT);
        dl::image::img_t small{.data = thumbnail,
                               .width = HB_FACE_INPUT_WIDTH,
                               .height = HB_FACE_INPUT_HEIGHT,
                               .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565LE};
        int64_t stage = esp_timer_get_time();
        auto &detections = detector->run(small);
        s.detection_valid = true;
        s.detection_us = esp_timer_get_time() - stage;
        s.faces = detections.size();
        if (s.faces != 1 || detections.front().box.size() != 4)
        {
            message(s, s.faces > 1 ? "Wykryto więcej niż jedną twarz" : "Spójrz w kamerę");
            return false;
        }
        const auto &box = detections.front().box;
        s.quality = hb_face_quality_measure(thumbnail, HB_FACE_INPUT_BYTES, HB_FACE_INPUT_WIDTH,
                                            HB_FACE_INPUT_HEIGHT, box[0], box[1], box[2], box[3]);
        if (!hb_id_roi_map(box[0], box[1], box[2], box[3], HB_FACE_INPUT_WIDTH,
                           HB_FACE_INPUT_HEIGHT, rw, rh, &s.roi))
        {
            message(s, "Powtórz zdjęcie: twarz poza zakresem ROI");
            return false;
        }
        const auto &roi = s.roi;
        if (!hb_id_raw_roi(raw, (size_t)rw * rh, rw, rh, roi, roi_pixels, 640 * 640 * 3))
            return false;
        dl::image::img_t crop{.data = roi_pixels,
                              .width = (uint16_t)roi.w,
                              .height = (uint16_t)roi.h,
                              .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888};
        std::list<dl::detect::result_t> candidates;
        int left = box[0] * (rh - 2) / HB_FACE_INPUT_WIDTH - roi.x;
        int top = box[1] * (rw - 2) / HB_FACE_INPUT_HEIGHT - roi.y;
        int right = (box[2] + 1) * (rh - 2) / HB_FACE_INPUT_WIDTH - roi.x;
        int bottom = (box[3] + 1) * (rw - 2) / HB_FACE_INPUT_HEIGHT - roi.y;
        candidates.push_back({0, detections.front().score, {left, top, right, bottom}, {}});
        stage = esp_timer_get_time();
        if (s.official_landmarks && !official &&
            (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) < 8 * 1024 * 1024 ||
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) < 128 * 1024))
        {
            message(s, "Za mało pamięci na porównanie MSR + MNP");
            return false;
        }
        if (s.official_landmarks && !official)
            official = new (std::nothrow) HumanFaceDetect(HumanFaceDetect::MSRMNP_S8_V1, false);
        if (s.official_landmarks && !official)
            return false;
        auto &landmarks = s.official_landmarks ? official->run(crop) : mnp->run(crop, candidates);
        s.landmarks_us = esp_timer_get_time() - stage;
        if (landmarks.size() != 1 || landmarks.front().keypoint.size() != 10)
        {
            message(s, "Brak pięciu poprawnych punktów twarzy");
            return false;
        }
        auto &keypoints = landmarks.front().keypoint;
        hb_id_landmarks local{};
        for (unsigned j = 0; j < 5; ++j)
            local.p[j] = {(float)keypoints[2 * j], (float)keypoints[2 * j + 1]};
        if (!hb_id_landmarks_valid(&local, roi.w, roi.h))
        {
            message(s, "Spójrz w kamerę: nieprawidłowa geometria twarzy");
            return false;
        }
        for (unsigned j = 0; j < 5; ++j)
            s.landmarks.p[j] = {local.p[j].x + roi.x, local.p[j].y + roi.y};
        stage = esp_timer_get_time();
        // Actual upstream similarity alignment + 112x112 RGB preprocessing, not box proportions.
        dl::TensorBase *feat = mbf->run(crop, keypoints);
        s.embedding_us = esp_timer_get_time() - stage;
        s.total_us = esp_timer_get_time() - begin;
        if (!feat || feat->dtype != dl::DATA_TYPE_FLOAT || feat->get_size() != HB_ID_DIM ||
            !feat->data)
        {
            message(s, "Nieprawidłowy wynik modelu MBF");
            return false;
        }
        memcpy(embedding.v, feat->data, sizeof(embedding.v));
        if (!hb_id_embedding_valid(&embedding))
        {
            message(s, "Nieprawidłowa normalizacja wektora MBF");
            return false;
        }
        static uint64_t last_diagnostic_ms;
        if (!last_diagnostic_ms || now_ms() - last_diagnostic_ms >= 10000)
        {
            last_diagnostic_ms = now_ms();
            ESP_LOGI("identity",
                     "MBF_EXEC_OK seq=%u D/L/MBF/total_us=%u/%u/%u/%u embedding_dim=%d load_us=%u",
                     (unsigned)s.sequence, (unsigned)s.detection_us, (unsigned)s.landmarks_us,
                     (unsigned)s.embedding_us, (unsigned)s.total_us, HB_ID_DIM,
                     (unsigned)s.model_load_us);
        }
        message(s, "Kandydat z nowej klatki; progi eksperymentalne");
        return true;
    }
};
#endif

static void worker(void *)
{
    hb_id_status state{};
    hb_id_tracker tracker{};
    persistence_start(state);
    state.auto_enabled = hb_config_identity_auto();
    unsigned usable = 0;
    for (const auto &p : database->people)
        usable += p.id && p.enabled && p.count;
    if (hb_id_auto_ready(state.auto_enabled, usable))
        state.mode = HB_ID_AUTO;
    else if (state.auto_enabled)
        message(state, "AUTO oczekuje na prawidłowe profile domowników");
    bool capture = false, verify = false, evaluate = false, guided = false;
    unsigned evaluation_frames = 0;
    uint64_t evaluation_expected = 0;
    uint32_t evaluation_sequence = 0;
    uint64_t until = 0;
    unsigned load_attempts = 0;
    uint64_t next_load_ms = 0;
#if CONFIG_HB_IDENTITY_EXPERIMENTAL
    Models models;
    hb_id_embedding embedding{};
#endif
    for (;;)
    {
        static hb_voice_capture_result voice_result;
        if (hb_voice_capture_take(&voice_result))
        {
            if (voice_enrollment && voice_result.epoch == voice_epoch)
            {
                auto r =
                    voice_result.error == ESP_OK
                        ? hb_household_voice_sample(voice_enrollment, database, voice_database,
                                                    &voice_result.embedding, voice_epoch,
                                                    voice_result.end_ms, now_ms(), voice_verifying)
                        : HB_VEN_SAMPLE;
                message(state, hb_household_voice_error(r));
            }
            memset(&voice_result, 0, sizeof(voice_result));
        }
        if (voice_enrollment && voice_enrollment->deadline_ms &&
            now_ms() > voice_enrollment->deadline_ms)
        {
            hb_voice_diagnostic_stop();
            hb_household_voice_cancel(voice_enrollment);
            ++voice_epoch;
        }
        if (stop_requested.exchange(false))
        {
            hb_voice_diagnostic_stop();
            hb_household_voice_cancel(voice_enrollment);
            ++voice_epoch;
            memset(enrollment, 0, sizeof(*enrollment));
            state.mode = HB_ID_STOP;
            state.confirmed = false;
            state.confirmed_id = 0;
            state.match = {HB_ID_UNAVAILABLE, 0, -1, -1};
            capture = verify = false;
            hb_id_tracker_reset(&tracker, epoch.load());
            message(state, "Rozpoznawanie zatrzymane");
        }
        if (disable_requested.exchange(false))
        {
            if (hb_config_identity_auto_save(false) == ESP_OK)
                state.auto_enabled = false;
            else
                snprintf(state.last_error, sizeof(state.last_error),
                         "Stop działa; błąd zapisu OFF. AUTO może wrócić po restarcie");
        }
        hb_id_command c{};
        while (xQueueReceive(jobs, &c, 0) == pdTRUE)
        {
            if (c.type < HB_ID_CMD_VOICE_BEGIN)
            {
                hb_voice_diagnostic_stop();
                hb_household_voice_cancel(voice_enrollment);
                ++voice_epoch;
            }
            if (c.type == HB_ID_CMD_EVALUATE)
            {
                if (state.mode == HB_ID_TEST && !evaluate)
                {
                    evaluate = true;
                    evaluation_frames = 0;
                    evaluation_expected = state.expected_id;
                    evaluation_sequence = state.sequence;
                    hb_id_tracker_reset(&tracker, epoch.load());
                    message(state, "Nowa próba: trzy nowe obserwacje; bez zapisu zdjęć");
                }
                else
                    message(state, "Próba już trwa lub TEST nieaktywny");
                continue;
            }
            if (c.type == HB_ID_CMD_AUTO)
            {
                load_attempts = 0;
                next_load_ms = 0;
                if (hb_config_identity_auto_save(true) != ESP_OK)
                {
                    snprintf(state.last_error, sizeof(state.last_error),
                             "Nie potwierdzono zapisu zgody AUTO");
                    continue;
                }
                state.auto_enabled = true;
            }
            if (c.type == HB_ID_CMD_TEST || c.type == HB_ID_CMD_BACKGROUND)
            {
                if (hb_config_identity_auto_save(false) == ESP_OK)
                    state.auto_enabled = false;
                else
                    snprintf(state.last_error, sizeof(state.last_error),
                             "Nie zapisano wyłączenia AUTO po restarcie");
            }
            state.confirmed = false;
            state.confirmed_id = 0;
            state.match = {HB_ID_UNAVAILABLE, 0, -1, -1};
            hb_id_tracker_reset(&tracker, epoch.load());
            evaluate = false;
            if ((c.type == HB_ID_CMD_CREATE || c.type == HB_ID_CMD_BEGIN ||
                 c.type == HB_ID_CMD_VOICE_BEGIN) &&
                !hb_config_biometric_notice())
            {
                message(state, "Najpierw potwierdź lokalny zapis bez szyfrowania");
                continue;
            }
            switch (c.type)
            {
            case HB_ID_CMD_NOTICE_ACK:
                message(state, hb_config_biometric_notice_save() == ESP_OK
                                   ? "Przyjęto zgodę na lokalny zapis bez szyfrowania"
                                   : "Błąd zapisu zgody");
                break;
            case HB_ID_CMD_RETRY_SAVE:
                if ((storage_dirty || cleanup_pending) && persistence_commit(state, false))
                {
                    state.saved_id = pending_saved_id;
                    pending_saved_id = 0;
                }
                break;
            case HB_ID_CMD_ENABLE:
            {
                hb_id_person *person = hb_id_find(database, c.person_id);
                if (person)
                {
                    person->enabled = c.guided;
                    persistence_commit(state, false);
                    state.mode = resting_mode(state.auto_enabled);
                }
                break;
            }
            case HB_ID_CMD_VOICE_BEGIN:
            {
                if (!hb_voice_accepted(&voice_acceptance, &voice_policy))
                {
                    message(state, hb_household_voice_error(HB_VEN_GATED));
                    break;
                }
                if (!voice_database)
                {
                    voice_database = (hb_voice_database *)heap_caps_calloc(
                        1, sizeof(*voice_database), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                    if (voice_database)
                        hb_voice_database_init(voice_database);
                }
                if (!voice_enrollment)
                    voice_enrollment = (hb_household_voice_enrollment *)heap_caps_calloc(
                        1, sizeof(*voice_enrollment), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                if (!voice_database || !voice_enrollment)
                {
                    message(state, "Brak pamięci do rejestracji głosu");
                    break;
                }
                hb_voice_diagnostic_stop();
                ++voice_epoch;
                auto r =
                    hb_household_voice_begin(voice_enrollment, database, c.person_id, voice_epoch,
                                             now_ms(), &voice_acceptance, &voice_policy);
                message(state, r == HB_VEN_OK ? "Naciśnij Pobierz próbkę i mów normalnie"
                                              : hb_household_voice_error(r));
                break;
            }
            case HB_ID_CMD_VOICE_SAMPLE:
            case HB_ID_CMD_VOICE_VERIFY:
                if (voice_enrollment && c.person_id == voice_enrollment->staging.id &&
                    hb_voice_accepted(&voice_acceptance, &voice_policy) &&
                    voice_enrollment->state ==
                        (c.type == HB_ID_CMD_VOICE_VERIFY ? HB_VEN_VERIFY : HB_VEN_COLLECT))
                {
                    voice_verifying = c.type == HB_ID_CMD_VOICE_VERIFY;
                    auto err = hb_voice_capture_start(0, voice_epoch);
                    message(state, err == ESP_OK
                                       ? "Mów normalnie; jedna osoba, inne zdanie"
                                       : "Mikrofon zajęty lub brak pamięci; spróbuj ponownie");
                }
                else
                    message(state, hb_household_voice_error(HB_VEN_GATED));
                break;
            case HB_ID_CMD_VOICE_SAVE:
                if (voice_enrollment && c.person_id == voice_enrollment->staging.id &&
                    hb_voice_accepted(&voice_acceptance, &voice_policy))
                {
                    auto result = hb_household_voice_commit(voice_enrollment, database,
                                                            voice_database, voice_epoch, now_ms());
                    message(state, hb_household_voice_error(result));
                    if (result == HB_VEN_OK)
                        persistence_commit(state, false);
                }
                else
                    message(state, hb_household_voice_error(HB_VEN_GATED));
                break;
            case HB_ID_CMD_CREATE:
            {
                uint64_t created = hb_id_create(database, c.name);
                message(state, created ? "Profil utworzony w RAM" : "Nie można dodać profilu");
                if (created)
                    persistence_commit(state, false);
                // Saving publishes status. Do not expose the navigation receipt until
                // the storage attempt ends, even when the user continues in RAM.
                state.created_id = created;
                break;
            }
            case HB_ID_CMD_RENAME:
                if (hb_id_rename(database, c.person_id, c.name))
                    persistence_commit(state, false);
                else
                    message(state, "Nieprawidłowa nazwa");
                break;
            case HB_ID_CMD_DELETE:
                state.deleted_id = hb_id_delete(database, c.person_id) ? c.person_id : 0;
                memset(enrollment, 0, sizeof(*enrollment));
                state.mode = resting_mode(state.auto_enabled);
                message(state, "Profil i próbki usunięte z RAM");
                hb_household_voice_reconcile(database, voice_database);
                if (state.deleted_id)
                    persistence_commit(state, true);
                break;
            case HB_ID_CMD_BEGIN:
                capture = verify = false;
                guided = c.guided;
                state.saved_id = 0;
                state.sequence = 0;
                if (guided ? hb_enroll_fresh(database, enrollment, c.person_id)
                           : hb_id_enroll_begin(database, enrollment, c.person_id))
                {
                    state.mode = HB_ID_ENROLL;
                    until = now_ms() + 600000;
                    message(state, "Rejestracja w RAM: pobierz 8-15 różnych próbek");
                }
                break;
            case HB_ID_CMD_SAMPLE:
                enrollment->verified = false;
                capture = state.mode == HB_ID_ENROLL;
                break;
            case HB_ID_CMD_VERIFY:
                enrollment->verified = false;
                verify = state.mode == HB_ID_ENROLL;
                break;
            case HB_ID_CMD_SAVE:
                if (state.mode == HB_ID_ENROLL && hb_id_enroll_commit(database, enrollment))
                {
                    state.mode = resting_mode(state.auto_enabled);
                    pending_saved_id = c.person_id;
                    if (persistence_commit(state, false))
                    {
                        state.saved_id = c.person_id;
                        pending_saved_id = 0;
                    }
                }
                else
                    message(state, "Najpierw 8 próbek i weryfikacja nową klatką");
                break;
            case HB_ID_CMD_TEST:
                capture = verify = false;
                memset(enrollment, 0, sizeof(*enrollment));
                state.sequence = 0;
                state.mode = HB_ID_TEST;
                until = now_ms() + 600000;
                break;
            case HB_ID_CMD_AUTO:
                capture = verify = false;
                memset(enrollment, 0, sizeof(*enrollment));
                state.sequence = 0;
                state.mode = HB_ID_STOP;
                for (const auto &p : database->people)
                    if (p.id && p.enabled && p.count)
                        state.mode = HB_ID_AUTO;
                until = 0;
                message(state, state.mode == HB_ID_AUTO
                                   ? "AUTO aktywne; model pozostaje w pamięci"
                                   : "AUTO oczekuje: zarejestruj domownika i włącz AUTO");
                break;
            case HB_ID_CMD_BACKGROUND:
                capture = verify = false;
                memset(enrollment, 0, sizeof(*enrollment));
                state.sequence = 0;
                state.mode = HB_ID_BACKGROUND;
                until = now_ms() + 600000;
                ESP_LOGI("identity", "SESSION_BACKGROUND session=%u until_ms=%llu",
                         (unsigned)epoch.load(), (unsigned long long)until);
                break;
            case HB_ID_CMD_LANDMARK_METHOD:
                state.official_landmarks = !state.official_landmarks;
                break;
            case HB_ID_CMD_EXPECTED:
                state.expected_id = c.person_id;
                break;
            case HB_ID_CMD_EVALUATE:
                break;
            case HB_ID_CMD_CANCEL:
            case HB_ID_CMD_STOP:
                memset(enrollment, 0, sizeof(*enrollment));
                state.mode = HB_ID_STOP;
                capture = verify = false;
                message(state, "Rozpoznawanie zatrzymane");
                break;
            }
        }
        state.samples = enrollment->count;
        state.verified = enrollment->verified;
        uint32_t current_session = epoch.load();
        if (state.session != current_session)
        {
            state.confirmed = false;
            state.confirmed_id = 0;
            state.match = {HB_ID_UNAVAILABLE, 0, -1, -1};
            hb_id_tracker_reset(&tracker, current_session);
        }
        state.session = current_session;
        state.deadline_ms = state.mode == HB_ID_AUTO || state.mode == HB_ID_STOP ? 0 : until;
        session_deadline.store(state.deadline_ms);
        if (hb_id_expired(state.mode, until, now_ms()))
        {
            ESP_LOGI("identity", "SESSION_EXPIRED session=%u now_ms=%llu until_ms=%llu",
                     (unsigned)state.session, (unsigned long long)now_ms(),
                     (unsigned long long)until);
            state.mode = HB_ID_STOP;
            state.confirmed = false;
            memset(enrollment, 0, sizeof(*enrollment));
            capture = verify = false;
            message(state, "Koniec 10-minutowej sesji rozpoznawania");
        }
#if CONFIG_HB_IDENTITY_EXPERIMENTAL
        bool usable = false;
        for (const auto &p : database->people)
            usable |= p.id && p.enabled && p.count;
        if (state.mode == HB_ID_STOP || (state.mode != HB_ID_ENROLL && !usable))
        {
            if (hb_compute_enter(0))
            {
                if (models.mbf)
                    ++state.model_unloads;
                models.unload();
                state.ready = false;
                hb_compute_leave();
            }
        }
        else if (hb_compute_enter(0))
        {
            uint32_t ticket = state.session;
            if (ticket != epoch.load())
            {
                hb_compute_leave();
                continue;
            }
            state.confirmed = false;
            state.confirmed_id = 0;
            state.match = {HB_ID_UNAVAILABLE, 0, -1, -1};
            state.landmarks = {};
            state.roi = {};
            state.quality = {};
            state.detection_us = state.landmarks_us = state.embedding_us = state.total_us = 0;
            state.faces = 0;
            state.detection_valid = false;
            bool was_loaded = models.mbf != nullptr;
            if (!models.mbf && (load_attempts >= 3 || now_ms() < next_load_ms))
            {
                hb_compute_leave();
                publish(state);
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
            if (!models.mbf)
            {
                ++load_attempts;
                next_load_ms = now_ms() + 30000;
            }
            state.ready = models.load(state);
            if (state.ready)
                load_attempts = 0;
            if (state.ready && !was_loaded)
                ++state.model_loads;
            if (!state.ready)
                snprintf(state.last_error, sizeof(state.last_error), "%s", state.message);
            bool ok = state.ready && models.infer(state, embedding);
            static uint64_t last_inference_log;
            if (!ok && (!last_inference_log || now_ms() - last_inference_log >= 10000))
            {
                last_inference_log = now_ms();
                ESP_LOGI("identity",
                         "INFER_WAIT ready=%d faces=%u official=%d D/L/MBF_us=%u/%u/%u reason=%s",
                         state.ready, state.faces, state.official_landmarks,
                         (unsigned)state.detection_us, (unsigned)state.landmarks_us,
                         (unsigned)state.embedding_us, state.message);
            }
            if (!ok)
                snprintf(state.last_error, sizeof(state.last_error), "%s", state.message);
            state.completed_ms = now_ms();
            if (state.ready && state.detection_valid && state.sequence && ticket == epoch.load())
            {
                // Inference thumbnail is already display-corrected. Retain the exact frame for
                // overlays.
                xSemaphoreTake(guard, portMAX_DELAY);
                for (unsigned y = 0; y < 318; ++y)
                    for (unsigned x = 0; x < 180; ++x)
                        published_preview[y * 180 + x] =
                            models
                                .thumbnail[(y * HB_FACE_INPUT_HEIGHT / 318) * HB_FACE_INPUT_WIDTH +
                                           x * HB_FACE_INPUT_WIDTH / 180];
                preview_sequence = state.sequence;
                xSemaphoreGive(guard);
            }
            if (ticket == epoch.load() && !hb_id_expired(state.mode, until, now_ms()) &&
                state.completed_ms >= state.frame_ms && state.completed_ms - state.frame_ms <= 5000)
            {
                if (ok && state.mode == HB_ID_ENROLL)
                {
                    if (guided && !enrollment->verified)
                    {
                        auto step = hb_enroll_next(enrollment, state.faces, ok, state.sequence,
                                                   state.frame_ms);
                        capture = step == HB_ENROLL_SAMPLE;
                        verify = step == HB_ENROLL_VERIFY;
                    }
                    if (capture)
                        message(state, hb_id_enroll_sample(enrollment, &embedding, state.faces,
                                                           true, state.sequence, state.frame_ms)
                                           ? "Próbka przyjęta w RAM"
                                           : "Powtórz zdjęcie: zmień pozę lub oświetlenie");
                    if (verify)
                    {
                        bool accepted = hb_id_enroll_verify(enrollment, &embedding, state.sequence,
                                                            state.frame_ms, &policy);
                        // Reject a new enrollment also matching a different registered person.
                        auto other = hb_id_match_embedding(database, &embedding, &policy);
                        if (other.state == HB_ID_AMBIGUOUS ||
                            (other.state == HB_ID_KNOWN &&
                             other.person_id != enrollment->target_id))
                            accepted = enrollment->verified = false;
                        message(state, accepted ? "Weryfikacja OK. Potwierdź zapis próbek"
                                                : "Weryfikacja nieudana lub konflikt profili");
                    }
                    capture = verify = false;
                }
                else if (ok)
                {
                    state.match = hb_id_match_embedding(database, &embedding, &policy);
                    state.confirmed =
                        hb_id_confirm(&tracker, state.match, ticket, state.sequence, state.frame_ms,
                                      state.completed_ms, now_ms(), &policy);
                    if (state.confirmed)
                    {
                        state.confirmed_id = state.match.person_id;
                        state.last_recognized_ms = state.completed_ms;
                        auto *person = hb_id_find(database, state.confirmed_id);
                        if (person)
                            snprintf(state.last_recognized, sizeof(state.last_recognized), "%s",
                                     person->name);
                    }
                }
                else
                {
                    state.match.state = hb_id_detection_failure(state.detection_valid, state.faces);
                    hb_id_tracker_reset(&tracker, ticket);
                }
            }
            else
            {
                state.confirmed = false;
                hb_id_tracker_reset(&tracker, epoch.load());
            }
            if (evaluate && state.mode == HB_ID_TEST && ticket == epoch.load() &&
                !hb_id_expired(state.mode, until, now_ms()) && ++evaluation_frames >= 3)
            {
                if (state.sequence <= evaluation_sequence || !state.detection_valid)
                    hb_id_evaluation_unavailable(&state.evaluation, evaluation_expected);
                else if (hb_id_evaluate(&state.evaluation, evaluation_expected, state.match,
                                        state.confirmed, state.sequence, state.frame_ms))
                {
                    // No-face/failed preprocessing may have no complete timing receipt.
                    // Keep its outcome count, but do not fabricate a zero-latency sample.
                    if (state.total_us)
                    {
                        state.evaluation.latency_total_us += state.total_us;
                        ++state.evaluation.latency_samples;
                        if (state.total_us > state.evaluation.latency_max_us)
                            state.evaluation.latency_max_us = state.total_us;
                    }
                }
                evaluate = false;
                message(state, "Próba oceniona; zmień warunek przed kolejną próbą");
            }
            memset(&embedding, 0, sizeof(embedding));
            hb_compute_leave();
        }
#else
        if (state.mode != HB_ID_STOP)
        {
            state.ready = false;
            state.confirmed = false;
            message(state, "Model wyłączony. Wymagany profil recognition i model we Flash");
        }
        capture = verify = false;
#endif
        state.samples = enrollment->count;
        state.verified = enrollment->verified;
        if (hb_id_expired(state.mode, until, now_ms()))
        {
            state.mode = HB_ID_STOP;
            state.confirmed = false;
            state.confirmed_id = 0;
            state.match = {HB_ID_UNAVAILABLE, 0, -1, -1};
            epoch.fetch_add(1);
        }
        publish(state);
        identity_events(state);
        static uint64_t health_at;
        if (now_ms() >= health_at)
        {
            health_at = now_ms() + 30000;
            uint32_t overflow[HB_EVENT_CONSUMERS], stale[HB_EVENT_CONSUMERS];
            hb_identity_event_diagnostics(overflow, stale);
            ESP_LOGI("identity",
                     "HEALTH mode=%u ready=%d stack=%u models=%u/%u rejected=%u "
                     "overflow=%u/%u/%u stale=%u/%u/%u",
                     (unsigned)state.mode, state.ready, (unsigned)state.stack_min,
                     (unsigned)state.model_loads, (unsigned)state.model_unloads,
                     (unsigned)state.command_rejections, (unsigned)overflow[0],
                     (unsigned)overflow[1], (unsigned)overflow[2], (unsigned)stale[0],
                     (unsigned)stale[1], (unsigned)stale[2]);
            ESP_LOGI("identity",
                     "PROFILE_STATUS result=%u ready=%d saved=%d dirty=%d bank=%d generation=%llu",
                     state.storage_result, state.persistence_ready, state.storage_saved,
                     storage_dirty, state.storage_bank,
                     (unsigned long long)state.storage_generation);
        }
        // No overlapping inference, no unbounded queue. Commands are checked at least every 100ms
        // idle.
        for (unsigned j = 0; j < (state.mode == HB_ID_STOP ? 1U : 10U); ++j)
        {
            if (stop_requested.load() || uxQueueMessagesWaiting(jobs))
                break;
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
esp_err_t hb_identity_start(void)
{
    // Called before UI startup, or exclusively by the UI command producer.
    if (!jobs)
    {
        household = (hb_household_snapshot *)heap_caps_calloc(1, sizeof(*household),
                                                              MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        database = household ? &household->face : nullptr;
        voice_database = household ? &household->voice : nullptr;
        enrollment = (hb_id_enrollment *)heap_caps_calloc(1, sizeof(*enrollment),
                                                          MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        published_preview =
            (uint16_t *)heap_caps_calloc(180 * 318, 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        guard = xSemaphoreCreateMutex();
        jobs = xQueueCreate(8, sizeof(hb_id_command));
        if (!database || !enrollment || !published_preview || !guard || !jobs)
        {
            free(household);
            free(enrollment);
            free(published_preview);
            if (guard)
                vSemaphoreDelete(guard);
            if (jobs)
                vQueueDelete(jobs);
            guard = nullptr;
            jobs = nullptr;
            return ESP_ERR_NO_MEM;
        }
        hb_id_database_init(database, esp_random());
        hb_voice_database_init(voice_database);
        boot_id = ((uint64_t)esp_random() << 32) | esp_random();
        portENTER_CRITICAL(&event_lock);
        hb_event_bus_init(&event_bus);
        portEXIT_CRITICAL(&event_lock);
        if (xTaskCreate(worker, "local_identity", 12288, nullptr, 1, nullptr) != pdPASS)
        {
            free(household);
            free(enrollment);
            free(published_preview);
            vSemaphoreDelete(guard);
            vQueueDelete(jobs);
            guard = nullptr;
            jobs = nullptr;
            return ESP_ERR_NO_MEM;
        }
    }
    return ESP_OK;
}
esp_err_t hb_identity_command(const hb_id_command *c)
{
    if (!c || (unsigned)c->type > HB_ID_CMD_ENABLE || !memchr(c->name, 0, HB_ID_NAME))
        return ESP_ERR_INVALID_ARG;
    esp_err_t start = hb_identity_start();
    if (start)
        return start;
    portENTER_CRITICAL(&event_lock);
    if (c->type == HB_ID_CMD_STOP || c->type == HB_ID_CMD_CANCEL)
    {
        epoch.fetch_add(1);
        xQueueReset(jobs);
        if (c->type == HB_ID_CMD_STOP)
            disable_requested.store(true);
        stop_requested.store(true);
        portEXIT_CRITICAL(&event_lock);
        return ESP_OK;
    }
    bool accepted = xQueueSend(jobs, c, 0) == pdTRUE;
    if (accepted && c->type != HB_ID_CMD_EVALUATE)
        epoch.fetch_add(1);
    portEXIT_CRITICAL(&event_lock);
    if (accepted)
        return ESP_OK;
    command_rejections.fetch_add(1);
    return ESP_ERR_TIMEOUT;
}
void hb_identity_get(hb_id_status *out)
{
    if (!out)
        return;
    *out = {};
    if (!guard)
    {
        message(*out, "RAM-only: profile znikają po restarcie");
        return;
    }
    xSemaphoreTake(guard, portMAX_DELAY);
    *out = published;
    xSemaphoreGive(guard);
    if (out->session != epoch.load() || !out->completed_ms || !out->frame_ms ||
        out->frame_ms > now_ms() || now_ms() - out->frame_ms > 5000 ||
        now_ms() - out->completed_ms > 5000)
    {
        out->confirmed = false;
        out->confirmed_id = 0;
        out->match = {HB_ID_UNAVAILABLE, 0, -1, -1};
    }
}
bool hb_identity_preview(uint16_t *pixels, size_t capacity, hb_id_status *out)
{
    if (!guard || !pixels || !out || capacity < 180 * 318)
        return false;
    xSemaphoreTake(guard, portMAX_DELAY);
    bool valid = preview_sequence && preview_sequence == published.sequence &&
                 published.session == epoch.load() && published.completed_ms <= now_ms() &&
                 published.frame_ms <= now_ms() && now_ms() - published.frame_ms <= 5000 &&
                 now_ms() - published.completed_ms <= 5000;
    if (valid)
    {
        memcpy(pixels, published_preview, 180 * 318 * 2);
        *out = published;
    }
    xSemaphoreGive(guard);
    return valid;
}
bool hb_identity_greeting(uint64_t now, char name[HB_ID_NAME])
{
    struct Cooldown
    {
        uint64_t id, ms;
    };
    static Cooldown cooldown[HB_ID_PEOPLE]{};
    hb_id_event event{};
    bool found = false;
    while (hb_identity_event_read(HB_EVENTS_GREETING, &event))
        if (event.kind == HB_ID_EVENT_RECOGNIZED)
        {
            found = true;
            break;
        }
    if (!name || !found)
        return false;
    hb_id_status s{};
    hb_identity_get(&s);
    if (!hb_id_background(s.mode) || event.session != epoch.load())
        return false;
    for (auto &c : cooldown)
    {
        bool exists = false;
        for (unsigned i = 0; i < s.profile_count; ++i)
            exists |= s.profiles[i].id == c.id;
        if (!exists)
            c = {};
    }
    for (unsigned i = 0; i < s.profile_count; ++i)
        if (s.profiles[i].id == event.person_id)
        {
            for (auto &c : cooldown)
                if (c.id == event.person_id || !c.id)
                {
                    if (c.id && (now < c.ms || now - c.ms < 300000))
                        return false;
                    c = {event.person_id, now};
                    snprintf(name, HB_ID_NAME, "%s", s.profiles[i].name);
                    return true;
                }
        }
    return false;
}
bool hb_identity_event_read(hb_event_consumer consumer, hb_id_event *out)
{
    portENTER_CRITICAL(&event_lock);
    uint64_t deadline = session_deadline.load();
    bool ok = (!deadline || now_ms() < deadline) &&
              hb_event_pop(&event_bus, consumer, epoch.load(), now_ms(), out);
    portEXIT_CRITICAL(&event_lock);
    return ok;
}
bool hb_identity_event_current(const hb_id_event *event)
{
    uint64_t deadline = session_deadline.load();
    return (!deadline || now_ms() < deadline) && event && event->session == epoch.load() &&
           event->frame_ms <= now_ms() && now_ms() - event->frame_ms <= 5000;
}
void hb_identity_event_diagnostics(uint32_t overflow[HB_EVENT_CONSUMERS],
                                   uint32_t stale[HB_EVENT_CONSUMERS])
{
    portENTER_CRITICAL(&event_lock);
    memcpy(overflow, event_bus.overflow, sizeof(event_bus.overflow));
    memcpy(stale, event_bus.stale, sizeof(event_bus.stale));
    portEXIT_CRITICAL(&event_lock);
}
bool hb_identity_event(hb_id_event *out)
{
    return hb_identity_event_read(HB_EVENTS_LEGACY, out);
}
