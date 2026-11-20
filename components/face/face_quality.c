// SPDX-License-Identifier: Apache-2.0
#include "hb_face_quality.h"
#include <math.h>
static unsigned luma(uint16_t p)
{
    unsigned r = (((p >> 11) & 31) * 255 + 15) / 31;
    unsigned g = (((p >> 5) & 63) * 255 + 31) / 63;
    unsigned b = ((p & 31) * 255 + 15) / 31;
    return (54 * r + 183 * g + 19 * b + 128) / 256;
}
static unsigned difference(unsigned a, unsigned b)
{
    return a > b ? a - b : b - a;
}
hb_face_quality hb_face_quality_measure(const uint16_t *p, size_t capacity, unsigned w, unsigned h,
                                        int x1, int y1, int x2, int y2)
{
    hb_face_quality q = {0};
    if (!p || w < 3 || h < 3 || w > 800 || h > 480 || capacity < (size_t)w * h * 2)
        return q;
    if (x1 <= 0 || y1 <= 0 || x2 >= (int)w - 1 || y2 >= (int)h - 1)
        q.flags |= HB_FQ_EDGE;
    if (x1 < 0)
        x1 = 0;
    if (y1 < 0)
        y1 = 0;
    if (x2 >= (int)w)
        x2 = (int)w - 1;
    if (y2 >= (int)h)
        y2 = (int)h - 1;
    if (x1 >= x2 || y1 >= y2)
        return (hb_face_quality){0};
    uint64_t sum = 0, squares = 0, gradients = 0;
    unsigned count = 0, edges = 0, near_limit = 0;
    for (int y = y1; y <= y2; y++)
        for (int x = x1; x <= x2; x++)
        {
            unsigned v = luma(p[(size_t)y * w + x]);
            sum += v;
            squares += v * v;
            count++;
            if (v <= 8 || v >= 247)
                near_limit++;
            if (x < x2)
            {
                gradients += difference(v, luma(p[(size_t)y * w + x + 1]));
                edges++;
            }
            if (y < y2)
            {
                gradients += difference(v, luma(p[(size_t)(y + 1) * w + x]));
                edges++;
            }
        }
    q.valid = true;
    q.width = (uint16_t)(x2 - x1 + 1);
    q.height = (uint16_t)(y2 - y1 + 1);
    q.mean = (float)sum / count;
    // Exact integer numerator avoids cancellation on large, constant ROIs.
    float variance = (float)(squares * count - sum * sum) / ((float)count * count);
    q.contrast = sqrtf(fmaxf(0, variance));
    q.detail = edges ? (float)gradients / edges : 0;
    q.near_limit_percent = 100.0f * near_limit / count;
    if (q.width < 48 || q.height < 48)
        q.flags |= HB_FQ_SMALL;
    if (q.mean < 45)
        q.flags |= HB_FQ_DARK;
    if (q.mean > 220)
        q.flags |= HB_FQ_BRIGHT;
    if (q.contrast < 12)
        q.flags |= HB_FQ_LOW_CONTRAST;
    if (q.detail < 3)
        q.flags |= HB_FQ_LOW_DETAIL;
    return q;
}
