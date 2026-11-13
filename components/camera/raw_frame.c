// SPDX-License-Identifier: Apache-2.0
#include "hb_raw_frame.h"
#include <math.h>
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

bool hb_rgb_thumbnail_upright(const uint8_t *src, unsigned w, unsigned h, uint16_t *dst,
                              unsigned ow, unsigned oh)
{
    if (!src || !dst || w < 32 || h < 32 || w > 4096 || h > 4096 || !ow || !oh || ow > 800 ||
        oh > 800)
        return false;
    const uint16_t *rgb = (const uint16_t *)src;
    for (unsigned y = 0; y < oh; ++y)
        for (unsigned x = 0; x < ow; ++x)
            dst[(size_t)y * ow + x] =
                rgb[(size_t)(x * (h - 2) / ow) * w + w - 2 - y * (w - 2) / oh];
    return true;
}

uint32_t hb_isp_gamma_gain(uint32_t x, float gain)
{
    // OV02C10 mode has RAW10 black target 64, i.e. 16 in the ISP 8-bit gamma domain.
    // CCM rows sum to one, so a neutral black pedestal is unchanged by that stage.
    if (x <= 16)
        return 0;
    if (x >= 255)
        return 255;
    float linear = (x - 16) / 239.0f;
    // A soft highlight shoulder preserves the white endpoint without hard clipping.
    linear = gain * linear / (1 + (gain - 1) * linear);
    return (uint32_t)lroundf(255 * powf(linear, 0.518f));
}

uint32_t hb_isp_gamma_black_corrected(uint32_t x)
{
    return hb_isp_gamma_gain(x, 1.0f);
}

uint32_t hb_isp_gamma_red(uint32_t x)
{
    return hb_isp_gamma_gain(x, 1.65f);
}

uint32_t hb_isp_gamma_blue(uint32_t x)
{
    return hb_isp_gamma_gain(x, 1.55f);
}


