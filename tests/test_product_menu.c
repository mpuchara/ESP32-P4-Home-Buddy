// SPDX-License-Identifier: Apache-2.0
#include "product_menu.h"
#include "screen_lifecycle.h"
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
LV_FONT_DECLARE(hb_font_20);
static lv_color_t pixels[800 * 480];
static unsigned clicked;
static void action(lv_event_t *e)
{
    clicked = (unsigned)(uintptr_t)lv_event_get_user_data(e);
}
static void flush(lv_disp_drv_t *d, const lv_area_t *a, lv_color_t *p)
{
    for (int y = a->y1; y <= a->y2; ++y)
        for (int x = a->x1; x <= a->x2; ++x)
            pixels[y * 800 + x] = *p++;
    lv_disp_flush_ready(d);
}
static void bounds(lv_obj_t *parent, unsigned count)
{
    lv_obj_update_layout(parent);
    CHECK(lv_obj_get_child_cnt(parent) == (count == 4 ? 7 : count));
    for (unsigned i = 0; i < count; ++i)
    {
        lv_obj_t *b = lv_obj_get_child(parent, i);
        lv_area_t a, text;
        lv_obj_get_coords(b, &a);
        lv_obj_get_coords(lv_obj_get_child(b, 0), &text);
        CHECK(a.x1 >= 0 && a.x2 < 800 && a.y1 >= 0 && a.y2 < 480);
        CHECK(lv_obj_get_height(b) >= 48);
        CHECK(text.x1 >= a.x1 && text.x2 <= a.x2 && text.y1 >= a.y1 && text.y2 <= a.y2);
        lv_event_send(b, LV_EVENT_CLICKED, NULL);
        CHECK(clicked == i + 1);
    }
}
int main(void)
{
    lv_init();
    static lv_color_t draw[800 * 20];
    static lv_disp_draw_buf_t buffer;
    lv_disp_draw_buf_init(&buffer, draw, NULL, 800 * 20);
    lv_disp_drv_t d;
    lv_disp_drv_init(&d);
    d.hor_res = 800;
    d.ver_res = 480;
    d.draw_buf = &buffer;
    d.flush_cb = flush;
    CHECK(lv_disp_drv_register(&d));
    lv_obj_t *saver = lv_scr_act();
    const hb_menu_item cards[] = {{"Domownicy", action, (void *)1},
                                  {"Rozpoznawanie", action, (void *)2},
                                  {"Ustawienia", action, (void *)3},
                                  {"Informacje", action, (void *)4}};
    const hb_menu_item rows[] = {{"Sieć Wi-Fi", action, (void *)1},
                                 {"Obraz z kamery UBox", action, (void *)2},
                                 {"Pogoda", action, (void *)3},
                                 {"Wyświetlacz", action, (void *)4},
                                 {"Home Assistant", action, (void *)5}};
    const hb_menu_item diagnostics[] = {
        {"Test obrazu", action, (void *)1},    {"Test powitania", action, (void *)2},
        {"Test dotyku", action, (void *)3},    {"Kamera lokalna", action, (void *)4},
        {"Obecność w tle", action, (void *)5}, {"Jakość twarzy", action, (void *)6},
        {"System", action, (void *)7},         {"Ocena rozpoznawania", action, (void *)8},
        {"Mikrofon", action, (void *)9},       {"Połączenie HA", action, (void *)10}};
    lv_mem_monitor_t before, after;
    for (unsigned i = 0; i < 400; ++i)
    {
        lv_obj_t *p = lv_obj_create(NULL);
        lv_obj_set_style_text_font(p, &hb_font_20, 0);
        lv_obj_set_style_bg_color(p, lv_color_hex(0x10171c), 0);
        lv_obj_set_style_text_color(p, lv_color_white(), 0);
        lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
        if (i % 3 == 1)
            hb_product_rows(p, rows, 5);
        else if (i % 3 == 2)
            hb_product_rows(p, diagnostics, 10);
        else
            hb_product_home(p, cards, action);
        lv_scr_load(p);
        bounds(p, i % 3 == 1 ? 5 : i % 3 == 2 ? 10 : 4);
        if (!(i % 3))
        {
            lv_obj_t *back = lv_obj_get_child(p, 4);
            CHECK(lv_obj_get_height(back) >= 48);
            lv_event_send(back, LV_EVENT_CLICKED, NULL);
            CHECK(clicked == 0);
        }
        lv_refr_now(NULL);
        if (i < 3)
        {
            FILE *f = fopen(i == 0   ? "product-menu.ppm"
                            : i == 1 ? "product-settings.ppm"
                                     : "product-diagnostics.ppm",
                            "wb");
            CHECK(f);
            fprintf(f, "P6\n800 480\n255\n");
            for (unsigned j = 0; j < 800 * 480; ++j)
            {
                lv_color32_t c;
                c.full = lv_color_to32(pixels[j]);
                fputc(c.ch.red, f);
                fputc(c.ch.green, f);
                fputc(c.ch.blue, f);
            }
            fclose(f);
        }
        hb_screen_delete(p, saver);
        CHECK(lv_scr_act() == saver);
        if (i == 1)
            lv_mem_monitor(&before);
    }
    lv_mem_monitor(&after);
    CHECK(after.free_size >= before.free_size);
    CHECK(!hb_product_timeout(29999, 0, false, false, false, false));
    CHECK(hb_product_timeout(30000, 0, false, false, false, false));
    CHECK(hb_product_timeout(20000, UINT32_MAX - 10000, false, false, false, false));
    CHECK(!hb_product_timeout(60000, 0, true, false, false, false));
    CHECK(!hb_product_timeout(60000, 0, false, true, false, false));
    CHECK(!hb_product_timeout(60000, 0, false, false, true, false));
    CHECK(!hb_product_timeout(60000, 0, false, false, false, true));
    puts("Product LVGL: 400 transitions, real Polish font, touch bounds, callbacks, pool and "
         "timeout passed");
    return 0;
}
