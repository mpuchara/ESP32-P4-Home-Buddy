// SPDX-License-Identifier: Apache-2.0
#include "board_p4.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hb_brightness_service.h"
#include "hb_camera.h"
#include "hb_camera_preview.h"
#include "hb_dashboard.h"
#include "hb_face.h"
#include "hb_geometry.h"
#include "hb_image.h"
#include "hb_raw_frame.h"
#include "hb_services.h"
#include "hb_ui.h"
#include "hb_wifi.h"
#include "lvgl.h"
#include "screen_lifecycle.h"
#include "ui_commands.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

LV_FONT_DECLARE(hb_font_20);
static lv_disp_draw_buf_t draw;
static lv_disp_drv_t display;
static lv_indev_drv_t touch;
static esp_timer_handle_t tick;
static lv_color_t *pixels;
static lv_obj_t *saver, *menu, *status, *photo, *empty, *clock_label, *temp_label, *bubble,
    *greeting;
static lv_obj_t *page, *keyboard, *field[8], *touch_label;
static QueueHandle_t commands, jobs;
static uint32_t last_touch;
static bool in_menu, critical, diagnostic;
static atomic_bool test_busy;
static atomic_bool image_is_test;
static char network_state[128] = "Wi-Fi: oczekiwanie";
static lv_obj_t *network_info;
static lv_obj_t *network_list;
static char displayed_networks[300];
static lv_obj_t *page_error;
static lv_obj_t *system_info;
static lv_obj_t *test_banner;
static uint32_t image_since;
static char service_state[3][128];
static int section;
static hb_config_t editor;
static hb_weather_config_t weather_editor;
static hb_places_t weather_places;
static lv_obj_t *place_list;
static lv_img_dsc_t image;
static char diagnostic_text[128] = "";
static double outdoor;
static int64_t measured;
static uint32_t freshness = 900;
static bool outdoor_available;
static uint32_t greeting_start;
static void show_form(void);
static void scan(lv_event_t *e);
static void close_page(void);
static void update(lv_timer_t *timer);
static lv_obj_t *camera_info, *camera_photo, *brightness_info, *pwm_info;
static lv_obj_t *face_label, *presence_label, *face_boxes[HB_FACE_MAX];
static lv_img_dsc_t camera_image;
static uint16_t *preview_pixels;
static bool preview_enabled;
static uint32_t preview_tick, preview_sequence;
static uint32_t preview_count, preview_started, camera_info_tick;
static uint32_t present_us;
static hb_brightness_config brightness_editor;
static int calibration;
static float calibration_metric;

