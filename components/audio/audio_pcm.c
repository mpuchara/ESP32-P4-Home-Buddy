// SPDX-License-Identifier: Apache-2.0
#include "hb_audio.h"
#include <math.h>
bool hb_audio_pcm(const int16_t *stereo, size_t samples, uint32_t rate, int16_t *mono,
                  size_t capacity, hb_audio_status *status)
{
    if (!stereo || !mono || !status || !samples || samples % 2 || rate != 16000 ||
        samples / 2 > capacity || samples > 640)
        return false;
    int64_t sum = 0, square = 0;
    unsigned peak = 0, clipped = 0;
    size_t n = samples / 2;
    for (size_t i = 0; i < n; ++i)
    {
        int32_t v = stereo[2 * i];
        mono[i] = (int16_t)v;
        unsigned a = v < 0 ? (unsigned)-v : (unsigned)v;
        if (a > peak)
            peak = a;
        clipped += a >= 32760;
        sum += v;
        square += (int64_t)v * v;
    }
    status->peak = peak;
    status->rms = (uint32_t)sqrt((double)(square * (int64_t)n - sum * sum) / (double)(n * n));
    status->clipped = clipped;
    status->sample_rate = rate;
    status->app_bytes = (uint32_t)(samples + n) * sizeof(int16_t);
    return true;
}
