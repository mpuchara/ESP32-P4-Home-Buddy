// SPDX-License-Identifier: Apache-2.0
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>

LV_FONT_DECLARE(hb_font_20);
#if LVGL_VERSION_MAJOR != 8 || LVGL_VERSION_MINOR != 3 || LVGL_VERSION_PATCH != 11
#error "Font regression requires the pinned LVGL 8.3.11 decoder"
#endif
#define CHECK(expr)                                                                                \
    do                                                                                             \
    {                                                                                              \
        if (!(expr))                                                                               \
        {                                                                                          \
            fprintf(stderr, "failed line %d: %s\n", __LINE__, #expr);                              \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

static void check_glyph(uint32_t codepoint)
{
    lv_font_glyph_dsc_t glyph = {0};
    CHECK(lv_font_get_glyph_dsc(&hb_font_20, &glyph, codepoint, 0));
    CHECK(!glyph.is_placeholder);
    CHECK(glyph.box_w > 0 && glyph.box_h > 0 && glyph.bpp == 4);
    const uint8_t *bitmap = lv_font_get_glyph_bitmap(glyph.resolved_font, codepoint);
    CHECK(bitmap != NULL);
    size_t bytes = ((size_t)glyph.box_w * glyph.box_h * glyph.bpp + 7) / 8;
    bool ink = false;
    for (size_t i = 0; i < bytes; ++i)
        ink |= bitmap[i] != 0;
    CHECK(ink);
}

int main(void)
{
    CHECK(LV_USE_FONT_COMPRESSED == 0);
    for (uint32_t cp = 33; cp <= 126; ++cp)
        check_glyph(cp);
    const uint32_t polish[] = {0x104, 0x105, 0x106, 0x107, 0x118, 0x119, 0x141, 0x142, 0x143, 0x144,
                               0xd3,  0xf3,  0x15a, 0x15b, 0x179, 0x17a, 0x17b, 0x17c, 0xb0};
    for (size_t i = 0; i < sizeof(polish) / sizeof(polish[0]); ++i)
        check_glyph(polish[i]);
    const uint32_t controls[] = {0x2013, 0x2014, 0x2022, 0x2026, 0x2212, 0xf00c,
                                 0xf053, 0xf054, 0xf078, 0xf11c, 0xf55a, 0xf8a2};
    for (size_t i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i)
        check_glyph(controls[i]);
    puts("font: real LVGL descriptors and visible bitmap data passed for ASCII and Polish");
    return 0;
}