static void flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *data)
{
    (void)area;
    int64_t started = esp_timer_get_time();
    board_p4_present_rotated((const uint16_t *)data, 800, 480);
    present_us = (uint32_t)(esp_timer_get_time() - started);
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
        if (diagnostic && touch_label)
            lv_label_set_text_fmt(touch_label, "x=%d  y=%d  (800 x 480)", x, y);
    }
    else
        data->state = LV_INDEV_STATE_RELEASED;
}
static void tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(2);
}
static lv_obj_t *label(lv_obj_t *parent, const char *text, lv_align_t align, int x, int y)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, lv_color_hex(0xffffff), 0);
    lv_obj_align(l, align, x, y);
    return l;
}
static lv_obj_t *screen(void)
{
    lv_obj_t *s = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x10171c), 0);
    lv_obj_set_style_text_font(s, &hb_font_20, 0);
    lv_obj_clear_flag(s, LV_OBJ_FLAG_SCROLLABLE);
    return s;
}
static lv_obj_t *button(lv_obj_t *parent, const char *text, int x, int y, int width,
                        lv_event_cb_t cb, void *user)
{
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, width, 48);
    label(b, text, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, user);
    return b;
}
static void return_menu(lv_event_t *event)
{
    (void)event;
    if (critical)
        return;
    close_page();
    in_menu = true;
    last_touch = lv_tick_get();
    lv_scr_load(menu);
}
static void open_menu(lv_event_t *event)
{
    (void)event;
    in_menu = true;
    last_touch = lv_tick_get();
    lv_scr_load(menu);
}
static void close_menu(lv_event_t *event)
{
    (void)event;
    if (critical)
        return;
    close_page();
    in_menu = false;
    lv_scr_load(saver);
}
static void close_page(void)
{
    hb_face_enable(false);
    face_label = NULL;
    presence_label = NULL;
    memset(face_boxes, 0, sizeof(face_boxes));
    hb_camera_test_pattern(false);
    camera_info = NULL;
    brightness_info = NULL;
    pwm_info = NULL;
    preview_enabled = false;
    if (preview_pixels)
    {
        lv_img_cache_invalidate_src(&camera_image);
        // Delete the image widget before releasing its descriptor storage.
        if (camera_photo)
            lv_obj_del(camera_photo);
        free(preview_pixels);
        preview_pixels = NULL;
        memset(&camera_image, 0, sizeof(camera_image));
    }
    camera_photo = NULL;
    diagnostic = false;
    touch_label = NULL;
    network_info = NULL;
    network_list = NULL;
    place_list = NULL;
    displayed_networks[0] = 0;
    page_error = NULL;
    system_info = NULL;
    if (page)
    {
        // Clear secret text before releasing LVGL buffers.
        for (unsigned i = 0; i < 8; ++i)
            if (field[i])
                lv_textarea_set_text(field[i], "");
        hb_screen_delete(page, saver);
        page = NULL;
    }
    memset(field, 0, sizeof(field));
    keyboard = NULL;
    memset(&editor, 0, sizeof(editor));
}
static void brightness_sync_mode(void)
{
    hb_brightness_config live;
    hb_brightness_get(&live, NULL);
    if (brightness_editor.mode == HB_BRIGHT_OVERRIDE && live.mode != HB_BRIGHT_OVERRIDE)
        brightness_editor.mode = live.mode;
}
static void brightness(lv_event_t *e)
{
    if (critical)
        return;
    brightness_sync_mode();
    brightness_editor.manual_pwm = (uint32_t)lv_slider_get_value(lv_event_get_target(e));
    if (brightness_editor.mode == HB_BRIGHT_AUTO || calibration)
        brightness_editor.mode = HB_BRIGHT_MANUAL;
    hb_brightness_apply(&brightness_editor);
}
static void focus(lv_event_t *e)
{
    if (keyboard)
    {
        lv_keyboard_set_textarea(keyboard, lv_event_get_target(e));
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}
static lv_obj_t *input(lv_obj_t *container, const char *name, const char *value, unsigned max,
                       bool secret, int y)
{
    label(container, name, LV_ALIGN_TOP_LEFT, 0, y);
    lv_obj_t *ta = lv_textarea_create(container);
    lv_obj_set_pos(ta, 270, y - 4);
    lv_obj_set_size(ta, 430, 46);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, max);
    lv_textarea_set_password_mode(ta, secret);
    lv_textarea_set_text(ta, value);
    lv_obj_set_style_text_color(ta, lv_color_black(), 0);
    lv_obj_set_style_text_font(ta, &hb_font_20, 0);
    lv_obj_add_event_cb(ta, focus, LV_EVENT_FOCUSED, NULL);
    return ta;
}
static void page_base(const char *title)
{
    close_page();
    page = screen();
    label(page, title, LV_ALIGN_TOP_LEFT, 20, 18);
    button(page, "Powrót", 650, 8, 130, return_menu, NULL);
    keyboard = lv_keyboard_create(page);
    lv_obj_set_size(keyboard, 800, 170);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    page_error = label(page, "", LV_ALIGN_TOP_LEFT, 24, 300);
    lv_obj_set_width(page_error, 750);
}
static void form_error(const char *text)
{
    if (page_error)
        lv_label_set_text(page_error, text);
    else
        lv_label_set_text(status, text);
}
static void settings(lv_event_t *e)
{
    if (!hb_config_ready())
    {
        hb_ui_diagnostic("Konfiguracja", "Blad NVS; dane zachowane");
        return;
    }
    section = (int)(intptr_t)lv_event_get_user_data(e);
    show_form();
}
static bool number_field(unsigned index, unsigned low, unsigned high, uint32_t *out)
{
    const char *text = lv_textarea_get_text(field[index]);
    if (!text[0])
        return false;
    uint32_t value = 0;
    for (const char *p = text; *p; ++p)
    {
        if (*p < '0' || *p > '9' || value > (high - (unsigned)(*p - '0')) / 10)
            return false;
        value = value * 10 + (unsigned)(*p - '0');
    }
    if (value < low || value > high)
        return false;
    *out = value;
    return true;
}
static void save(lv_event_t *e)
{
    (void)e;
    if (critical)
        return;
    config_job job = {.wifi = section == 0};
    job.value = editor;
    const size_t capacities[3][5] = {{33, 65, 0, 0, 0}, {128, 128, 6, 5, 0}, {128, 0, 0, 0, 0}};
    for (unsigned i = 0; i < 5 && capacities[section][i]; ++i)
        if (strlen(lv_textarea_get_text(field[i])) >= capacities[section][i])
        {
            form_error("Pole jest za długie (limit bajtów UTF-8).");
            memset(&job, 0, sizeof(job));
            return;
        }
    if (section == 0)
    {
        strlcpy(job.value.ssid, lv_textarea_get_text(field[0]), sizeof(job.value.ssid));
        strlcpy(job.value.wifi_password, lv_textarea_get_text(field[1]),
                sizeof(job.value.wifi_password));
    }
    else if (section == 1)
    {
        strlcpy(job.value.ubox_host, lv_textarea_get_text(field[0]), sizeof(job.value.ubox_host));
        strlcpy(job.value.ubox_token, lv_textarea_get_text(field[1]), sizeof(job.value.ubox_token));
        uint32_t port;
        if (!number_field(2, 1, 65535, &port) || !number_field(3, 5, 3600, &job.value.poll_seconds))
        {
            form_error("Port: 1–65535; interwał: 5–3600 s.");
            memset(&job, 0, sizeof(job));
            return;
        }
        job.value.ubox_port = (uint16_t)port;
    }
    else
    {
        const char *city = lv_textarea_get_text(field[0]);
        if (city[0] && !weather_editor.has_location)
        {
            form_error("Wyszukaj i wybierz miejscowość z listy.");
            memset(&job, 0, sizeof(job));
            return;
        }
        job.weather = true;
        job.weather_value = weather_editor;
        if (!city[0])
            job.weather_value = (hb_weather_config_t){.schema = 1};
    }
    if (xQueueSend(jobs, &job, 0) == pdTRUE)
        critical = true;
    memset(&job, 0, sizeof(job));
}
static void disconnect_wifi(lv_event_t *e)
{
    (void)e;
    hb_wifi_disconnect();
}
static void select_network(lv_event_t *e)
{
    if (lv_dropdown_get_selected(lv_event_get_target(e)) == 0)
        return;
    char selected[33];
    lv_dropdown_get_selected_str(lv_event_get_target(e), selected, sizeof(selected));
    lv_textarea_set_text(field[0], selected);
}
static void refresh_networks(void)
{
    if (!network_list)
        return;
    char networks[300];
    hb_wifi_networks(networks, sizeof(networks));
    if (strcmp(networks, displayed_networks))
    {
        char options[340];
        snprintf(options, sizeof(options), "Wybierz sieć%s%s", networks[0] ? "\n" : "", networks);
        lv_dropdown_set_options(network_list, options);
        strlcpy(displayed_networks, networks, sizeof(displayed_networks));
    }
}
static void city_changed(lv_event_t *e)
{
    (void)e;
    weather_editor.has_location = false;
}
static void search_city(lv_event_t *e)
{
    (void)e;
    weather_editor.has_location = false;
    lv_dropdown_set_options(place_list, "Wyszukiwanie...");
    if (!hb_weather_search(lv_textarea_get_text(field[0])))
        form_error("Wpisz co najmniej 2 znaki miejscowości.");
    else
        form_error("Wyszukiwanie Open-Meteo...");
}
static void choose_city(lv_event_t *e)
{
    unsigned index = lv_dropdown_get_selected(lv_event_get_target(e));
    if (!index || index > weather_places.count)
        return;
    hb_place_t *place = &weather_places.place[index - 1];
    lv_textarea_set_text(field[0], place->label);
    weather_editor = (hb_weather_config_t){.schema = 1,
                                           .has_location = true,
                                           .latitude = place->latitude,
                                           .longitude = place->longitude};
    strlcpy(weather_editor.city, place->label, sizeof(weather_editor.city));
    form_error("Wybrano miejscowość. Naciśnij Zapisz.");
}
static void show_form(void)
{
    page_base(section == 0 ? "Wi-Fi" : section == 1 ? "Kamera UBox" : "Pogoda / Open-Meteo");
    hb_config_get(&editor);
    lv_obj_t *fields = lv_obj_create(page);
    lv_obj_set_pos(fields, 12, 70);
    lv_obj_set_size(fields, 776, 226);
    lv_obj_set_style_bg_color(fields, lv_color_hex(0x10171c), 0);
    lv_obj_set_style_pad_all(fields, 8, 0);
    char number[16];
    if (section == 0)
    {
        field[0] = input(fields, "SSID", editor.ssid, 32, false, 4);
        field[1] = input(fields, "Hasło Wi-Fi", editor.wifi_password, 64, true, 64);
        button(fields, "Rozłącz", 0, 124, 160, disconnect_wifi, NULL);
        network_list = lv_dropdown_create(fields);
        lv_obj_set_pos(network_list, 180, 124);
        lv_obj_set_width(network_list, 370);
        lv_dropdown_set_options(network_list, "Wybierz sieć");
        lv_obj_set_style_text_font(network_list, &hb_font_20, 0);
        lv_obj_add_event_cb(network_list, select_network, LV_EVENT_VALUE_CHANGED, NULL);
        button(fields, "Skanuj", 565, 124, 130, scan, NULL);
        refresh_networks();
        label(fields, "Wybierz sieć, wpisz hasło i zapisz. Bez PIN-u.", LV_ALIGN_TOP_LEFT, 0, 184);
        network_info = label(fields, network_state, LV_ALIGN_TOP_LEFT, 0, 244);
    }
    else if (section == 1)
    {
        field[0] = input(fields, "Host / IP HA", editor.ubox_host, 127, false, 4);
        field[1] = input(fields, "Token UBox", editor.ubox_token, 127, false, 64);
        snprintf(number, sizeof(number), "%u", editor.ubox_port);
        field[2] = input(fields, "Port", number, 5, false, 124);
        snprintf(number, sizeof(number), "%u", (unsigned)editor.poll_seconds);
        field[3] = input(fields, "Interwał (s)", number, 4, false, 184);
    }
    else
    {
        hb_config_weather_get(&weather_editor);
        weather_places = (hb_places_t){0};
        field[0] = input(fields, "Miejscowość", weather_editor.city, 127, false, 4);
        lv_obj_add_event_cb(field[0], city_changed, LV_EVENT_VALUE_CHANGED, NULL);
        button(fields, "Szukaj", 0, 64, 160, search_city, NULL);
        place_list = lv_dropdown_create(fields);
        lv_obj_set_pos(place_list, 180, 64);
        lv_obj_set_width(place_list, 520);
        lv_dropdown_set_options(place_list, "Wyszukaj miejscowość");
        lv_obj_set_style_text_font(place_list, &hb_font_20, 0);
        lv_obj_add_event_cb(place_list, choose_city, LV_EVENT_VALUE_CHANGED, NULL);
        label(fields, "Źródło: Open-Meteo.com (dane modelowe)", LV_ALIGN_TOP_LEFT, 0, 124);
        label(fields, "Odświeżanie co 10 min. Zapisz po wyborze.", LV_ALIGN_TOP_LEFT, 0, 184);
    }
    button(page, "Zapisz", 490, 8, 140, save, NULL);
    lv_scr_load(page);
}
static void scan(lv_event_t *e)
{
    (void)e;
    hb_wifi_request_scan();
}
static void system_test(lv_event_t *e)
{
    (void)e;
    page_base("Diagnostyka systemu");
    system_info = label(page, "", LV_ALIGN_TOP_LEFT, 24, 75);
    lv_obj_set_width(system_info, 750);
}
static void test_greeting(lv_event_t *e)
{
    (void)e;
    close_menu(NULL);
    hb_ui_show_greeting("Test UI");
}
static void synthetic_worker(void *arg)
{
    (void)arg;
    extern const uint8_t start[] asm("_binary_synthetic_jpg_start");
    extern const uint8_t end[] asm("_binary_synthetic_jpg_end");
    uint16_t *frame = NULL;
    if (hb_image_decode(start, end - start, &frame) == ESP_OK)
    {
        command c = {.type = CMD_IMAGE, .pixels = frame, .available = true};
        if (xQueueSend(commands, &c, 0) != pdTRUE)
            free(frame);
    }
    else
        hb_ui_diagnostic("JPEG", "Błąd testu JPEG");
    atomic_store(&test_busy, false);
    vTaskDelete(NULL);
}
static void test_image(lv_event_t *e)
{
    (void)e;
    if (atomic_exchange(&test_busy, true))
        return;
    close_menu(NULL);
    if (xTaskCreate(synthetic_worker, "jpeg_test", 6144, NULL, 2, NULL) != pdPASS)
        atomic_store(&test_busy, false);
}
static void touch_test(lv_event_t *e)
{
    (void)e;
    page_base("Test dotyku: narożniki, środek, przeciąganie");
    lv_obj_align(lv_obj_get_child(page, 0), LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_set_pos(lv_obj_get_child(page, 1), 320, 400);
    diagnostic = true;
    touch_label = label(page, "Dotknij znaczników", LV_ALIGN_TOP_MID, 0, 80);
    const int points[][2] = {{12, 12}, {750, 12}, {12, 420}, {750, 420}, {380, 220}};
    for (unsigned i = 0; i < 5; ++i)
    {
        lv_obj_t *dot = lv_obj_create(page);
        lv_obj_set_size(dot, 36, 36);
        lv_obj_set_pos(dot, points[i][0], points[i][1]);
        lv_obj_set_style_bg_color(dot, lv_color_hex(0x22d3ee), 0);
    }
    lv_obj_move_foreground(lv_obj_get_child(page, 1));
    lv_scr_load(page);
}
static void config_worker(void *arg)
{
    (void)arg;
    config_job job;
    for (;;)
    {
        xQueueReceive(jobs, &job, portMAX_DELAY);
        esp_err_t err;
        command result = {0};
        err = job.brightness ? hb_brightness_save(&job.brightness_value)
              : job.weather  ? hb_config_weather_save(&job.weather_value)
              : job.wifi     ? hb_config_save_wifi(job.value.ssid, job.value.wifi_password)
                             : hb_config_save(&job.value);
        result.type = CMD_SAVED;
        if (err)
        {
            result.type = CMD_CONFIG_ERROR;
            strlcpy(result.text, "Błąd zapisu konfiguracji.", sizeof(result.text));
        }
        xQueueSend(commands, &result, portMAX_DELAY);
        memset(&job, 0, sizeof(job));
    }
}
static void preview_toggle(lv_event_t *e)
{
    (void)e;
    preview_enabled = !preview_enabled;
    if (preview_enabled)
    {
        preview_count = 0;
        preview_started = lv_tick_get();
    }
    if (!preview_enabled)
    {
        hb_face_enable(false);
        if (camera_photo)
            lv_obj_add_flag(camera_photo, LV_OBJ_FLAG_HIDDEN);
    }
}
static void pattern_toggle(lv_event_t *e)
{
    (void)e;
    hb_face_enable(false);
    hb_camera_status camera;
    hb_camera_get_status(&camera);
    hb_camera_test_pattern(!camera.test_pattern);
    preview_enabled = true;
    preview_count = 0;
    preview_started = lv_tick_get();
}
static void face_toggle(lv_event_t *e)
{
    (void)e;
    hb_face_status state;
    hb_face_get(&state);
    if (hb_face_enable(!state.enabled) != ESP_OK)
        form_error("Brak pamięci na zadanie AI.");
    if (!state.enabled)
    {
        hb_camera_test_pattern(false);
        preview_enabled = true;
        preview_count = 0;
        preview_started = lv_tick_get();
    }
}
static void local_camera(lv_event_t *e)
{
    (void)e;
    page_base("Kamera lokalna: diagnostyka");
    preview_tick = 0;
    preview_sequence = 0;
    preview_count = 0;
    preview_started = lv_tick_get();
    camera_info_tick = 0;
    button(page, "Podgląd wł. / wył.", 24, 70, 250, preview_toggle, NULL);
    button(page, "Test kolorów", 290, 70, 220, pattern_toggle, NULL);
    lv_obj_t *face_button = button(page, "Test twarzy", 526, 70, 244, face_toggle, NULL);
    face_label = lv_obj_get_child(face_button, 0);
    button(page, "Wygaszacz UBox", 500, 425, 270, close_menu, NULL);
    preview_pixels = heap_caps_malloc(180 * 318 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (preview_pixels)
    {
        memset(preview_pixels, 0, 180 * 318 * 2);
        camera_image = (lv_img_dsc_t){.header = {.cf = LV_IMG_CF_TRUE_COLOR, .w = 180, .h = 318},
                                      .data_size = 180 * 318 * 2,
                                      .data = (const uint8_t *)preview_pixels};
        camera_photo = lv_img_create(page);
        lv_obj_set_pos(camera_photo, 120, 125);
        lv_img_set_src(camera_photo, &camera_image);
        lv_obj_add_flag(camera_photo, LV_OBJ_FLAG_HIDDEN);
    }
    for (unsigned i = 0; i < HB_FACE_MAX; i++)
    {
        face_boxes[i] = lv_obj_create(page);
        lv_obj_set_style_bg_opa(face_boxes[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(face_boxes[i], lv_color_hex(0x33ff77), 0);
        lv_obj_set_style_border_width(face_boxes[i], 2, 0);
        lv_obj_set_style_radius(face_boxes[i], 0, 0);
        lv_obj_clear_flag(face_boxes[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(face_boxes[i], LV_OBJ_FLAG_HIDDEN);
    }
    camera_info = label(page, "Oczekiwanie na kamerę", LV_ALIGN_TOP_LEFT, 430, 130);
    lv_obj_set_width(camera_info, 345);
    presence_label = label(page, "Obecność: wyłączona", LV_ALIGN_TOP_LEFT, 430, 350);
    lv_obj_set_width(presence_label, 345);
    label(page, "Obraz lokalny. Bez zapisu i wysyłania.", LV_ALIGN_TOP_LEFT, 24, 450);
    lv_scr_load(page);
}
static void brightness_mode(lv_event_t *e)
{
    if (critical)
        return;
    calibration = 0;
    brightness_editor.mode = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    if (brightness_editor.mode == HB_BRIGHT_OVERRIDE)
        hb_brightness_begin_override(&brightness_editor);
    else
        hb_brightness_apply(&brightness_editor);
}
static void calibrate(lv_event_t *e)
{
    if (critical)
        return;
    brightness_sync_mode();
    hb_camera_status camera;
    hb_camera_get_status(&camera);
    uint64_t now = (uint64_t)esp_timer_get_time() / 1000;
    if (!camera.running || !camera.light.valid || camera.light_ms > now ||
        now - camera.light_ms > (uint64_t)brightness_editor.sample_seconds * 3000)
    {
        form_error("Brak aktualnego pomiaru światła. Kalibracja niedostępna.");
        return;
    }
    calibration = (int)(intptr_t)lv_event_get_user_data(e);
    calibration_metric = camera.light.metric;
    brightness_editor.mode = HB_BRIGHT_MANUAL;
    hb_brightness_apply(&brightness_editor);
    form_error(camera.light.below_range ? "Próg ciemności pobrany. Ustaw suwak i wybierz Zapisz."
                                        : "Pomiar pobrany. Ustaw suwak i wybierz Zapisz.");
}
static void save_brightness(lv_event_t *e)
{
    (void)e;
    if (critical)
        return;
    brightness_sync_mode();
    if (calibration)
    {
        hb_light_anchor anchor = {
            .metric = calibration_metric, .pwm = brightness_editor.manual_pwm, .valid = 1};
        if (calibration == 1)
            brightness_editor.day = anchor;
        else
            brightness_editor.night = anchor;
    }
    config_job job = {.brightness = true, .brightness_value = brightness_editor};
    if (xQueueSend(jobs, &job, 0) == pdTRUE)
    {
        critical = true;
        form_error("Zapisywanie kalibracji i ustawień...");
    }
    else
        form_error("Zapis zajęty. Spróbuj ponownie.");
}
static void brightness_options(lv_event_t *e)
{
    if (critical)
        return;
    brightness_sync_mode();
    unsigned option = lv_dropdown_get_selected(lv_event_get_target(e));
    if (lv_event_get_user_data(e))
        brightness_editor.transition_seconds = 20 + option * 10;
    else
    {
        static const uint32_t seconds[] = {1, 5, 10, 30, 60};
        if (option < 5)
            brightness_editor.sample_seconds = seconds[option];
    }
    hb_brightness_apply(&brightness_editor);
}
static void fallback_changed(lv_event_t *e)
{
    if (critical)
        return;
    brightness_sync_mode();
    brightness_editor.fallback_pwm = (uint32_t)lv_slider_get_value(lv_event_get_target(e));
    hb_brightness_apply(&brightness_editor);
}
static void brightness_page(lv_event_t *e)
{
    (void)e;
    page_base("Wyświetlacz: Jasność");
    hb_brightness_get(&brightness_editor, NULL);
    calibration = 0;
    button(page, "AUTO", 24, 70, 160, brightness_mode, (void *)HB_BRIGHT_AUTO);
    button(page, "MANUAL", 200, 70, 160, brightness_mode, (void *)HB_BRIGHT_MANUAL);
    button(page, "TYMCZASOWO 5 min", 376, 70, 270, brightness_mode, (void *)HB_BRIGHT_OVERRIDE);
    lv_obj_t *slider = lv_slider_create(page);
    lv_obj_set_pos(slider, 24, 150);
    lv_obj_set_size(slider, 620, 22);
    lv_slider_set_range(slider, 26, 255);
    lv_slider_set_value(slider, brightness_editor.manual_pwm, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, brightness, LV_EVENT_VALUE_CHANGED, NULL);
    pwm_info = label(page, "PWM", LV_ALIGN_TOP_LEFT, 670, 140);
    brightness_info = label(page, "", LV_ALIGN_TOP_LEFT, 24, 192);
    lv_obj_set_width(brightness_info, 750);
    button(page, "Kalibruj dzień", 24, 250, 220, calibrate, (void *)1);
    button(page, "Kalibruj noc", 262, 250, 220, calibrate, (void *)2);
    button(page, "Zapisz", 500, 250, 270, save_brightness, NULL);
    label(page, "Pomiar", LV_ALIGN_TOP_LEFT, 24, 355);
    lv_obj_t *interval = lv_dropdown_create(page);
    lv_obj_set_pos(interval, 115, 345);
    lv_obj_set_width(interval, 140);
    lv_dropdown_set_options(interval, "1 s\n5 s\n10 s\n30 s\n60 s");
    unsigned index = brightness_editor.sample_seconds == 1    ? 0
                     : brightness_editor.sample_seconds == 5  ? 1
                     : brightness_editor.sample_seconds == 10 ? 2
                     : brightness_editor.sample_seconds == 30 ? 3
                                                              : 4;
    lv_dropdown_set_selected(interval, index);
    lv_obj_add_event_cb(interval, brightness_options, LV_EVENT_VALUE_CHANGED, NULL);
    label(page, "Przejście", LV_ALIGN_TOP_LEFT, 300, 355);
    lv_obj_t *response = lv_dropdown_create(page);
    lv_obj_set_pos(response, 420, 345);
    lv_obj_set_width(response, 140);
    lv_dropdown_set_options(response, "20 s\n30 s\n40 s\n50 s\n60 s");
    lv_dropdown_set_selected(response, (brightness_editor.transition_seconds - 20) / 10);
    lv_obj_add_event_cb(response, brightness_options, LV_EVENT_VALUE_CHANGED, (void *)1);
    label(page, "Awaryjna jasność", LV_ALIGN_TOP_LEFT, 24, 420);
    lv_obj_t *fallback = lv_slider_create(page);
    lv_obj_set_pos(fallback, 285, 430);
    lv_obj_set_size(fallback, 420, 22);
    lv_slider_set_range(fallback, 26, 255);
    lv_slider_set_value(fallback, brightness_editor.fallback_pwm, LV_ANIM_OFF);
    lv_obj_add_event_cb(fallback, fallback_changed, LV_EVENT_VALUE_CHANGED, NULL);
    form_error("Wartości ujemne są prawidłowe. Noc kalibruj bez latarki.");
    lv_scr_load(page);
}
static void camera_brightness_update(void)
{
    if (!camera_info && !brightness_info)
        return;
    if ((uint32_t)(lv_tick_get() - preview_tick) < 100)
        return;
    preview_tick = lv_tick_get();
    hb_camera_status camera;
    hb_camera_get_status(&camera);
    hb_brightness_config current;
    uint8_t pwm;
    hb_brightness_get(&current, &pwm);
    uint64_t now = (uint64_t)esp_timer_get_time() / 1000;
    bool fresh = camera.running && camera.light.valid && camera.light_ms <= now &&
                 now - camera.light_ms <= (uint64_t)current.sample_seconds * 3000;
    char light_text[80];
    if (fresh)
        snprintf(light_text, sizeof(light_text), "%s %.3f",
                 camera.light.below_range ? "ciemno (próg)" : "OK", (double)camera.light.metric);
    else
        snprintf(light_text, sizeof(light_text), "brak aktualnego pomiaru");
    if (camera_info)
    {
        if ((uint32_t)(lv_tick_get() - camera_info_tick) >= 1000)
        {
            camera_info_tick = lv_tick_get();
            char text[640];
            snprintf(text, sizeof(text),
                     "ID: 0x%04x  %s\n%u x %u RAW8 | %.1f FPS\nKlatki: %u  Błędy: %u\nPominięte: "
                     "%u\nExp: %.0f us G: %.1f/%.1f\nŚwiatło: %s\nPodgląd: %.1f "
                     "FPS RAM: %u MB\n%s",
                     camera.sensor_id,
                     camera.quarantined    ? "RESTART"
                     : camera.test_pattern ? "TEST"
                     : camera.running      ? "OK"
                                           : "brak",
                     camera.width, camera.height, (double)camera.fps, (unsigned)camera.frames,
                     (unsigned)camera.errors, (unsigned)camera.dropped, (double)camera.exposure_us,
                     (double)camera.analogue_gain, (double)camera.digital_gain, light_text,
                     (double)((float)preview_count * 1000 / (lv_tick_get() - preview_started + 1)),
                     (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / (1024 * 1024)),
                     esp_err_to_name(camera.error));
            lv_label_set_text(camera_info, text);
        }
        if (preview_enabled && preview_pixels)
        {
            hb_camera_frame frame = {0};
            if (hb_camera_acquire(&frame))
            {
                if (frame.sequence != preview_sequence)
                {
                    int64_t started = esp_timer_get_time();
                    hb_raw_thumbnail_upright(frame.pixels, frame.width, frame.height,
                                             preview_pixels, 180, 318);
                    if (!camera.test_pattern)
                        hb_camera_make_preview(preview_pixels, 180, 318, preview_pixels, 180, 318);
                    preview_sequence = frame.sequence;
                    preview_count++;
                    if (preview_count % 50 == 0)
                        ESP_LOGI("preview",
                                 "updates=%u fps=%.1f conversion_us=%u last_present_us=%u",
                                 (unsigned)preview_count,
                                 (double)((float)preview_count * 1000 /
                                          (lv_tick_get() - preview_started + 1)),
                                 (unsigned)(esp_timer_get_time() - started), (unsigned)present_us);
                    lv_img_cache_invalidate_src(&camera_image);
                    lv_obj_clear_flag(camera_photo, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_invalidate(camera_photo);
                }
                hb_camera_release(&frame);
            }
        }
    }
    if (face_label)
    {
        hb_face_status face;
        hb_face_get(&face);
        char caption[80];
        if (face.error != ESP_OK)
            snprintf(caption, sizeof(caption), "%s",
                     face.error == ESP_ERR_NO_MEM ? "AI: brak pamięci" : "AI: błąd");
        else if (!face.enabled)
            snprintf(caption, sizeof(caption), "Test twarzy");
        else if (!face.ready)
            snprintf(caption, sizeof(caption), "AI: start...");
        else if (!hb_face_fresh(&face, now))
            snprintf(caption, sizeof(caption), "AI: czekam...");
        else
            snprintf(caption, sizeof(caption), "Twarze: %u / %u ms", (unsigned)face.count,
                     (unsigned)(face.inference_us / 1000));
        if (strcmp(lv_label_get_text(face_label), caption))
            lv_label_set_text(face_label, caption);
        char occupancy[128];
        hb_presence_status p = face.presence;
        const char *value = p.value == HB_PRESENCE_PRESENT  ? "TAK"
                            : p.value == HB_PRESENCE_ABSENT ? "NIE"
                            : p.value == HB_PRESENCE_OFF    ? "wyłączona"
                                                            : "nieustalona";
        if (p.fresh && p.misses)
            snprintf(occupancy, sizeof(occupancy), "Obecność: %s\nBrak twarzy: %u/4 (min. 5 s)",
                     value, p.misses);
        else if (p.fresh && p.value != HB_PRESENCE_PRESENT)
            snprintf(occupancy, sizeof(occupancy), "Obecność: %s\nWykrycia: %u/2", value, p.hits);
        else
            snprintf(occupancy, sizeof(occupancy), "Obecność: %s", value);
        if (presence_label && strcmp(lv_label_get_text(presence_label), occupancy))
            lv_label_set_text(presence_label, occupancy);
        bool visible = preview_enabled && !camera.test_pattern && hb_face_fresh(&face, now);
        for (unsigned i = 0; i < HB_FACE_MAX; i++)
        {
            if (visible && i < face.count)
            {
                hb_face_box b = face.boxes[i];
                lv_obj_set_pos(face_boxes[i], 120 + b.x1, 125 + b.y1);
                lv_obj_set_size(face_boxes[i], b.x2 - b.x1 + 1, b.y2 - b.y1 + 1);
                lv_obj_clear_flag(face_boxes[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
                lv_obj_add_flag(face_boxes[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (brightness_info)
    {
        if (brightness_editor.mode == HB_BRIGHT_OVERRIDE && current.mode != HB_BRIGHT_OVERRIDE)
            brightness_editor.mode = current.mode;
        lv_label_set_text_fmt(pwm_info, "%u / 255", pwm);
        char text[400];
        snprintf(text, sizeof(text), "%s | Światło: %s\nKalibracja: %s | Dzień: %s | Noc: %s",
                 current.mode == HB_BRIGHT_AUTO     ? "AUTO"
                 : current.mode == HB_BRIGHT_MANUAL ? "MANUAL"
                                                    : "TYMCZASOWO",
                 light_text,
                 hb_brightness_calibrated(&brightness_editor) ? "OK"
                                                              : "niepełna / zbyt bliskie punkty",
                 brightness_editor.day.valid ? "zapisany" : "brak",
                 brightness_editor.night.valid ? "zapisany" : "brak");
        lv_label_set_text(brightness_info, text);
    }
}
bool hb_ui_publish_image(uint16_t *frame)
{
    command c = {.type = CMD_IMAGE, .pixels = frame};
    return commands && xQueueSend(commands, &c, 0) == pdTRUE;
}
bool hb_ui_image_is_test(void)
{
    return atomic_load(&image_is_test);
}
void hb_ui_weather_places(const hb_places_t *places)
{
    if (!commands || !places)
        return;
    hb_places_t *copy = malloc(sizeof(*copy));
    if (!copy)
        return;
    *copy = *places;
    command c = {.type = CMD_PLACES, .places = copy};
    if (xQueueSend(commands, &c, 0) != pdTRUE)
        free(copy);
}
bool hb_ui_show_greeting(const char *name)
{
    if (!name || strlen(name) > 60)
        return false;
    command c = {.type = CMD_GREETING};
    snprintf(c.text, sizeof(c.text), "Cześć %s!", name);
    return commands && xQueueSend(commands, &c, 0) == pdTRUE;
}
void hb_ui_diagnostic(const char *service, const char *message)
{
    command c = {.type = CMD_DIAGNOSTIC};
    snprintf(c.text, sizeof(c.text), "%s: %s", service, message);
    if (commands)
        xQueueSend(commands, &c, 0);
}
void hb_ui_temperature(bool available, double value, int64_t timestamp, uint32_t max_age)
{
    command c = {.type = CMD_TEMPERATURE,
                 .available = available,
                 .temperature = value,
                 .timestamp = timestamp,
                 .max_age = max_age};
    if (commands)
        xQueueSend(commands, &c, 0);
}
static void update(lv_timer_t *timer)
{
    (void)timer;
    camera_brightness_update();
    command c;
    while (xQueueReceive(commands, &c, 0) == pdTRUE)
    {
        if (c.type == CMD_IMAGE)
        {
            const uint8_t *old = image.data;
            image = (lv_img_dsc_t){.header = {.cf = LV_IMG_CF_TRUE_COLOR, .w = 800, .h = 480},
                                   .data_size = 768000,
                                   .data = (const uint8_t *)c.pixels};
            lv_img_cache_invalidate_src(&image);
            lv_img_set_src(photo, &image);
            image_since = lv_tick_get();
            atomic_store(&image_is_test, c.available);
            if (c.available)
                lv_obj_clear_flag(test_banner, LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(test_banner, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(empty, LV_OBJ_FLAG_HIDDEN);
            free((void *)old);
        }
        else if (c.type == CMD_PLACES)
        {
            if (place_list && field[0] && !strcmp(c.places->query, lv_textarea_get_text(field[0])))
            {
                weather_places = *c.places;
                char options[800] = "Wybierz miejscowość";
                for (unsigned i = 0; i < weather_places.count; ++i)
                {
                    strlcat(options, "\n", sizeof(options));
                    strlcat(options, weather_places.place[i].label, sizeof(options));
                }
                lv_dropdown_set_options(place_list, options);
                form_error(weather_places.count ? "Wybierz wynik i zapisz."
                                                : "Nie znaleziono miejscowości.");
            }
            free(c.places);
        }
        else if (c.type == CMD_GREETING)
        {
            lv_label_set_text(greeting, c.text);
            greeting_start = lv_tick_get();
            lv_obj_clear_flag(bubble, LV_OBJ_FLAG_HIDDEN);
        }
        else if (c.type == CMD_TEMPERATURE)
        {
            outdoor_available = c.available;
            outdoor = c.temperature;
            measured = c.timestamp;
            freshness = c.max_age;
        }
        else if (c.type == CMD_SAVED)
        {
            critical = false;
            return_menu(NULL);
            strlcpy(diagnostic_text, "Zapisano konfigurację", sizeof(diagnostic_text));
        }
        else if (c.type == CMD_CONFIG_ERROR)
        {
            critical = false;
            return_menu(NULL);
            strlcpy(diagnostic_text, c.text, sizeof(diagnostic_text));
        }
        else
        {
            strlcpy(diagnostic_text, c.text, sizeof(diagnostic_text));
            if (!strncmp(c.text, "Wi-Fi:", 6))
            {
                strlcpy(network_state, c.text, sizeof(network_state));
                strlcpy(service_state[0], c.text, sizeof(service_state[0]));
            }
            else if (!strncmp(c.text, "UBox:", 5))
                strlcpy(service_state[1], c.text, sizeof(service_state[1]));
            else if (!strncmp(c.text, "Pogoda:", 7))
            {
                strlcpy(service_state[2], c.text, sizeof(service_state[2]));
                if (place_list)
                    form_error(c.text);
            }
        }
    }
    char clock[6];
    hb_clock_text(hb_time_valid(), time(NULL), clock);
    if (strcmp(lv_label_get_text(clock_label), clock))
        lv_label_set_text(clock_label, clock);
    char text[32] = "--°C";
    if (outdoor_available && hb_time_valid() && hb_fresh(measured, time(NULL), freshness))
    {
        snprintf(text, sizeof(text), "%.1f°C", outdoor);
        for (char *p = text; *p; ++p)
            if (*p == '.')
                *p = ',';
    }
    if (strcmp(lv_label_get_text(temp_label), text))
        lv_label_set_text(temp_label, text);
    if (strcmp(lv_label_get_text(status), diagnostic_text))
        lv_label_set_text(status, diagnostic_text);
    if (network_info && strcmp(lv_label_get_text(network_info), network_state))
        lv_label_set_text(network_info, network_state);
    refresh_networks();
    if (system_info)
    {
        char info[768];
        snprintf(info, sizeof(info),
                 "Firmware 0.4.3-dev\n%s\n%s\n%s\nSNTP: %s\n"
                 "RAM wolna / blok: %u / %u\nPSRAM wolna / blok: %u / %u\n"
                 "Bufor LVGL: 768000 B; JPEG: limit 2 MiB\nObraz zaakceptowany: %u s temu%s",
                 service_state[0], service_state[1], service_state[2],
                 hb_time_valid() ? "OK" : "brak synchronizacji",
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
                 image.data ? (unsigned)((lv_tick_get() - image_since) / 1000) : 0,
                 image.data ? " (czas odbioru, nie wykonania)" : " (brak obrazu)");
        if (strcmp(lv_label_get_text(system_info), info))
            lv_label_set_text(system_info, info);
    }
    if (!lv_obj_has_flag(bubble, LV_OBJ_FLAG_HIDDEN))
    {
        uint32_t elapsed = lv_tick_get() - greeting_start;
        if (elapsed >= 4800)
            lv_obj_add_flag(bubble, LV_OBJ_FLAG_HIDDEN);
        else
        {
            unsigned opacity = elapsed < 300    ? elapsed * 255 / 300
                               : elapsed < 4300 ? 255
                                                : (4800 - elapsed) * 255 / 500;
            lv_obj_set_style_opa(bubble, (lv_opa_t)opacity, 0);
        }
    }
    if (in_menu && !critical && !diagnostic && (uint32_t)(lv_tick_get() - last_touch) >= 30000)
        close_menu(NULL);
}
static void screens(void)
{
    saver = screen();
    photo = lv_img_create(saver);
    lv_obj_set_size(photo, 800, 480);
    lv_obj_set_pos(photo, 0, 0);
    empty = label(saver, "Brak zapisanej migawki UBox", LV_ALIGN_CENTER, 0, 0);
    test_banner = label(saver, "TEST JPEG / obraz syntetyczny", LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_set_style_bg_color(test_banner, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(test_banner, 180, 0);
    lv_obj_add_flag(test_banner, LV_OBJ_FLAG_HIDDEN);
    clock_label = label(saver, "--:--", LV_ALIGN_BOTTOM_LEFT, 24, -18);
    temp_label = label(saver, "--°C", LV_ALIGN_BOTTOM_RIGHT, -24, -18);
    lv_obj_set_style_text_font(clock_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_shadow_width(clock_label, 8, 0);
    lv_obj_set_style_text_opa(clock_label, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(clock_label, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(clock_label, 100, 0);
    lv_obj_set_style_bg_color(temp_label, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(temp_label, 100, 0);
    bubble = lv_obj_create(saver);
    lv_obj_set_size(bubble, 500, 76);
    lv_obj_center(bubble);
    lv_obj_set_style_bg_color(bubble, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(bubble, 180, 0);
    lv_obj_set_style_radius(bubble, 20, 0);
    greeting = label(bubble, "", LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(bubble, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(bubble, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(saver, open_menu, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(photo, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(photo, LV_OBJ_FLAG_CLICKABLE);
    menu = screen();
    label(menu, "Home Buddy: ustawienia", LV_ALIGN_TOP_LEFT, 24, 14);
    button(menu, "Wi-Fi", 24, 68, 230, settings, (void *)0);
    button(menu, "Kamera UBox", 282, 68, 230, settings, (void *)1);
    button(menu, "Pogoda", 540, 68, 230, settings, (void *)2);
    button(menu, "Test JPEG", 24, 136, 230, test_image, NULL);
    button(menu, "Test powitania", 282, 136, 230, test_greeting, NULL);
    button(menu, "Test dotyku", 540, 136, 230, touch_test, NULL);
    button(menu, "Wyświetlacz", 24, 204, 230, brightness_page, NULL);
    button(menu, "Kamera lokalna", 282, 204, 230, local_camera, NULL);
    button(menu, "Skanuj Wi-Fi", 24, 290, 230, scan, NULL);
    button(menu, "System", 282, 290, 230, system_test, NULL);
    button(menu, "Powrót", 620, 410, 150, close_menu, NULL);
    status = label(menu, "", LV_ALIGN_TOP_LEFT, 24, 355);
    lv_obj_set_width(status, 740);
    lv_timer_create(update, 20, NULL);
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
    const uint32_t probe[] = {'A', 0x141, 0x17c};
    for (unsigned i = 0; i < 3; ++i)
        if (!lv_font_get_glyph_bitmap(&hb_font_20, probe[i]))
            return ESP_ERR_INVALID_STATE;
    commands = xQueueCreate(4, sizeof(command));
    jobs = xQueueCreate(2, sizeof(config_job));
    if (!commands || !jobs)
        return ESP_ERR_NO_MEM;
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
    if (err)
        return err;
    err = esp_timer_start_periodic(tick, 2000);
    if (err)
        return err;
    if (xTaskCreate(config_worker, "config", 8192, NULL, 1, NULL) != pdPASS)
        return ESP_ERR_NO_MEM;
    if (xTaskCreate(task, "lvgl", 8192, NULL, 3, NULL) != pdPASS)
        return ESP_ERR_NO_MEM;
    ESP_LOGI("ui", "LVGL 800x480 ready; queued image/config commands");
    return ESP_OK;
}
