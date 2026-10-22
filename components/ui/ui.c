// SPDX-License-Identifier: Apache-2.0
#include "board_p4.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hb_geometry.h"
#include "hb_ui.h"
#include "hb_wifi.h"
#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

LV_FONT_DECLARE(hb_font_20);
static lv_disp_draw_buf_t draw;
static lv_disp_drv_t display;
static lv_indev_drv_t touch;
static lv_obj_t *saver, *menu, *status;
static bool in_menu;
static uint32_t last_touch;
static esp_timer_handle_t tick;
static lv_color_t *pixels;

static void flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *data)
{
    (void)area;
    board_p4_present_rotated((const uint16_t *)data, 800, 480);
    lv_disp_flush_ready(drv);
}
static void read_touch(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    int px = 0, py = 0, x = 0, y = 0;
    bool pressed = false;
    board_p4_touch_read(&px, &py, &pressed);
    if (pressed && hb_touch_landscape(px, py, &x, &y))
    {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
        last_touch = lv_tick_get();
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
static void tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(2);
}
static lv_obj_t *label(lv_obj_t *parent, const char *text, lv_align_t align, int x, int y)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), 0);
    lv_obj_align(obj, align, x, y);
    return obj;
}
static void open_menu(lv_event_t *e)
{
    (void)e;
    in_menu = true;
    last_touch = lv_tick_get();
    lv_scr_load(menu);
}
static void close_menu(lv_event_t *e)
{
    (void)e;
    in_menu = false;
    lv_scr_load(saver);
}
static void brightness(lv_event_t *e)
{
    // Bounded PWM update only. No flash writes in an LVGL callback.
    int duty = lv_slider_get_value(lv_event_get_target(e));
    board_p4_set_brightness((uint8_t)duty);
}
static void scan(lv_event_t *e)
{
    (void)e;
    bool queued = hb_wifi_request_scan();
    lv_label_set_text(status, queued ? "Skanowanie zlecone. Wynik w konsoli USB."
                                     : "Wi-Fi wyłączone lub zadanie zajęte.");
}
static void timeout(lv_timer_t *timer)
{
    (void)timer;
    if (in_menu && (uint32_t)(lv_tick_get() - last_touch) >= 30000)
        close_menu(NULL);
}
static void screens(void)
{
    saver = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(saver, lv_color_hex(0x10171c), 0);
    lv_obj_set_style_text_font(saver, &hb_font_20, 0);
    lv_obj_clear_flag(saver, LV_OBJ_FLAG_SCROLLABLE);
    // No fabricated snapshot, time or weather. This disappears when the real
    // image transport is implemented; image, clock and weather get separate objects.
    label(saver, "Brak zapisanej migawki UBox", LV_ALIGN_CENTER, 0, 0);
    label(saver, "--:--", LV_ALIGN_BOTTOM_LEFT, 24, -18);
    label(saver, "--°C", LV_ALIGN_BOTTOM_RIGHT, -24, -18);
    lv_obj_add_event_cb(saver, open_menu, LV_EVENT_CLICKED, NULL);

    menu = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(menu, lv_color_hex(0x10171c), 0);
    lv_obj_set_style_text_font(menu, &hb_font_20, 0);
    lv_obj_clear_flag(menu, LV_OBJ_FLAG_SCROLLABLE);
    label(menu, "Uruchomienie sprzętu", LV_ALIGN_TOP_LEFT, 22, 16);
    label(menu, "Profil eksperymentalny: JC4880P443C", LV_ALIGN_TOP_LEFT, 22, 49);
    static const char *names[] = {"Domownicy", "Rozpoznawanie", "Home Assistant", "Kamera UBox"};
    for (int i = 0; i < 4; ++i)
    {
        lv_obj_t *tile = lv_obj_create(menu);
        lv_obj_set_size(tile, 180, 70);
        lv_obj_set_pos(tile, 16 + i * 194, 94);
        lv_obj_set_style_bg_color(tile, lv_color_hex(0x23313b), 0);
        label(tile, names[i], LV_ALIGN_TOP_MID, 0, -3);
        label(tile, "W przygotowaniu", LV_ALIGN_BOTTOM_MID, 0, 5);
    }
    label(menu, "Podświetlenie", LV_ALIGN_TOP_LEFT, 24, 193);
    lv_obj_t *slider = lv_slider_create(menu);
    lv_obj_set_size(slider, 440, 22);
    lv_obj_set_pos(slider, 285, 206);
    lv_slider_set_range(slider, 26, 255);
    lv_slider_set_value(slider, 128, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, brightness, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *button = lv_btn_create(menu);
    lv_obj_set_size(button, 260, 55);
    lv_obj_set_pos(button, 24, 264);
    label(button, "Skanuj sieci Wi-Fi", LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(button, scan, LV_EVENT_CLICKED, NULL);
    status = label(menu, "Diagnostyka uruchomienia, bez zapisanej konfiguracji.", LV_ALIGN_TOP_LEFT,
                   24, 340);
    button = lv_btn_create(menu);
    lv_obj_set_size(button, 160, 52);
    lv_obj_align(button, LV_ALIGN_BOTTOM_RIGHT, -24, -18);
    label(button, "Powrót", LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(button, close_menu, LV_EVENT_CLICKED, NULL);
    lv_timer_create(timeout, 250, NULL);
    lv_scr_load(saver);
}
static void task(void *arg)
{
    (void)arg;
    for (;;)
    {
        uint32_t delay = lv_timer_handler();
        if (delay < 2)
            delay = 2;
        if (delay > 20)
            delay = 20;
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}
esp_err_t hb_ui_start(void)
{
    lv_init();
    const uint32_t font_probe[] = {'A', 0x141, 0x17c};
    for (size_t i = 0; i < sizeof(font_probe) / sizeof(font_probe[0]); ++i)
    {
        lv_font_glyph_dsc_t glyph = {0};
        if (!lv_font_get_glyph_dsc(&hb_font_20, &glyph, font_probe[i], 0) ||
            !lv_font_get_glyph_bitmap(&hb_font_20, font_probe[i]) || !glyph.box_w || !glyph.box_h)
        {
            ESP_LOGE("ui", "Font bitmap unavailable for U+%04x", (unsigned)font_probe[i]);
            return ESP_ERR_INVALID_STATE;
        }
    }
    ESP_LOGI("ui", "Font bitmaps ready: ASCII and Polish");
    pixels = heap_caps_aligned_alloc(64, 800 * 480 * sizeof(lv_color_t),
                                     MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!pixels)
        return ESP_ERR_NO_MEM;
    lv_disp_draw_buf_init(&draw, pixels, NULL, 800 * 480);
    lv_disp_drv_init(&display);
    display.hor_res = 800;
    display.ver_res = 480;
    display.full_refresh = 1;
    display.flush_cb = flush;
    display.draw_buf = &draw;
    if (!lv_disp_drv_register(&display))
        return ESP_ERR_NO_MEM;
    lv_indev_drv_init(&touch);
    touch.type = LV_INDEV_TYPE_POINTER;
    touch.read_cb = read_touch;
    if (!lv_indev_drv_register(&touch))
        return ESP_ERR_NO_MEM;
    screens();
    const esp_timer_create_args_t cfg = {.callback = tick_cb, .name = "lv_tick"};
    esp_err_t err = esp_timer_create(&cfg, &tick);
    if (err != ESP_OK)
        return err;
    err = esp_timer_start_periodic(tick, 2000);
    if (err != ESP_OK)
        return err;
    if (xTaskCreate(task, "lvgl", 8192, NULL, 3, NULL) != pdPASS)
    {
        esp_timer_stop(tick);
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI("ui", "LVGL logical 800x480; single owner task; touch opens menu");
    return ESP_OK;
}