static bool rgb_bounds(const uint8_t *src, unsigned w, unsigned h, unsigned x, unsigned y,
                       unsigned cw, unsigned ch)
{
    return src && w >= 32 && h >= 32 && w <= 4096 && h <= 4096 && cw && ch && x < h && y < w &&
           cw <= h - x && ch <= w - y;
}
static const uint8_t *upright(const uint8_t *src, unsigned w, unsigned x, unsigned y)
{
    return src + ((size_t)x * w + w - 1 - y) * 3;
}
static void sample_rgb(const uint8_t *src, unsigned w, unsigned h, unsigned x, unsigned y,
                       unsigned ow, unsigned oh, uint8_t rgb[3])
{
    // A small box filter avoids aliasing on the detector and display downsample.
    unsigned sx = x * (h - 2) / ow, sy = y * (w - 2) / oh;
    const uint8_t *a = upright(src, w, sx, sy);
    const uint8_t *b = upright(src, w, sx + 1, sy);
    const uint8_t *c = upright(src, w, sx, sy + 1);
    const uint8_t *d = upright(src, w, sx + 1, sy + 1);
    for (unsigned i = 0; i < 3; ++i)
        rgb[i] = (a[2 - i] + b[2 - i] + c[2 - i] + d[2 - i] + 2) / 4;
}
bool hb_rgb888_thumbnail(const uint8_t *src, unsigned w, unsigned h, uint8_t *dst, unsigned ow,
                         unsigned oh)
{
    if (!dst || !rgb_bounds(src, w, h, 0, 0, h, w) || !ow || !oh || ow > 800 || oh > 800)
        return false;
    for (unsigned y = 0; y < oh; ++y)
        for (unsigned x = 0; x < ow; ++x)
            sample_rgb(src, w, h, x, y, ow, oh, dst + ((size_t)y * ow + x) * 3);
    return true;
}
bool hb_rgb888_preview(const uint8_t *src, unsigned w, unsigned h, uint16_t *dst, unsigned ow,
                       unsigned oh)
{
    if (!dst || !rgb_bounds(src, w, h, 0, 0, h, w) || !ow || !oh || ow > 800 || oh > 800)
        return false;
    for (unsigned y = 0; y < oh; ++y)
        for (unsigned x = 0; x < ow; ++x)
        {
            uint8_t rgb[3];
            sample_rgb(src, w, h, x, y, ow, oh, rgb);
            dst[(size_t)y * ow + x] = ((rgb[0] >> 3) << 11) | ((rgb[1] >> 2) << 5) | (rgb[2] >> 3);
        }
    return true;
}
bool hb_rgb888_crop(const uint8_t *src, unsigned w, unsigned h, unsigned x, unsigned y,
                    unsigned cw, unsigned ch, uint8_t *dst, size_t capacity)
{
    if (!dst || !rgb_bounds(src, w, h, x, y, cw, ch) || capacity < (size_t)cw * ch * 3)
        return false;
    for (unsigned j = 0; j < ch; ++j)
        for (unsigned i = 0; i < cw; ++i)
        {
            const uint8_t *p = upright(src, w, x + i, y + j);
            uint8_t *d = dst + ((size_t)j * cw + i) * 3;
            d[0] = p[2];
            d[1] = p[1];
            d[2] = p[0];
        }
    return true;
}
static unsigned rgb_luma(const uint8_t *p)
{
    return (54 * p[2] + 183 * p[1] + 19 * p[0] + 128) / 256;
}
hb_rgb888_stats hb_rgb888_quality(const uint8_t *src, unsigned w, unsigned h, unsigned x,
                                  unsigned y, unsigned cw, unsigned ch)
{
    hb_rgb888_stats q = {0};
    if (!rgb_bounds(src, w, h, x, y, cw, ch) || cw < 8 || ch < 8)
        return q;
    unsigned sum = 0, sq = 0, edges = 0, clipped = 0;
    for (unsigned j = 0; j < 24; ++j)
        for (unsigned i = 0; i < 32; ++i)
        {
            unsigned px = x + i * (cw - 3) / 31, py = y + j * (ch - 3) / 23;
            unsigned a = rgb_luma(upright(src, w, px, py));
            unsigned b = rgb_luma(upright(src, w, px + 1, py));
            unsigned c = rgb_luma(upright(src, w, px, py + 1));
            unsigned d = rgb_luma(upright(src, w, px + 1, py + 1));
            unsigned v = (a + b + c + d + 2) / 4;
            sum += v;
            sq += v * v;
            clipped += v <= 8 || v >= 247;
            // Two-pixel averaged gradients suppress isolated noise spikes.
            int dx = (int)(a + c) - (int)(b + d);
            int dy = (int)(a + b) - (int)(c + d);
            edges += (unsigned)(dx < 0 ? -dx : dx) + (unsigned)(dy < 0 ? -dy : dy);
        }
    q.mean = sum / 768.0f;
    q.contrast = sqrtf(fmaxf(0, sq / 768.0f - q.mean * q.mean));
    q.detail = edges / 3072.0f;
    q.clipped = clipped / 768.0f;
    q.width = cw;
    q.height = ch;
    q.valid = true;
    return q;
}
float hb_rgb888_rank(hb_rgb888_stats q)
{
    if (!q.valid)
        return -1;
    float size = fminf(q.width, q.height);
    float exposure = fmaxf(0, 45 - q.mean) + fmaxf(0, q.mean - 220);
    return fminf(size, 240) * 0.04f + fminf(q.contrast, 45) * 0.15f +
           fminf(q.detail, 20) - exposure * 0.3f - q.clipped * 25;
}
bool hb_rgb888_neutral(const uint8_t *src, unsigned w, unsigned h, float *red, float *blue)
{
    if (!red || !blue || !rgb_bounds(src, w, h, 0, 0, h, w))
        return false;
    unsigned r = 0, g = 0, b = 0, count = 0;
    for (unsigned y = 0; y < 24; ++y)
        for (unsigned x = 0; x < 32; ++x)
        {
            const uint8_t *p = src + ((size_t)(h / 8 + y * (h * 3 / 4 - 1) / 23) * w +
                                      w / 8 + x * (w * 3 / 4 - 1) / 31) *
                                         3;
            unsigned v = p[1];
            if (v < 85 || v > 210 || p[2] * 5 < v * 4 || p[2] * 4 > v * 5 ||
                p[0] * 5 < v * 4 || p[0] * 4 > v * 5)
                continue;
            r += p[2];
            g += v;
            b += p[0];
            ++count;
        }
    if (count < 80 || !r || !b)
        return false;
    *red = (float)g / r;
    *blue = (float)g / b;
    return true;
}
void hb_camera_exposure_plan(float line_us, unsigned max_lines, unsigned old_lines,
                             unsigned old_gain, float luma, bool face, unsigned *lines,
                             unsigned *gain)
{
    if (!lines || !gain || !isfinite(line_us) || line_us <= 0 || max_lines < 4)
        return;
    // Feedback is measured after gamma. Bound the correction to avoid visible pumping.
    float ratio = fminf(1.5f, fmaxf(0.67f, 125 / (luma + 1)));
    if (luma >= 100 && luma <= 155)
        ratio = 1;
    float total = old_lines * (old_gain / 256.0f) * ratio;
    unsigned period = (unsigned)fmaxf(1, lroundf(10000 / line_us));
    unsigned preferred = face ? period : period * 2;
    preferred = preferred < max_lines ? preferred : max_lines;
    float chosen = fminf(total, preferred);
    if (total > chosen * 8)
        chosen = fminf(max_lines, total / 8 + period);
    unsigned n = (unsigned)fmaxf(4, fminf(max_lines, chosen));
    // 50 Hz mains: multiples of 10 ms mitigate 100 Hz brightness ripple.
    if (n >= period && period >= 4)
        n = n / period * period;
    *lines = n;
    *gain = (unsigned)fminf(2048, fmaxf(256, lroundf(total * 256 / n))) & ~1U;
}
