// SPDX-License-Identifier: Apache-2.0
#include "hb_camera_preview.h"
#include <math.h>
#include <stddef.h>
static unsigned display_channel(float linear, unsigned maximum)
{
    linear = fminf(1, fmaxf(0, linear));
    float value = linear <= 0.0031308f ? 12.92f * linear : 1.055f * powf(linear, 1 / 2.4f) - 0.055f;
    return (unsigned)lroundf(value * maximum);
}
bool hb_camera_make_preview(const uint16_t *src, unsigned w, unsigned h, uint16_t *dst, unsigned ow,
                            unsigned oh)
{
    if (!src || !dst || w < 32 || h < 32 || w > 4096 || h > 4096 || !ow || !oh || ow > 800 ||
        oh > 480)
        return false;
    float rsum = 0, gsum = 0, bsum = 0;
    unsigned count = 0;
    // Gray-world white balance from bounded ROI samples; reject black/clipped colors.
    for (unsigned y = 0; y < 12; y++)
        for (unsigned x = 0; x < 16; x++)
        {
            uint16_t p = src[(size_t)(h / 8 + y * (h * 3 / 4 - 1) / 11) * w + w / 8 +
                             x * (w * 3 / 4 - 1) / 15];
            unsigned r = (p >> 11) & 31, g = (p >> 5) & 63, b = p & 31;
            if (r < 2 || r > 29 || g < 4 || g > 59 || b < 2 || b > 29)
                continue;
            rsum += r / 31.0f;
            gsum += g / 63.0f;
            bsum += b / 31.0f;
            count++;
        }
    float rgain = 1, bgain = 1;
    if (count >= 24 && rsum > 0 && bsum > 0)
    {
        rgain = fminf(3, fmaxf(0.5f, gsum / rsum));
        bgain = fminf(3, fmaxf(0.5f, gsum / bsum));
    }
    uint16_t red[32], green[64], blue[32];
    for (unsigned i = 0; i < 32; i++)
    {
        red[i] = (uint16_t)(display_channel(i * rgain / 31, 31) << 11);
        blue[i] = (uint16_t)display_channel(i * bgain / 31, 31);
    }
    for (unsigned i = 0; i < 64; i++)
        green[i] = (uint16_t)(display_channel(i / 63.0f, 63) << 5);
    for (unsigned y = 0; y < oh; y++)
    {
        const uint16_t *row = src + (size_t)(y * h / oh) * w;
        for (unsigned x = 0; x < ow; x++)
        {
            uint16_t p = row[x * w / ow];
            dst[(size_t)y * ow + x] = red[(p >> 11) & 31] | green[(p >> 5) & 63] | blue[p & 31];
        }
    }
    return true;
}
