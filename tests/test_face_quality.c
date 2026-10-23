// SPDX-License-Identifier: Apache-2.0
#include "hb_face_quality.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "line %d: %s\n", __LINE__, #x);                                        \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static uint16_t gray(unsigned v)
{
    return (uint16_t)(((v * 31 / 255) << 11) | ((v * 63 / 255) << 5) | (v * 31 / 255));
}
int main(void)
{
    uint16_t pixels[64 * 64] = {0};
    hb_face_quality q = hb_face_quality_measure(pixels, sizeof(pixels), 64, 64, 8, 8, 55, 55);
    CHECK(q.valid && q.width == 48 && q.height == 48);
    CHECK(q.mean == 0 && q.contrast == 0 && q.detail == 0 && q.near_limit_percent == 100);
    CHECK((q.flags & HB_FQ_DARK) && !(q.flags & HB_FQ_SMALL) && !(q.flags & HB_FQ_EDGE));
    for (unsigned i = 0; i < 64 * 64; i++)
        pixels[i] = 0xffff;
    q = hb_face_quality_measure(pixels, sizeof(pixels), 64, 64, -8, -8, 99, 99);
    CHECK(q.valid && q.width == 64 && q.height == 64 && (q.flags & HB_FQ_EDGE));
    CHECK(q.mean == 255 && q.contrast == 0 && q.detail == 0 && (q.flags & HB_FQ_BRIGHT));
    for (unsigned y = 0; y < 64; y++)
        for (unsigned x = 0; x < 64; x++)
            pixels[y * 64 + x] = ((x + y) & 1) ? 0xffff : 0;
    q = hb_face_quality_measure(pixels, sizeof(pixels), 64, 64, 8, 8, 55, 55);
    CHECK(fabsf(q.mean - 127.5f) < 0.01f && fabsf(q.contrast - 127.5f) < 0.01f);
    CHECK(q.detail == 255 && q.near_limit_percent == 100);
    float high_detail = q.detail;
    for (unsigned y = 0; y < 64; y++)
        for (unsigned x = 0; x < 64; x++)
            pixels[y * 64 + x] = gray(((x + y) & 1) ? 142 : 113);
    q = hb_face_quality_measure(pixels, sizeof(pixels), 64, 64, 8, 8, 55, 55);
    CHECK(q.detail > 0 && q.detail < high_detail / 2 && q.contrast < 30);
    // Only pixels and neighbour pairs inside the ROI may contribute.
    for (unsigned i = 0; i < 64 * 64; i++)
        pixels[i] = 0xffff;
    for (unsigned y = 8; y <= 55; y++)
        for (unsigned x = 8; x <= 55; x++)
            pixels[y * 64 + x] = 0;
    q = hb_face_quality_measure(pixels, sizeof(pixels), 64, 64, 8, 8, 55, 55);
    CHECK(q.mean == 0 && q.detail == 0);
    q = hb_face_quality_measure(pixels, sizeof(pixels), 64, 64, 8, 8, 54, 55);
    CHECK(q.flags & HB_FQ_SMALL);
    CHECK(!hb_face_quality_measure(NULL, 8192, 64, 64, 1, 1, 9, 9).valid);
    CHECK(!hb_face_quality_measure(pixels, sizeof(pixels) - 1, 64, 64, 1, 1, 9, 9).valid);
    CHECK(!hb_face_quality_measure(pixels, sizeof(pixels), 801, 64, 1, 1, 9, 9).valid);
    CHECK(!hb_face_quality_measure(pixels, sizeof(pixels), 64, 481, 1, 1, 9, 9).valid);
    CHECK(!hb_face_quality_measure(pixels, sizeof(pixels), 2, 64, 1, 1, 9, 9).valid);
    CHECK(!hb_face_quality_measure(pixels, sizeof(pixels), 64, 64, 9, 1, 1, 9).valid);
    CHECK(!hb_face_quality_measure(pixels, sizeof(pixels), 64, 64, INT_MAX, 1, INT_MAX, 9).valid);
    CHECK(!hb_face_quality_measure(pixels, sizeof(pixels), 64, 64, 1, 1, INT_MIN, 9).valid);
    // Maximum supported ROI exceeds uint32 sum-of-squares; accumulator must not wrap.
    size_t bytes = 800 * 480 * 2;
    uint16_t *large = malloc(bytes + 2);
    CHECK(large);
    for (unsigned i = 0; i < 800 * 480; i++)
        large[i] = 0xffff;
    large[800 * 480] = 0x5a5a;
    q = hb_face_quality_measure(large, bytes, 800, 480, INT_MIN, INT_MIN, INT_MAX, INT_MAX);
    CHECK(q.valid && q.mean == 255 && q.contrast == 0 && q.detail == 0 &&
          q.near_limit_percent == 100);
    CHECK(large[800 * 480] == 0x5a5a);
    free(large);
    puts("face quality: ROI bounds, brightness/contrast/detail, smoothing and accumulator limits "
         "OK");
    return 0;
}
