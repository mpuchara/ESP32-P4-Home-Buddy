// SPDX-License-Identifier: Apache-2.0
#include "features_platform.hpp"

#include "../components/voice/voice_features.hpp"
#include <cfenv>
#include <cstdio>
static int16_t pcm[HB_VOICE_SAMPLES], candidate[24000], reference[24000];
static float ref[24000], scratch[24000];
int main()
{
    std::fesetround(FE_TONEAREST);
    float fft_probe[512] = {1};
    dl_fft_f32_t *fft = dl_rfft_f32_init(512, 0);
    assert(dl_rfft_f32_run(fft, fft_probe) == ESP_OK);
    dl::audio::compute_spectrum(fft_probe, 512, true);
    for (unsigned i = 0; i <= 256; ++i)
        assert(fabsf(fft_probe[i] - 1) < 1e-6f);
    for (unsigned i = 0; i < 512; ++i)
        fft_probe[i] = i % 2 ? -1 : 1;
    assert(dl_rfft_f32_run(fft, fft_probe) == ESP_OK);
    dl::audio::compute_spectrum(fft_probe, 512, true);
    assert(fft_probe[0] == 0 && fft_probe[256] == 512 * 512);
    dl_rfft_f32_deinit(fft);
    Features features;
    assert(features.init());
    dl::audio::Fbank upstream;
    upstream.m_win_func = dl::audio::win_func_init(dl::audio::WinType::HAMMING, 400);
    upstream.m_fft_config = dl_rfft_f32_init(512, 0);
    upstream.m_mel_filter = dl::audio::mel_filter_init(512, 80, 20, 0, 16000, 0);
    const char *names[] = {"silence", "sine", "multitone", "chirp", "noise", "transient"};
    for (unsigned fixture = 0; fixture < 6; ++fixture)
    {
        uint32_t rng = 12345;
        for (unsigned i = 0; i < HB_VOICE_SAMPLES; ++i)
        {
            double t = i / 16000.0, v = 0;
            rng = rng * 1664525u + 1013904223u;
            if (fixture == 1)
                v = 9000 * sin(2 * 3.141592653589793 * 440 * t);
            if (fixture == 2)
                v = 4000 * sin(2 * 3.141592653589793 * 180 * t) +
                    3000 * cos(2 * 3.141592653589793 * 1200 * t);
            if (fixture == 3)
                v = 10000 * sin(2 * 3.141592653589793 * (100 * t + 900 * t * t));
            if (fixture == 4)
                v = (int)(rng >> 16) - 32768;
            if (fixture == 5 && i % 1000 == 0)
                v = 24000;
            pcm[i] = (int16_t)v;
        }
        assert(features.run(pcm, HB_VOICE_SAMPLES, nullptr, nullptr) == ESP_OK);
        float frame[400];
        for (unsigned t = 0; t < 300; ++t)
        {
            // Independent full-buffer reference normalization/indexing.
            for (unsigned j = 0; j < 400; ++j)
                frame[j] = pcm[t * 160 + j] / 32768.0f;
            assert(upstream.process_frame(frame, 400, ref + t * 80) == ESP_OK);
        }
        float maximum = 0;
        for (unsigned i = 0; i < 24000; ++i)
        {
            assert(std::isfinite(features.values[i]));
            maximum = std::max(maximum, fabsf(features.values[i] - ref[i]));
        }
        assert(maximum <= 2e-5f);
        for (int exponent : {-12, -8, -4, 0})
        {
            memcpy(scratch, ref, sizeof(ref));
            for (unsigned d = 0; d < 80; ++d)
            {
                float mean = 0;
                for (unsigned t = 0; t < 300; ++t)
                    mean += scratch[t * 80 + d];
                mean /= 300;
                for (unsigned t = 0; t < 300; ++t)
                    scratch[t * 80 + d] -= mean;
            }
            for (unsigned i = 0; i < 24000; ++i)
                reference[i] = (int16_t)std::max(
                    -32768.0f, std::min(32767.0f, nearbyintf(ldexpf(scratch[i], -exponent))));
            memcpy(scratch, features.values, sizeof(scratch));
            assert(hb_voice_quantize(scratch, 24000, exponent, candidate));
            for (unsigned i = 0; i < 24000; ++i)
                assert(abs((int)candidate[i] - reference[i]) <= 1);
        }
        printf("%s log_mel_max_abs=%g; CMVN/INT16 <=1 LSB (4 exponents)\n", names[fixture],
               maximum);
    }
    free(upstream.m_win_func);
    dl::audio::mel_filter_deinit(upstream.m_mel_filter);
    dl_rfft_f32_deinit(upstream.m_fft_config);
}
