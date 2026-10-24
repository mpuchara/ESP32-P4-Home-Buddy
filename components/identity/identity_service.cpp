// SPDX-License-Identifier: Apache-2.0
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hb_face.h"
#include "hb_identity.h"
#include "hb_profile_flash.h"
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
static hb_id_database *database;
static hb_id_enrollment *enrollment;
static uint16_t *published_preview;
static uint32_t preview_sequence;
static portMUX_TYPE event_lock = portMUX_INITIALIZER_UNLOCKED;
static hb_event_bus event_bus;
static uint64_t boot_id;
static hb_profile_io profile_io{};
static hb_profile_revision profile_revision{0, -1};
static hb_id_database *storage_scratch;
// Initial engineering thresholds, deliberately not described as calibrated probabilities.
static const hb_id_policy policy{CONFIG_HB_IDENTITY_THRESHOLD_MILLI / 1000.0f,
                                 CONFIG_HB_IDENTITY_MARGIN_MILLI / 1000.0f,
                                 CONFIG_HB_IDENTITY_AGREEMENT_MILLI / 1000.0f, 3};
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
    for (const auto &p : database->people)
        if (p.id)
        {
            auto &d = s.profiles[s.profile_count++];
            d.id = p.id;
            d.samples = p.count;
            snprintf(d.name, sizeof(d.name), "%s", p.name);
        }
    s.psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
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
    if (s.mode != HB_ID_BACKGROUND || s.session != epoch.load() || !s.detection_valid ||
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
    e.persistent_id = s.persistence_ready;
    if (e.person_id)
        for (unsigned i = 0; i < s.profile_count; ++i)
            if (s.profiles[i].id == e.person_id)
                snprintf(e.person_name, sizeof(e.person_name), "%s", s.profiles[i].name);
    portENTER_CRITICAL(&event_lock);
    if (s.session == epoch.load())
        hb_event_push(&event_bus, e);
    portEXIT_CRITICAL(&event_lock);
}

static void persistence_start(hb_id_status &s)
{
    snprintf(s.persistence_message, sizeof(s.persistence_message),
             "RAM-only: brak bezpiecznego provisioningu; profile znikają po restarcie");
    if (!hb_profile_flash_open(&profile_io))
        return;
    storage_scratch = (hb_id_database *)heap_caps_calloc(1, sizeof(*database),
                                                         MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!storage_scratch)
        return;
    hb_store_result r =
        hb_profile_restore(&profile_io, database, storage_scratch, &profile_revision);
    s.persistence_ready = r == HB_STORE_OK || r == HB_STORE_EMPTY;
    snprintf(s.persistence_message, sizeof(s.persistence_message),
             s.persistence_ready ? "Profile: chroniony Flash, schemat 1"
                                 : "Baza profili uszkodzona lub niezgodna. Zapis zablokowany");
}

