// SPDX-License-Identifier: Apache-2.0
#include "hb_raw_frame.h"
#include <stddef.h>
static unsigned linear_channel(unsigned value)
{
    // Selected sensor table sets black target64 in RAW10; ISP RAW8 retains top8 bits.
    return value > 16 ? (value - 16) * 255 / 239 : 0;
}
static uint16_t pixel(const uint8_t *data, unsigned w, unsigned x, unsigned y)
{
    const uint8_t *top = data + (size_t)(y & ~1U) * w + (x & ~1U);
    unsigned r = linear_channel(top[w]),
             g = (linear_channel(top[0]) + linear_channel(top[w + 1])) / 2,
             b = linear_channel(top[1]);
    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}
bool hb_raw_thumbnail(const uint8_t *src, unsigned w, unsigned h, uint16_t *dst, unsigned ow,
                      unsigned oh)
{
    if (!src || !dst || w < 32 || h < 32 || w > 4096 || h > 4096 || (w & 1) || (h & 1) || !ow ||
        !oh || ow > 800 || oh > 480)
        return false;
    for (unsigned y = 0; y < oh; y++)
        for (unsigned x = 0; x < ow; x++)
            dst[(size_t)y * ow + x] = pixel(src, w, x * (w - 2) / ow, y * (h - 2) / oh);
    return true;
}
bool hb_raw_thumbnail_upright(const uint8_t *src, unsigned w, unsigned h, uint16_t *dst,
                              unsigned ow, unsigned oh)
{
    if (!src || !dst || w < 32 || h < 32 || w > 4096 || h > 4096 || (w & 1) || (h & 1) || !ow ||
        !oh || ow > 800 || oh > 480)
        return false;
    for (unsigned y = 0; y < oh; y++)
        for (unsigned x = 0; x < ow; x++)
            dst[(size_t)y * ow + x] = pixel(src, w, w - 2 - y * (w - 2) / oh, x * (h - 2) / ow);
    return true;
}
hb_light_reading hb_raw_light(const uint8_t *src, unsigned w, unsigned h, float exposure, float ag,
                              float dg, bool meta)
{
    if (!src || w < 32 || h < 32 || w > 4096 || h > 4096 || (w & 1) || (h & 1))
        return (hb_light_reading){0};
    unsigned hist[256] = {0};
    // Measure unquantized linear channels, before RGB565, display white balance and gamma.
    for (unsigned y = 0; y < 24; y++)
        for (unsigned x = 0; x < 32; x++)
        {
            unsigned px = (w / 8 + x * (w * 3 / 4 - 2) / 31) & ~1U;
            unsigned py = (h / 8 + y * (h * 3 / 4 - 2) / 23) & ~1U;
            const uint8_t *top = src + (size_t)py * w + px;
            unsigned r = linear_channel(top[w]),
                     g = (linear_channel(top[0]) + linear_channel(top[w + 1])) / 2,
                     b = linear_channel(top[1]);
            hist[(54 * r + 183 * g + 19 * b) / 256]++;
        }
    return hb_light_histogram(hist, exposure, ag, dg, meta);
}
