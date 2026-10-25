// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include "hb_voice_core.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct
    {
        uint32_t load_us, features_us, inference_us;
        uint32_t internal_free, internal_largest, psram_free, psram_largest;
        uint32_t internal_low, psram_low, stack_min;
    } hb_voice_metrics;
    // Worker-only, caller owns immutable validated mono PCM and cancellation context.
    // Does not publish identities or persist data. Temporaries/model are always released.
    esp_err_t hb_voice_embedding_run(const int16_t *pcm, size_t count, uint32_t rate,
                                     hb_voice_cancel_fn cancel, void *context,
                                     hb_voice_embedding *out, hb_voice_metrics *metrics);
    // Explicit, one-shot diagnostic. No enrollment, identity, HA or continuous listening.
    esp_err_t hb_voice_diagnostic_start(unsigned slot);
    void hb_voice_diagnostic_stop(void);
    void hb_voice_diagnostic_status(char *text, size_t capacity);
#ifdef __cplusplus
}
#endif