static void persistence_commit(hb_id_status &s, bool deletion)
{
    if (!s.persistence_ready)
        return;
    hb_store_result r =
        hb_profile_commit(&profile_io, database, storage_scratch, &profile_revision);
    if (r == HB_STORE_OK && deletion)
        r = hb_profile_scrub(&profile_io, &profile_revision);
    if (r != HB_STORE_OK)
    {
        // Never report unsaved RAM changes as durable. Re-read last valid committed copy.
        hb_store_result restored =
            hb_profile_restore(&profile_io, database, storage_scratch, &profile_revision);
        s.persistence_ready = false;
        snprintf(s.persistence_message, sizeof(s.persistence_message),
                 "Błąd zapisu (%u/%u). Zapis zablokowany; sprawdź bazę po restarcie", (unsigned)r,
                 (unsigned)restored);
        message(s, "Nie potwierdzono trwałego zapisu profili");
    }
    else
        message(s, deletion ? "Profil usunięty z Flash; stara kopia wymazana"
                            : "Profile zapisane we Flash");
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
    bool capture = false, verify = false;
    uint64_t until = 0;
#if CONFIG_HB_IDENTITY_EXPERIMENTAL
    Models models;
    hb_id_embedding embedding{};
#endif
    for (;;)
    {
        if (stop_requested.exchange(false))
        {
            memset(enrollment, 0, sizeof(*enrollment));
            state.mode = HB_ID_STOP;
            state.confirmed = false;
            state.confirmed_id = 0;
            state.match = {HB_ID_UNAVAILABLE, 0, -1, -1};
            capture = verify = false;
            hb_id_tracker_reset(&tracker, epoch.load());
            message(state, "Rozpoznawanie zatrzymane");
        }
        hb_id_command c{};
        while (xQueueReceive(jobs, &c, 0) == pdTRUE)
        {
            if (c.type == HB_ID_CMD_EVALUATE)
            {
                bool fresh = state.mode == HB_ID_TEST && state.completed_ms <= now_ms() &&
                             now_ms() - state.completed_ms <= 5000;
                bool recorded =
                    fresh && hb_id_evaluate(&state.evaluation, state.expected_id, state.match,
                                            state.confirmed, state.sequence, state.frame_ms);
                message(state, recorded ? "Próba oceniona lokalnie (bez zapisu zdjęć)"
                                        : "Brak nowej, aktualnej próby");
                continue;
            }
            state.confirmed = false;
            state.confirmed_id = 0;
            state.match = {HB_ID_UNAVAILABLE, 0, -1, -1};
            hb_id_tracker_reset(&tracker, epoch.load());
            switch (c.type)
            {
            case HB_ID_CMD_CREATE:
                message(state, hb_id_create(database, c.name) ? "Profil utworzony w RAM"
                                                              : "Nie można dodać profilu");
                persistence_commit(state, false);
                break;
            case HB_ID_CMD_RENAME:
                message(state, hb_id_rename(database, c.person_id, c.name) ? "Nazwa zmieniona"
                                                                           : "Nieprawidłowa nazwa");
                persistence_commit(state, false);
                break;
            case HB_ID_CMD_DELETE:
                hb_id_delete(database, c.person_id);
                memset(enrollment, 0, sizeof(*enrollment));
                state.mode = HB_ID_STOP;
                message(state, "Profil i próbki usunięte z RAM");
                persistence_commit(state, true);
                break;
            case HB_ID_CMD_BEGIN:
                capture = verify = false;
                state.sequence = 0;
                if (hb_id_enroll_begin(database, enrollment, c.person_id))
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
                    state.mode = HB_ID_STOP;
                    message(state, "Próbki zapisane tylko w RAM");
                    persistence_commit(state, false);
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
            case HB_ID_CMD_BACKGROUND:
                capture = verify = false;
                memset(enrollment, 0, sizeof(*enrollment));
                state.sequence = 0;
                state.mode = HB_ID_BACKGROUND;
                until = now_ms() + 600000;
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
        if (state.mode != HB_ID_STOP && now_ms() >= until)
        {
            state.mode = HB_ID_STOP;
            state.confirmed = false;
            memset(enrollment, 0, sizeof(*enrollment));
            capture = verify = false;
            message(state, "Koniec 10-minutowej sesji rozpoznawania");
        }
#if CONFIG_HB_IDENTITY_EXPERIMENTAL
        if (state.mode == HB_ID_STOP)
        {
            if (hb_compute_enter(0))
            {
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
            state.ready = models.load(state);
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
            if (ticket == epoch.load() && state.completed_ms >= state.frame_ms &&
                state.completed_ms - state.frame_ms <= 5000)
            {
                if (ok && state.mode == HB_ID_ENROLL)
                {
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
                        state.confirmed_id = state.match.person_id;
                }
                else
                    hb_id_tracker_reset(&tracker, ticket);
            }
            else
            {
                state.confirmed = false;
                hb_id_tracker_reset(&tracker, epoch.load());
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
        publish(state);
        identity_events(state);
        // No overlapping inference, no unbounded queue. Commands are checked at least every 100ms
        // idle.
        for (unsigned j = 0; j < (state.mode == HB_ID_STOP ? 1U : 10U); ++j)
        {
            if (uxQueueMessagesWaiting(jobs))
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
        database = (hb_id_database *)heap_caps_calloc(1, sizeof(*database),
                                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        enrollment = (hb_id_enrollment *)heap_caps_calloc(1, sizeof(*enrollment),
                                                          MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        published_preview =
            (uint16_t *)heap_caps_calloc(180 * 318, 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        guard = xSemaphoreCreateMutex();
        jobs = xQueueCreate(8, sizeof(hb_id_command));
        if (!database || !enrollment || !published_preview || !guard || !jobs)
        {
            free(database);
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
        boot_id = ((uint64_t)esp_random() << 32) | esp_random();
        portENTER_CRITICAL(&event_lock);
        hb_event_bus_init(&event_bus);
        portEXIT_CRITICAL(&event_lock);
        if (xTaskCreate(worker, "local_identity", 12288, nullptr, 1, nullptr) != pdPASS)
        {
            free(database);
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
    if (!c || c->type > HB_ID_CMD_EXPECTED || !memchr(c->name, 0, HB_ID_NAME))
        return ESP_ERR_INVALID_ARG;
    esp_err_t start = hb_identity_start();
    if (start)
        return start;
    if (c->type != HB_ID_CMD_EVALUATE)
        epoch.fetch_add(1); // Cancel in-flight publication immediately, even if queue is full.
    if (c->type == HB_ID_CMD_STOP || c->type == HB_ID_CMD_CANCEL)
    {
        // Safety stop cannot be refused by a full command queue. Later explicit starts may resume.
        xQueueReset(jobs);
        stop_requested.store(true);
        return ESP_OK;
    }
    return xQueueSend(jobs, c, 0) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
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
    if (s.mode != HB_ID_BACKGROUND || event.session != epoch.load())
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
    bool ok = hb_event_pop(&event_bus, consumer, epoch.load(), now_ms(), out);
    portEXIT_CRITICAL(&event_lock);
    return ok;
}
bool hb_identity_event_current(const hb_id_event *event)
{
    return event && event->session == epoch.load() && event->frame_ms <= now_ms() &&
           now_ms() - event->frame_ms <= 5000;
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
