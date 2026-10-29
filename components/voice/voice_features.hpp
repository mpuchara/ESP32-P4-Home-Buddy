// SPDX-License-Identifier: Apache-2.0
// Private checked feature extractor, also compiled by the numerical reference test.
#pragma once
struct Features
{
    dl_fft_f32_t *fft = nullptr;
    float *window = nullptr, *cache = nullptr, *values = nullptr;
    dl::audio::mel_filter_t mel{};
    ~Features()
    {
        if (fft)
            dl_rfft_f32_deinit(fft);
        free(window);
        free(cache);
        free(values);
        free(mel.coeff);
        free(mel.bank_pos);
    }
    bool init()
    {
        const uint32_t caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
        window = dl::audio::win_func_init(dl::audio::WinType::HAMMING, 400, caps);
        cache = (float *)heap_caps_aligned_alloc(16, 512 * sizeof(float),
                                                 MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        values = (float *)heap_caps_malloc(24000 * sizeof(float), caps);
        mel.coeff = (float *)heap_caps_malloc(514 * sizeof(float), caps);
        mel.bank_pos = (int *)heap_caps_malloc(160 * sizeof(int), caps);
        if (!window || !cache || !values || !mel.coeff || !mel.bank_pos)
            return false;
        fft = dl_rfft_f32_init(512, caps);
        if (!fft || !fft->bitrev_table)
            return false;
        mel.nfilter = 80;
        float bin[82], bin_mel[257];
        float low = dl::audio::hz2mel(20), high = dl::audio::hz2mel(8000);
        for (int i = 0; i < 82; ++i)
            bin[i] = ((high - low) / 81 * i) + low;
        for (int i = 0; i < 257; ++i)
            bin_mel[i] = dl::audio::hz2mel(i * 8000.0f / 256);
        unsigned index = 0;
        for (int i = 0; i < 80; ++i)
        {
            int start = -1, stop = -1;
            for (int j = 1; j < 257; ++j)
            {
                float lower = (bin_mel[j] - bin[i]) / (bin[i + 1] - bin[i]);
                float upper = (bin[i + 2] - bin_mel[j]) / (bin[i + 2] - bin[i + 1]);
                if (lower > 0 && start == -1)
                    start = j;
                if (upper <= 0 && stop == -1)
                    stop = j - 1;
                float coefficient = fminf(lower, upper);
                if (coefficient > 0)
                {
                    if (index >= 514)
                        return false;
                    mel.coeff[index++] = coefficient;
                }
            }
            if (start < 1 || stop < start || stop > 256)
                return false;
            mel.bank_pos[2 * i] = start;
            mel.bank_pos[2 * i + 1] = stop;
        }
        return true;
    }
    esp_err_t run(const int16_t *pcm, size_t n, hb_voice_cancel_fn cancel, void *ctx)
    {
        for (unsigned t = 0; t < HB_VOICE_FRAMES; ++t)
        {
            if (cancel && cancel(ctx))
                return ESP_ERR_INVALID_STATE;
            if (!hb_voice_pcm_frame(pcm, n, t, cache))
                return ESP_ERR_INVALID_ARG;
            dl::audio::remove_dc_offset(cache, 400);
            dl::audio::apply_preemphasis(cache, 400, 0.97f, cache[0]);
            dl::audio::apply_window(cache, 400, window);
            memset(cache + 400, 0, 112 * sizeof(float));
            if (dl_rfft_f32_run(fft, cache) != ESP_OK)
                return ESP_FAIL;
            dl::audio::compute_spectrum(cache, 512, true);
            dl::audio::mel_dotprod(cache, &mel, values + t * 80);
            for (unsigned d = 0; d < 80; ++d)
                values[t * 80 + d] = logf(fmaxf(values[t * 80 + d], 1.1920928955078125e-7f));
            if (t % 8 == 0)
                vTaskDelay(1);
        }
        return ESP_OK;
    }
};
