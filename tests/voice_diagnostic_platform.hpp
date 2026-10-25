// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_audio.h"
#include "hb_voice.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#define MALLOC_CAP_SPIRAM 0
#define MALLOC_CAP_8BIT 0
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_NO_MEM 0x101
#define ESP_ERR_TIMEOUT 0x107
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
#define pdPASS 1
#define pdMS_TO_TICKS(x) (x)
#define ESP_LOGI(...) ((void)0)
typedef int portMUX_TYPE;
namespace diag_fake
{
inline void (*task)(void *);
inline hb_audio_consumer consumer;
inline void *context;
inline bool active = false, alloc_fail = false, start_fail = false, task_fail = false;
inline bool silence = false, cancel_inference = false;
inline int model_error = 0, allocations = 0, detachments = 0, model_calls = 0;
inline int64_t time_us = 1;
inline void reset()
{
    assert(!active && !consumer && !allocations);
    task = nullptr;
    alloc_fail = start_fail = task_fail = silence = cancel_inference = false;
    model_error = detachments = model_calls = 0;
    time_us = 1;
}
inline void run()
{
    assert(task);
    auto fn = task;
    task = nullptr;
    fn(nullptr);
}
} // namespace diag_fake
inline void *heap_caps_calloc(size_t n, size_t size, unsigned)
{
    if (diag_fake::alloc_fail)
        return nullptr;
    ++diag_fake::allocations;
    return calloc(n, size);
}
inline void diagnostic_free(void *p)
{
    assert(!diag_fake::consumer);
    const auto *bytes = (const unsigned char *)p;
    for (unsigned i = 0; i < HB_VOICE_SAMPLES * sizeof(int16_t); ++i)
        assert(bytes[i] == 0);
    --diag_fake::allocations;
    std::free(p);
}
#define free diagnostic_free
inline int xTaskCreate(void (*fn)(void *), const char *, unsigned, void *, unsigned, void *)
{
    if (diag_fake::task_fail)
        return 0;
    diag_fake::task = fn;
    return pdPASS;
}
inline void vTaskDelete(void *)
{
}
inline int64_t esp_timer_get_time()
{
    return diag_fake::time_us;
}
inline void vTaskDelay(unsigned ms)
{
    diag_fake::time_us += ms * 1000;
    if (diag_fake::active && diag_fake::consumer)
    {
        int16_t pcm[320] = {0};
        if (!diag_fake::silence)
            for (unsigned i = 0; i < 320; ++i)
                pcm[i] = (int16_t)((i * 971) % 6000 - 3000);
        diag_fake::consumer(pcm, 320, 16000, diag_fake::time_us, false, diag_fake::context);
    }
}
inline const char *esp_err_to_name(int error)
{
    return error ? "ERROR" : "OK";
}
inline esp_err_t hb_audio_start_capture(unsigned, hb_audio_consumer consumer, void *context)
{
    if (diag_fake::start_fail)
        return ESP_ERR_INVALID_STATE;
    diag_fake::consumer = consumer;
    diag_fake::context = context;
    diag_fake::active = true;
    return ESP_OK;
}
inline bool hb_audio_busy()
{
    return diag_fake::active;
}
inline void hb_audio_stop()
{
    diag_fake::active = false;
}
inline void hb_audio_detach_consumer(hb_audio_consumer consumer, void *context)
{
    assert(diag_fake::consumer == consumer && diag_fake::context == context);
    diag_fake::consumer = nullptr;
    ++diag_fake::detachments;
}
inline esp_err_t hb_voice_embedding_run(const int16_t *, size_t n, uint32_t rate,
                                        hb_voice_cancel_fn, void *, hb_voice_embedding *out,
                                        hb_voice_metrics *m)
{
    assert(!diag_fake::consumer && !diag_fake::active && n == HB_VOICE_SAMPLES && rate == 16000);
    ++diag_fake::model_calls;
    out->v[0] = 1;
    m->load_us = 1;
    if (diag_fake::cancel_inference)
        hb_voice_diagnostic_stop();
    return diag_fake::model_error;
}
