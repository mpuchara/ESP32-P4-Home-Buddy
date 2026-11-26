// SPDX-License-Identifier: Apache-2.0
#include "fvad.h"
#include "hb_voice.h"
esp_err_t hb_voice_vad_window(const int16_t *pcm, size_t count, hb_voice_cancel_fn cancel,
                              void *context, float *fraction)
{
    if (fraction)
        *fraction = 0;
    if (!pcm || count != HB_VOICE_SAMPLES || !fraction)
        return ESP_ERR_INVALID_ARG;
    if (cancel && cancel(context))
        return ESP_ERR_INVALID_STATE;
    Fvad *vad = fvad_new();
    if (!vad)
        return ESP_ERR_NO_MEM;
    esp_err_t error = ESP_OK;
    unsigned positive = 0;
    if (fvad_set_sample_rate(vad, 16000) || fvad_set_mode(vad, 3))
        error = ESP_ERR_INVALID_STATE;
    for (unsigned i = 0; error == ESP_OK && i < 150; ++i)
    {
        if (cancel && cancel(context))
        {
            error = ESP_ERR_INVALID_STATE;
            break;
        }
        int vote = fvad_process(vad, pcm + i * 320, 320);
        if (vote < 0)
            error = ESP_ERR_INVALID_ARG;
        else
            positive += vote != 0;
    }
    fvad_reset(vad);
    fvad_free(vad);
    if (error == ESP_OK)
        *fraction = (float)positive / 150;
    return error;
}
