// SPDX-License-Identifier: Apache-2.0
#include "hb_audio.h"
#include <math.h>
bool hb_audio_pcm_slot(const int16_t *stereo, size_t samples, uint32_t rate, unsigned slot,
                       int16_t *mono, size_t capacity, hb_audio_status *status)
{
    if (slot > 1 || !stereo || !mono || !status || !samples || samples % 2 || rate != 16000 ||
        samples / 2 > capacity || samples > 640)
        return false;
    int64_t sum = 0, square = 0;
    unsigned peak = 0, clipped = 0;
    size_t n = samples / 2;
    for (size_t i = 0; i < n; ++i)
    {
        int32_t v = stereo[2 * i + slot];
        mono[i] = (int16_t)v;
        unsigned a = v < 0 ? (unsigned)-v : (unsigned)v;
        if (a > peak)
            peak = a;
        clipped += a >= 32760;
        sum += v;
        square += (int64_t)v * v;
    }
    int64_t right_sum = 0, right_square = 0;
    unsigned right_peak = 0, right_clipped = 0;
    for (size_t i = 0; i < n; ++i)
    {
        int32_t v = stereo[2 * i + (1 - slot)];
        unsigned a = v < 0 ? (unsigned)-v : (unsigned)v;
        if (a > right_peak)
            right_peak = a;
        right_clipped += a >= 32760;
        right_sum += v;
        right_square += (int64_t)v * v;
    }
    status->selected_slot = slot;
    status->right_dc_offset = (int32_t)(right_sum / (int64_t)n);
    status->right_clipped = right_clipped;
    status->right_suspicious = false;
    for (size_t period = 1; period <= 8 && n >= period * 4; ++period)
    {
        bool repeats = true;
        for (size_t i = period; i < n; ++i)
            repeats &= stereo[2 * i + (1 - slot)] == stereo[2 * (i % period) + (1 - slot)];
        status->right_suspicious |= repeats;
    }
    status->right_peak = right_peak;
    status->right_rms = (uint32_t)sqrt((double)(right_square * (int64_t)n - right_sum * right_sum) /
                                       (double)(n * n));
    status->sample_count += n;
    status->dc_offset = (int32_t)(sum / (int64_t)n);
    status->suspicious = false;
    for (size_t period = 1; period <= 8 && n >= period * 4; ++period)
    {
        bool repeats = true;
        for (size_t i = period; i < n; ++i)
            repeats &= mono[i] == mono[i % period];
        status->suspicious |= repeats;
    }
    status->peak = peak;
    status->rms = (uint32_t)sqrt((double)(square * (int64_t)n - sum * sum) / (double)(n * n));
    status->clipped = clipped;
    status->sample_rate = rate;
    status->app_bytes = (uint32_t)(samples + n) * sizeof(int16_t);
    return true;
}

bool hb_audio_pcm(const int16_t *stereo, size_t samples, uint32_t rate, int16_t *mono,
                  size_t capacity, hb_audio_status *status)
{
    return hb_audio_pcm_slot(stereo, samples, rate, 0, mono, capacity, status);
}
