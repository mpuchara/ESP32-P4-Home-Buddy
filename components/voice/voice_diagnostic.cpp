// SPDX-License-Identifier: Apache-2.0
#ifdef HB_VOICE_DIAG_HOST_TEST
#include "voice_diagnostic_platform.hpp"
#else
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hb_audio.h"
#include "hb_voice.h"
#endif
#include <atomic>
#include <cstdio>
#include <cstring>
static std::atomic<bool> busy{false}, cancelled{false}, ready{false};
static portMUX_TYPE status_lock = portMUX_INITIALIZER_UNLOCKED;
static char message[160] = "Model głosu: test wyłączony";
static int16_t *window;
static hb_voice_gate gate;
static hb_voice_onset onset;
static bool automatic_capture;
static unsigned slot;
static uint32_t capture_epoch;
static hb_voice_capture_result result_mailbox;
static bool result_pending;
static hb_voice_readiness last_report;
static bool report_ready, standalone_ready;
static hb_voice_readiness standalone_report;
static int64_t inference_deadline;
struct AutoSlot
{
    int16_t *pcm;
    hb_voice_gate gate;
    std::atomic<bool> full;
};
static AutoSlot buffers[2];
static unsigned filling;
static std::atomic<uint32_t> skipped_frames;
static bool capture_gap;
static hb_voice_onset auto_onset;
bool hb_voice_diagnostic_report(hb_voice_readiness *out)
{
    if (!out)
        return false;
    portENTER_CRITICAL(&status_lock);
    bool present = report_ready;
    if (present)
        *out = last_report;
    portEXIT_CRITICAL(&status_lock);
    return present;
}
bool hb_voice_diagnostic_standalone_report(hb_voice_readiness *out)
{
    if (!out)
        return false;
    portENTER_CRITICAL(&status_lock);
    bool present = standalone_ready;
    if (present)
        *out = standalone_report;
    portEXIT_CRITICAL(&status_lock);
    return present;
}
bool hb_voice_capture_busy(void)
{
    return busy.load();
}
bool hb_voice_capture_result_pending(void)
{
    portENTER_CRITICAL(&status_lock);
    bool present = result_pending;
    portEXIT_CRITICAL(&status_lock);
    return present;
}
bool hb_voice_capture_take(hb_voice_capture_result *out)
{
    if (!out)
        return false;
    portENTER_CRITICAL(&status_lock);
    bool present = result_pending;
    if (present)
    {
        *out = result_mailbox;
        memset(&result_mailbox, 0, sizeof(result_mailbox));
        result_pending = false;
    }
    portEXIT_CRITICAL(&status_lock);
    return present;
}
static void status(const char *text)
{
    portENTER_CRITICAL(&status_lock);
    snprintf(message, sizeof(message), "%s", text);
    portEXIT_CRITICAL(&status_lock);
}
void hb_voice_diagnostic_status(char *text, size_t capacity)
{
    if (!text || !capacity)
        return;
    portENTER_CRITICAL(&status_lock);
    snprintf(text, capacity, "%s", message);
    portEXIT_CRITICAL(&status_lock);
}
static bool cancel(void *)
{
    return cancelled.load() || (inference_deadline && esp_timer_get_time() > inference_deadline);
}
void hb_voice_diagnostic_stop(void)
{
    cancelled.store(true);
    portENTER_CRITICAL(&status_lock);
    memset(&result_mailbox, 0, sizeof(result_mailbox));
    result_pending = false;
    portEXIT_CRITICAL(&status_lock);
}
static void consume(const int16_t *pcm, size_t n, uint32_t rate, uint64_t end_us,
                    bool discontinuity, void *)
{
    if (ready.load() || cancelled.load())
        return;
    if (discontinuity)
    {
        hb_voice_gate_reset(&gate);
        onset.run = 0;
    }
    if (automatic_capture && onset.run < 3 &&
        (rate != 16000 || !hb_voice_onset_feed(&onset, pcm, n)))
    {
        hb_voice_gate_reset(&gate);
        onset.run = 0;
        return;
    }
    size_t remaining = HB_VOICE_SAMPLES - gate.samples;
    if (n > remaining)
        n = remaining;
    unsigned offset = gate.samples;
    hb_voice_gate_state state = hb_voice_gate_feed(&gate, pcm, n, rate, end_us / 1000);
    if (state == HB_VAD_REJECTED)
    {
        hb_voice_gate_reset(&gate);
        onset.run = 0;
        return;
    }
    memcpy(window + offset, pcm, n * sizeof(int16_t));
    if (state == HB_VAD_READY)
        ready.store(true);
}
static void auto_consume(const int16_t *pcm, size_t n, uint32_t rate, uint64_t end_us,
                         bool discontinuity, void *)
{
    if (cancelled.load())
        return;
    auto &b = buffers[filling];
    if (b.full.load())
    {
        ++skipped_frames;
        capture_gap = true;
        return;
    }
    if (discontinuity || capture_gap)
    {
        hb_voice_gate_reset(&b.gate);
        auto_onset.run = 0;
        capture_gap = false;
    }
    if (auto_onset.run < 3 && (rate != 16000 || !hb_voice_onset_feed(&auto_onset, pcm, n)))
    {
        hb_voice_gate_reset(&b.gate);
        auto_onset.run = 0;
        return;
    }
    size_t remaining = HB_VOICE_SAMPLES - b.gate.samples;
    if (n > remaining)
        n = remaining;
    unsigned offset = b.gate.samples;
    auto state = hb_voice_gate_feed(&b.gate, pcm, n, rate, end_us / 1000);
    if (state == HB_VAD_REJECTED)
    {
        hb_voice_gate_reset(&b.gate);
        auto_onset.run = 0;
        return;
    }
    memcpy(b.pcm + offset, pcm, n * sizeof(int16_t));
    if (state == HB_VAD_READY)
    {
        b.full.store(true); // Publish immutable PCM and timestamp to the consumer.
        filling = (filling + 1) % 2;
        auto_onset.run = 0;
    }
}
static bool auto_publish(const hb_voice_capture_result &result)
{
    // Backpressure: never overwrite a household result. Bounded wait, cancellation wins.
    int64_t until = esp_timer_get_time() + 5000000;
    while (!cancelled.load() && hb_voice_capture_result_pending() && esp_timer_get_time() < until)
        vTaskDelay(pdMS_TO_TICKS(20));
    bool sent = false;
    portENTER_CRITICAL(&status_lock);
    if (!cancelled.load() && !result_pending)
    {
        result_mailbox = result;
        result_pending = true;
        sent = true;
    }
    portEXIT_CRITICAL(&status_lock);
    return sent;
}
static void auto_worker(void *)
{
    hb_voice_capture_result result{};
    result.epoch = capture_epoch;
    esp_err_t err = ESP_OK;
    filling = 0;
    skipped_frames.store(0);
    capture_gap = false;
    auto_onset = {};
    for (auto &b : buffers)
    {
        b.full.store(false);
        hb_voice_gate_reset(&b.gate);
        b.pcm = (int16_t *)heap_caps_calloc(HB_VOICE_SAMPLES, sizeof(int16_t),
                                            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!b.pcm)
            err = ESP_ERR_NO_MEM;
    }
    bool owns_audio = false;
    if (err == ESP_OK && !cancelled.load())
    {
        err = hb_audio_start_continuous(slot, auto_consume, nullptr);
        owns_audio = err == ESP_OK;
    }
    status("Głos AUTO: ciągły nasłuch lokalny");
    unsigned reading = 0;
    int64_t heartbeat = esp_timer_get_time() + 60000000;
    while (owns_audio && !cancelled.load())
    {
        auto &b = buffers[reading];
        if (!hb_audio_busy())
        {
            err = ESP_ERR_INVALID_STATE;
            break;
        }
        if (!b.full.load())
        {
            if (esp_timer_get_time() >= heartbeat)
            {
                hb_audio_status audio{};
                hb_audio_get(&audio);
                if (!audio.available || !audio.frames || audio.errors)
                {
                    err = ESP_ERR_INVALID_STATE;
                    break;
                }
                result = {};
                result.epoch = capture_epoch;
                result.error = ESP_ERR_INVALID_STATE;
                result.no_window = true;
                result.more = true;
                result.skipped_frames = skipped_frames.load();
                if (!auto_publish(result))
                {
                    err = ESP_ERR_TIMEOUT;
                    break;
                }
                heartbeat = esp_timer_get_time() + 60000000;
            }
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        result = {};
        result.epoch = capture_epoch;
        result.end_ms = b.gate.end_ms;
        hb_voice_metrics metrics{};
        // Do not waste a model run on a queued window already too old to identify.
        uint64_t now = esp_timer_get_time() / 1000;
        bool stale = result.end_ms > now || now - result.end_ms > 5000;
        float support = 0;
        if (!stale)
            result.screening = hb_voice_screen_window(b.pcm, HB_VOICE_SAMPLES, &support);
        if (!stale && result.screening == HB_VOICE_SCREEN_PASS)
        {
            // The identity freshness budget includes queueing, hashing and inference.
            inference_deadline = (int64_t)(result.end_ms + 5000) * 1000;
            err = hb_voice_embedding_run(b.pcm, HB_VOICE_SAMPLES, 16000, cancel, nullptr,
                                         &result.embedding, &metrics);
            bool expired = esp_timer_get_time() >= inference_deadline;
            inference_deadline = 0;
            if (expired &&
                (err == ESP_OK || err == ESP_ERR_INVALID_STATE || err == ESP_ERR_TIMEOUT))
            {
                stale = true;
                err = ESP_OK;
            }
            if (err == ESP_OK && !stale && !hb_voice_normalize(&result.embedding))
                err = ESP_ERR_INVALID_STATE;
            now = esp_timer_get_time() / 1000;
            stale |= now < result.end_ms || now - result.end_ms > 5000;
        }
        result.error = err;
        result.processing_ms =
            (metrics.load_us + metrics.features_us + metrics.inference_us) / 1000;
        if (err == ESP_OK && !stale && result.screening == HB_VOICE_SCREEN_PASS)
        {
            float activity = (float)b.gate.active_samples / HB_VOICE_SAMPLES;
            result.quality = support < activity ? support : activity;
        }
        else
            memset(&result.embedding, 0, sizeof(result.embedding));
        if (err == ESP_OK && stale)
        {
            result.error = ESP_ERR_INVALID_STATE;
            result.stale = true;
        }
        else if (err == ESP_OK && result.screening != HB_VOICE_SCREEN_PASS)
            result.error = ESP_ERR_INVALID_STATE;
        // Release only after all reads and scrubbing; producer acquires full=false.
        memset(b.pcm, 0, HB_VOICE_SAMPLES * sizeof(int16_t));
        hb_voice_gate_reset(&b.gate);
        b.full.store(false);
        reading = (reading + 1) % 2;
        if (err != ESP_OK || cancelled.load())
            break;
        result.more = true;
        result.skipped_frames = skipped_frames.load();
        if (!auto_publish(result))
        {
            err = ESP_ERR_TIMEOUT;
            break;
        }
        heartbeat = esp_timer_get_time() + 60000000;
    }
    if (owns_audio)
    {
        hb_audio_stop();
        hb_audio_detach_consumer(auto_consume, nullptr);
    }
    for (auto &b : buffers)
    {
        if (b.pcm)
        {
            memset(b.pcm, 0, HB_VOICE_SAMPLES * sizeof(int16_t));
            free(b.pcm);
            b.pcm = nullptr;
        }
        hb_voice_gate_reset(&b.gate);
        b.full.store(false);
    }
    result = {};
    result.epoch = capture_epoch;
    result.error = err == ESP_OK ? ESP_ERR_INVALID_STATE : err;
    result.skipped_frames = skipped_frames.load();
    if (!cancelled.load())
        auto_publish(result);
    memset(&result, 0, sizeof(result));
    busy.store(false);
    vTaskDelete(nullptr);
}
static void worker(void *)
{
    status("Model głosu: przygotowanie lokalnego testu");
    window = (int16_t *)heap_caps_calloc(HB_VOICE_SAMPLES, sizeof(int16_t),
                                         MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    esp_err_t err = window ? ESP_OK : ESP_ERR_NO_MEM;
    bool owns_audio = false;
    hb_audio_status audio{};
    hb_voice_readiness report{};
    report.slot = slot;
    report.standalone = capture_epoch == 0;
    if (err == ESP_OK && !cancelled.load())
    {
        err = hb_audio_start_capture(slot, consume, nullptr);
        owns_audio = err == ESP_OK;
    }
    if (owns_audio)
    {
        status("Model głosu: mów normalnie; filtr aktywności jest eksperymentalny");
        int64_t deadline = esp_timer_get_time() + 60000000;
        while (!cancelled.load() && !ready.load() && hb_audio_busy() &&
               esp_timer_get_time() < deadline)
            vTaskDelay(pdMS_TO_TICKS(20));
        hb_audio_get(&audio);
        report.sample_rate = audio.sample_rate;
        report.rms = audio.rms;
        report.peak = audio.peak;
        report.clipped = audio.clipped;
        report.suspicious = audio.suspicious;
        hb_audio_stop();
        // Retire the bounded callback, independent of driver cleanup progress.
        hb_audio_detach_consumer(consume, nullptr);
        report.callback_detached = true;
        report.samples = gate.samples;
        report.active_samples = gate.active_samples;
        if (!ready.load() || cancelled.load())
        {
            report.no_window = !cancelled.load() && audio.available && audio.frames &&
                               !audio.errors && !audio.dropped_samples;
            err = ESP_ERR_INVALID_STATE;
        }
    }
    else if (err == ESP_OK)
        err = ESP_ERR_INVALID_STATE;
    hb_voice_embedding embedding{};
    hb_voice_metrics metrics{};
    if (err == ESP_OK)
    {
        status("Model głosu: lokalne osadzenie; brak identyfikacji osoby");
        inference_deadline = esp_timer_get_time() + 120000000;
        err = hb_voice_embedding_run(window, HB_VOICE_SAMPLES, 16000, cancel, nullptr, &embedding,
                                     &metrics);
        if (esp_timer_get_time() > inference_deadline)
            err = ESP_ERR_TIMEOUT;
        inference_deadline = 0;
        if (err == ESP_OK && !hb_voice_normalize(&embedding))
            err = ESP_ERR_INVALID_STATE;
        if (err == ESP_OK)
            report.dimensions = HB_VOICE_DIM;
    }
    char result[160];
    if (cancelled.load())
    {
        err = ESP_ERR_INVALID_STATE;
        report.dimensions = 0;
    }
    snprintf(result, sizeof(result), "Model głosu: %s; load/pre/infer %lu/%lu/%lu ms",
             esp_err_to_name(err), (unsigned long)(metrics.load_us / 1000),
             (unsigned long)(metrics.features_us / 1000),
             (unsigned long)(metrics.inference_us / 1000));
    status(result);
    if (window)
    {
        memset(window, 0, HB_VOICE_SAMPLES * sizeof(int16_t));
        free(window);
        window = nullptr;
    }
    report.pcm_released = window == nullptr;
    report.error = err;
    report.model = metrics;
    ESP_LOGI("voice",
             "VOICE_READINESS error=%d slot=%u rate=%lu samples=%lu rms=%lu peak=%lu clipped=%lu "
             "suspicious=%u dim=%u detached=%u pcm_released=%u load_us=%lu features_us=%lu "
             "inference_us=%lu internal_low=%lu psram_low=%lu stack=%lu; diagnostic only",
             (int)err, slot, (unsigned long)report.sample_rate, (unsigned long)report.samples,
             (unsigned long)report.rms, (unsigned long)report.peak, (unsigned long)report.clipped,
             (unsigned)report.suspicious, report.dimensions, (unsigned)report.callback_detached,
             (unsigned)report.pcm_released, (unsigned long)metrics.load_us,
             (unsigned long)metrics.features_us, (unsigned long)metrics.inference_us,
             (unsigned long)metrics.internal_low, (unsigned long)metrics.psram_low,
             (unsigned long)metrics.stack_min);
    portENTER_CRITICAL(&status_lock);
    if (cancelled.load())
    {
        report.error = ESP_ERR_INVALID_STATE;
        report.dimensions = 0;
    }
    last_report = report;
    report_ready = true;
    if (report.standalone)
    {
        standalone_report = report;
        standalone_ready = true;
    }
    if (capture_epoch && !cancelled.load())
    {
        result_mailbox.epoch = capture_epoch;
        result_mailbox.end_ms = gate.end_ms;
        result_mailbox.error = err;
        result_mailbox.no_window = report.no_window;
        result_mailbox.processing_ms =
            (metrics.load_us + metrics.features_us + metrics.inference_us) / 1000;
        if (err == ESP_OK)
        {
            result_mailbox.embedding = embedding;
            result_mailbox.quality = (float)report.active_samples / HB_VOICE_SAMPLES;
        }
        result_pending = true;
    }
    portEXIT_CRITICAL(&status_lock);
    memset(&embedding, 0, sizeof(embedding));
    hb_voice_gate_reset(&gate);
    ready.store(false);
    busy.store(false);
    vTaskDelete(nullptr);
}
static esp_err_t start(unsigned selected, uint32_t epoch, bool automatic)
{
    if (selected > 1)
        return ESP_ERR_INVALID_ARG;
    bool expected = false;
    if (!busy.compare_exchange_strong(expected, true))
        return ESP_ERR_INVALID_STATE;
    portENTER_CRITICAL(&status_lock);
    bool pending = result_pending;
    portEXIT_CRITICAL(&status_lock);
    if (pending)
    {
        busy.store(false);
        return ESP_ERR_INVALID_STATE; // Never overwrite an unconsumed accepted result.
    }
    slot = selected;
    capture_epoch = epoch;
    automatic_capture = automatic;
    onset = {};
    portENTER_CRITICAL(&status_lock);
    memset(&result_mailbox, 0, sizeof(result_mailbox));
    result_pending = false;
    report_ready = false;
    if (!epoch)
        standalone_ready = false;
    portEXIT_CRITICAL(&status_lock);
    cancelled.store(false);
    ready.store(false);
    hb_voice_gate_reset(&gate);
    if (xTaskCreate(automatic ? auto_worker : worker, "voice_diag", 8192, nullptr, 1, nullptr) !=
        pdPASS)
    {
        busy.store(false);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
esp_err_t hb_voice_diagnostic_start(unsigned selected)
{
    return start(selected, 0, false);
}
esp_err_t hb_voice_capture_start(unsigned selected, uint32_t epoch)
{
    return epoch ? start(selected, epoch, false) : ESP_ERR_INVALID_ARG;
}
esp_err_t hb_voice_capture_start_auto(unsigned selected, uint32_t epoch)
{
    return epoch ? start(selected, epoch, true) : ESP_ERR_INVALID_ARG;
}
