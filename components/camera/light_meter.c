// SPDX-License-Identifier: Apache-2.0
#include "hb_light_meter.h"
#include <math.h>
#include <stddef.h>
hb_light_reading hb_light_histogram(const unsigned hist[256], float exposure, float again,
                                    float dgain, bool metadata)
{
    hb_light_reading out = {0};
    if (!hist || !metadata || !isfinite(exposure) || !isfinite(again) || !isfinite(dgain) ||
        exposure < 1 || exposure > 1000000 || again < 1 || again > 32 || dgain < 1 || dgain > 32)
        return out;
    unsigned total = 0, dark = 0, bright = 0;
    for (unsigned i = 0; i < 256; i++)
    {
        if (hist[i] > 4096 - total)
            return out;
        total += hist[i];
        if (i < 5)
            dark += hist[i];
        if (i > 245)
            bright += hist[i];
    }
    if (total < 64)
        return out;
    unsigned rejected = dark + bright;
    out.rejected_fraction = (float)rejected / total;
    unsigned good = total, sum = 0, count = 0, index = 0;
    // Trim darkest/brightest quartiles of the valid pixels.
    for (unsigned i = 0; i < 256; i++)
        for (unsigned j = 0; j < hist[i]; j++, index++)
            if (index >= good / 4 && index < good * 3 / 4)
            {
                sum += i;
                count++;
            }
    if (!count)
        return out;
    out.luminance = (float)sum / count;
    if (bright > total / 4 || dark >= total * 9 / 10 || out.luminance <= 0)
        return out;
    out.metric = log2f(out.luminance / (exposure * again * dgain));
    out.valid = isfinite(out.metric);
    return out;
}

hb_light_reading hb_light_measure(const uint16_t *rgb, unsigned w, unsigned h, float exposure,
                                  float again, float dgain, bool metadata)
{
    if (!rgb || w < 32 || h < 32 || w > 4096 || h > 4096)
        return (hb_light_reading){0};
    unsigned hist[256] = {0};
    // 32x24 evenly spaced points in the central 3/4 region: fixed work and no heap.
    for (unsigned y = 0; y < 24; y++)
        for (unsigned x = 0; x < 32; x++)
        {
            unsigned px = w / 8 + x * (w * 3 / 4 - 1) / 31, py = h / 8 + y * (h * 3 / 4 - 1) / 23;
            uint16_t p = rgb[(size_t)py * w + px];
            unsigned r = ((p >> 11) & 31) * 255 / 31, g = ((p >> 5) & 63) * 255 / 63,
                     b = (p & 31) * 255 / 31;
            hist[(54 * r + 183 * g + 19 * b) / 256]++;
        }
    return hb_light_histogram(hist, exposure, again, dgain, metadata);
}
