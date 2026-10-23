// SPDX-License-Identifier: Apache-2.0
#include "hb_face.h"
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
int main(void)
{
    CHECK(hb_face_admit(16 * 1024 * 1024, 8 * 1024 * 1024, 192 * 1024, 128 * 1024, true));
    CHECK(!hb_face_admit(16 * 1024 * 1024 - 1, 8 * 1024 * 1024, 192 * 1024, 128 * 1024, true));
    CHECK(!hb_face_admit(16 * 1024 * 1024, 8 * 1024 * 1024 - 1, 192 * 1024, 128 * 1024, true));
    CHECK(!hb_face_admit(16 * 1024 * 1024, 8 * 1024 * 1024, 192 * 1024 - 1, 128 * 1024, true));
    CHECK(!hb_face_admit(16 * 1024 * 1024, 8 * 1024 * 1024, 192 * 1024, 128 * 1024 - 1, true));
    CHECK(hb_face_admit(12 * 1024 * 1024, 4 * 1024 * 1024, 64 * 1024, 32 * 1024, false));
    CHECK(!hb_face_admit(0, 0, 0, 0, false));
    hb_face_box b = {-5, -10, 200, 400, 0.8f};
    CHECK(hb_face_clip(&b));
    CHECK(b.x1 == 0 && b.y1 == 0 && b.x2 == 179 && b.y2 == 317);
    b = (hb_face_box){200, 200, 100, 300, 0.9f};
    CHECK(!hb_face_clip(&b));
    b = (hb_face_box){0, 0, 40, 40, NAN};
    CHECK(!hb_face_clip(&b));
    CHECK(!hb_face_clip(NULL));
    hb_face_status s = {
        .enabled = true, .ready = true, .error = ESP_OK, .frame_ms = 1000, .count = 4};
    CHECK(hb_face_fresh(&s, 1750));
    CHECK(!hb_face_fresh(&s, 1751));
    CHECK(!hb_face_fresh(&s, 999));
    s.count = 5;
    CHECK(!hb_face_fresh(&s, 1000));
    s.count = 0;
    s.enabled = false;
    CHECK(!hb_face_fresh(&s, 1000));
    s.enabled = true;
    s.error = ESP_FAIL;
    CHECK(!hb_face_fresh(&s, 1000));
    uint16_t *pixels = malloc(HB_FACE_INPUT_BYTES + 2);
    CHECK(pixels);
    for (unsigned y = 0; y < HB_FACE_HEIGHT; y++)
        for (unsigned x = 0; x < HB_FACE_WIDTH; x++)
            pixels[y * HB_FACE_WIDTH + x] = (uint16_t)(1 + y * HB_FACE_WIDTH + x);
    pixels[HB_FACE_INPUT_BYTES / 2] = 0x5a5a;
    CHECK(!hb_face_letterbox(pixels, HB_FACE_INPUT_BYTES - 1));
    CHECK(hb_face_letterbox(pixels, HB_FACE_INPUT_BYTES));
    for (unsigned y = 0; y < HB_FACE_HEIGHT; y++)
        for (unsigned x = 0; x < HB_FACE_INPUT_WIDTH; x++)
        {
            uint16_t expected = x < HB_FACE_PAD_X || x >= HB_FACE_PAD_X + HB_FACE_WIDTH
                                    ? 0
                                    : (uint16_t)(1 + y * HB_FACE_WIDTH + x - HB_FACE_PAD_X);
            CHECK(pixels[y * HB_FACE_INPUT_WIDTH + x] == expected);
        }
    CHECK(pixels[HB_FACE_INPUT_BYTES / 2] == 0x5a5a);
    free(pixels);
    puts("face policy: memory admission, bounded boxes and stale/disabled results OK");
    return 0;
}
