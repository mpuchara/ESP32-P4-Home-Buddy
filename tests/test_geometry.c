// SPDX-License-Identifier: Apache-2.0
#include "hb_geometry.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(expr)                                                                                \
    do                                                                                             \
    {                                                                                              \
        if (!(expr))                                                                               \
        {                                                                                          \
            fprintf(stderr, "failed line %d: %s\n", __LINE__, #expr);                              \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
int main(void)
{
    hb_cover_t c;
    CHECK(hb_cover(1920, 1080, 800, 480, &c));
    CHECK(c.width == 854 && c.height == 480 && c.crop_x == 27 && c.crop_y == 0);
    CHECK(hb_cover(480, 800, 800, 480, &c));
    CHECK(c.width == 800 && c.height == 1334 && c.crop_y == 427);
    CHECK(hb_cover(800, 480, 800, 480, &c));
    CHECK(c.width == 800 && c.height == 480 && !c.crop_x && !c.crop_y);
    CHECK(!hb_cover(0, 480, 800, 480, &c));
    CHECK(!hb_cover(800, 0, 800, 480, &c));
    CHECK(!hb_cover(800, 480, 0, 480, &c));
    CHECK(!hb_cover(800, 480, 800, 0, &c));
    CHECK(!hb_cover(800, 480, 800, 480, NULL));
    CHECK(!hb_cover(UINT32_MAX, 1, 800, UINT32_MAX, &c));
    CHECK(hb_cover(UINT32_MAX, UINT32_MAX, 800, 480, &c));
    CHECK(c.width == 800 && c.height == 800);
    for (uint32_t w = 1; w < 4000; w += 37)
    {
        for (uint32_t h = 1; h < 2200; h += 29)
        {
            CHECK(hb_cover(w, h, 800, 480, &c));
            CHECK(c.width >= 800 && c.height >= 480);
            CHECK(c.width == 800 || c.height == 480);
            CHECK(c.crop_x * 2 <= c.width - 800 && c.width - 800 - c.crop_x * 2 <= 1);
            CHECK(c.crop_y * 2 <= c.height - 480 && c.height - 480 - c.crop_y * 2 <= 1);
        }
    }
    int x, y;
    CHECK(hb_touch_landscape(0, 0, &x, &y) && x == 0 && y == 479);
    CHECK(hb_touch_landscape(479, 799, &x, &y) && x == 799 && y == 0);
    CHECK(!hb_touch_landscape(-1, 0, &x, &y));
    CHECK(!hb_touch_landscape(480, 0, &x, &y));
    CHECK(!hb_touch_landscape(0, 800, &x, &y));
    CHECK(!hb_touch_landscape(0, -1, &x, &y));
    CHECK(!hb_touch_landscape(0, 0, NULL, &y));
    puts("geometry: aspect cover, bounds, overflow and touch corners passed");
    return 0;
}
