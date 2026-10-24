// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct
{
    bool active, available;
    uint32_t sample_rate, frames, errors, dropped_samples, peak, rms;
    uint32_t app_bytes, dma_bytes, stack_min;
    unsigned clipped;
    char message[120];
} hb_audio_status;
esp_err_t hb_audio_start(void);
void hb_audio_stop(void);
void hb_audio_get(hb_audio_status *out);
// Pure stereo-left PCM16 conversion/measurement; no VAD or identity claim.
bool hb_audio_pcm(const int16_t *stereo, size_t samples, uint32_t rate, int16_t *mono,
                  size_t capacity, hb_audio_status *status);
