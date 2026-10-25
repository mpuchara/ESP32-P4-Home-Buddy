// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct
    {
        bool active, available;
        uint32_t sample_rate, frames, errors, dropped_samples, peak, rms;
        uint32_t app_bytes, dma_bytes, stack_min;
        uint64_t sample_count;
        int32_t dc_offset;
        uint32_t right_rms, right_peak, right_clipped;
        int32_t right_dc_offset;
        bool right_suspicious;
        unsigned selected_slot;
        uint64_t frame_end_us;
        uint32_t internal_free, internal_largest, psram_free, psram_largest, processing_us;
        bool suspicious; // Constant or short-period data, not a hardware acceptance decision.
        unsigned clipped;
        char message[120];
    } hb_audio_status;
    esp_err_t hb_audio_start(void);
    void hb_audio_stop(void);
    void hb_audio_get(hb_audio_status *out);
    // Pure stereo-left PCM16 conversion/measurement; no VAD or identity claim.
    bool hb_audio_pcm(const int16_t *stereo, size_t samples, uint32_t rate, int16_t *mono,
                      size_t capacity, hb_audio_status *status);

    // One synchronous consumer, on the capture worker. Frame lifetime ends on return.
    // Must not block, retain this pointer, allocate, run inference or call LVGL.
    typedef void (*hb_audio_consumer)(const int16_t *pcm, size_t count, uint32_t rate,
                                      uint64_t end_us, bool discontinuity, void *context);
    esp_err_t hb_audio_start_capture(unsigned slot, hb_audio_consumer consumer, void *context);
    bool hb_audio_busy(void);
    // Retire the bounded synchronous callback before its owner releases context/buffers.
    void hb_audio_detach_consumer(hb_audio_consumer callback, void *context);
    bool hb_audio_pcm_slot(const int16_t *stereo, size_t samples, uint32_t rate, unsigned slot,
                           int16_t *mono, size_t capacity, hb_audio_status *status);

#ifdef __cplusplus
}
#endif
