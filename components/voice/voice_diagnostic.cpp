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
static unsigned slot;
static uint32_t capture_epoch;
static hb_voice_capture_result result_mailbox;
static bool result_pending;
static hb_voice_readiness last_report;
static bool report_ready;
static int64_t inference_deadline;
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
    }
    size_t remaining = HB_VOICE_SAMPLES - gate.samples;
    if (n > remaining)
        n = remaining;
    unsigned offset = gate.samples;
    hb_voice_gate_state state = hb_voice_gate_feed(&gate, pcm, n, rate, end_us / 1000);
    if (state == HB_VAD_REJECTED)
    {
        hb_voice_gate_reset(&gate);
        return;
    }
    memcpy(window + offset, pcm, n * sizeof(int16_t));
    if (state == HB_VAD_READY)
        ready.store(true);
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
        report.samples = gate.samples;
        report.rms = audio.rms;
        report.peak = audio.peak;
        report.clipped = audio.clipped;
        report.suspicious = audio.suspicious;
        hb_audio_stop();
        // Retire the bounded callback, independent of driver cleanup progress.
        hb_audio_detach_consumer(consume, nullptr);
        report.callback_detached = true;
        if (!ready.load() || cancelled.load())
            err = ESP_ERR_INVALID_STATE;
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
    portENTER_CRITICAL(&status_lock);
    if (capture_epoch && !cancelled.load())
    {
        result_mailbox.epoch = capture_epoch;
        result_mailbox.end_ms = gate.end_ms;
        result_mailbox.error = err;
        if (err == ESP_OK)
            result_mailbox.embedding = embedding;
        result_pending = true;
    }
    portEXIT_CRITICAL(&status_lock);
    memset(&embedding, 0, sizeof(embedding));
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
    last_report = report;
    report_ready = true;
    portEXIT_CRITICAL(&status_lock);
    hb_voice_gate_reset(&gate);
    ready.store(false);
    busy.store(false);
    vTaskDelete(nullptr);
}
static esp_err_t start(unsigned selected, uint32_t epoch)
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
    portENTER_CRITICAL(&status_lock);
    memset(&result_mailbox, 0, sizeof(result_mailbox));
    result_pending = false;
    report_ready = false;
    portEXIT_CRITICAL(&status_lock);
    cancelled.store(false);
    ready.store(false);
    hb_voice_gate_reset(&gate);
    if (xTaskCreate(worker, "voice_diag", 8192, nullptr, 1, nullptr) != pdPASS)
    {
        busy.store(false);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
esp_err_t hb_voice_diagnostic_start(unsigned selected)
{
    return start(selected, 0);
}
esp_err_t hb_voice_capture_start(unsigned selected, uint32_t epoch)
{
    return epoch ? start(selected, epoch) : ESP_ERR_INVALID_ARG;
}
