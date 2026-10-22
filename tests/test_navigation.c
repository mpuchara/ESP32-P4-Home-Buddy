// SPDX-License-Identifier: Apache-2.0
#include "lvgl.h"
#include "screen_lifecycle.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(expr)                                                                                \
    do                                                                                             \
    {                                                                                              \
        if (!(expr))                                                                               \
        {                                                                                          \
            fprintf(stderr, "line %d: %s\n", __LINE__, #expr);                                     \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static void flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *data)
{
    (void)area;
    (void)data;
    lv_disp_flush_ready(driver);
}
int main(void)
{
    lv_init();
    static lv_color_t pixels[80 * 48];
    static lv_disp_draw_buf_t buffer;
    lv_disp_draw_buf_init(&buffer, pixels, NULL, 80 * 48);
    lv_disp_drv_t driver;
    lv_disp_drv_init(&driver);
    driver.hor_res = 80;
    driver.ver_res = 48;
    driver.draw_buf = &buffer;
    driver.flush_cb = flush;
    CHECK(lv_disp_drv_register(&driver));
    lv_obj_t *saver = lv_scr_act();
    lv_mem_monitor_t before, after;
    lv_mem_monitor(&before);
    for (unsigned i = 0; i < 100; ++i)
    {
        lv_obj_t *pin = lv_obj_create(NULL);
        lv_obj_t *input = lv_textarea_create(pin);
        lv_obj_t *keyboard = lv_keyboard_create(pin);
        lv_keyboard_set_textarea(keyboard, input);
        lv_textarea_set_text(input, "123456");
        lv_scr_load(pin);
        hb_screen_delete(pin, saver);
        CHECK(lv_scr_act() == saver);
        lv_obj_t *form = lv_obj_create(NULL);
        lv_label_set_text(lv_label_create(form), "SSID");
        lv_scr_load(form);
        lv_tick_inc(30);
        lv_timer_handler();
        hb_screen_delete(form, saver);
        CHECK(lv_scr_act() == saver);
        if (i == 0)
            lv_mem_monitor(&before); // Account for the first LVGL theme/draw cache allocation.
    }
    lv_mem_monitor(&after);
    CHECK(after.free_size >= before.free_size);
    puts("navigation: 100 real LVGL PIN/form/return transitions; no active-screen gap or pool "
         "growth");
    return 0;
}
