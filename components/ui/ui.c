// SPDX-License-Identifier: Apache-2.0
#include "board_p4.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hb_dashboard.h"
#include "hb_geometry.h"
#include "hb_image.h"
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
static lv_obj_t *page, *keyboard, *field[8], *pin_field, *pin_repeat, *touch_label;
static QueueHandle_t commands, jobs;
static uint32_t last_touch;
static bool in_menu, critical, diagnostic;
static atomic_bool test_busy;
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
        if (diagnostic && touch_label)
            lv_label_set_text_fmt(touch_label, "x=%d  y=%d  (800 × 480)", x, y);
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
    diagnostic = false;
    touch_label = NULL;
    network_info = NULL;
    network_list = NULL;
    displayed_networks[0] = 0;
    page_error = NULL;
    system_info = NULL;
    if (page)
    {
        // Clear secret text before releasing LVGL buffers.
        for (unsigned i = 0; i < 8; ++i)
            if (field[i])
                lv_textarea_set_text(field[i], "");
        if (pin_field)
            lv_textarea_set_text(pin_field, "");
        if (pin_repeat)
            lv_textarea_set_text(pin_repeat, "");
        hb_screen_delete(page, saver);
        page = NULL;
    }
    memset(field, 0, sizeof(field));
    pin_field = NULL;
    pin_repeat = NULL;
    keyboard = NULL;
    memset(&editor, 0, sizeof(editor));
}
static void brightness(lv_event_t *e)
{
    board_p4_set_brightness((uint8_t)lv_slider_get_value(lv_event_get_target(e)));
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
static void submit_pin(lv_event_t *e)
{
    (void)e;
    if (critical)
        return;
    config_job job = {0};
    const char *p = lv_textarea_get_text(pin_field);
    if (strlen(p) < 6 || strlen(p) > 12 ||
        (pin_repeat && strcmp(p, lv_textarea_get_text(pin_repeat))))
    {
        form_error("PIN: 6–12 cyfr; potwierdzenia muszą się zgadzać.");
        return;
    }
    strcpy(job.pin, p);
    if (xQueueSend(jobs, &job, 0) == pdTRUE)
        critical = true;
    memset(&job, 0, sizeof(job));
}
static void settings(lv_event_t *e)
{
    if (!hb_config_ready())
    {
        hb_ui_diagnostic("Konfiguracja", "Błąd NVS; dane zachowane");
        return;
    }
    section = (int)(intptr_t)lv_event_get_user_data(e);
    if (section == 0)
    {
        show_form();
        return;
    }
    page_base("Konfiguracja chroniona PIN");
    bool first = !hb_config_has_pin();
    label(page, first ? "Ustaw własny PIN (6–12 cyfr)" : "Podaj PIN administratora",
          LV_ALIGN_TOP_LEFT, 24, 76);
    pin_field = lv_textarea_create(page);
    lv_obj_set_pos(pin_field, 24, 112);
    lv_obj_set_size(pin_field, 330, 48);
    lv_textarea_set_one_line(pin_field, true);
    lv_textarea_set_max_length(pin_field, 12);
    lv_textarea_set_password_mode(pin_field, true);
    lv_textarea_set_accepted_chars(pin_field, "0123456789");
    lv_obj_add_event_cb(pin_field, focus, LV_EVENT_FOCUSED, NULL);
    if (first)
    {
        pin_repeat = lv_textarea_create(page);
        lv_obj_set_pos(pin_repeat, 400, 112);
        lv_obj_set_size(pin_repeat, 330, 48);
        lv_textarea_set_one_line(pin_repeat, true);
        lv_textarea_set_max_length(pin_repeat, 12);
        lv_textarea_set_password_mode(pin_repeat, true);
        lv_textarea_set_accepted_chars(pin_repeat, "0123456789");
        lv_textarea_set_placeholder_text(pin_repeat, "Powtórz PIN");
        lv_obj_add_event_cb(pin_repeat, focus, LV_EVENT_FOCUSED, NULL);
    }
    button(page, "Otwórz", 24, 205, 180, submit_pin, NULL);
    lv_scr_load(page);
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
    config_job job = {.save = true, .wifi = section == 0};
    job.value = editor;
    const size_t capacities[3][5] = {
        {33, 65, 0, 0, 0}, {128, 128, 6, 5, 0}, {160, 65, 128, 128, 6}};
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
        strlcpy(job.value.mqtt_uri, lv_textarea_get_text(field[0]), sizeof(job.value.mqtt_uri));
        strlcpy(job.value.mqtt_user, lv_textarea_get_text(field[1]), sizeof(job.value.mqtt_user));
        strlcpy(job.value.mqtt_password, lv_textarea_get_text(field[2]),
                sizeof(job.value.mqtt_password));
        strlcpy(job.value.temperature_topic, lv_textarea_get_text(field[3]),
                sizeof(job.value.temperature_topic));
        if (!number_field(4, 30, 86400, &job.value.temperature_age))
        {
            form_error("Ważność temperatury: 30–86400 s.");
            memset(&job, 0, sizeof(job));
            return;
        }
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
static void show_form(void)
{
    page_base(section == 0 ? "Wi-Fi" : section == 1 ? "Kamera UBox" : "MQTT / temperatura");
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
        field[1] = input(fields, "Token UBox", editor.ubox_token, 127, true, 64);
        snprintf(number, sizeof(number), "%u", editor.ubox_port);
        field[2] = input(fields, "Port", number, 5, false, 124);
        snprintf(number, sizeof(number), "%u", (unsigned)editor.poll_seconds);
        field[3] = input(fields, "Interwał (s)", number, 4, false, 184);
    }
    else
    {
        field[0] = input(fields, "Adres mqtt(s)://", editor.mqtt_uri, 159, false, 4);
        field[1] = input(fields, "Użytkownik", editor.mqtt_user, 64, false, 64);
        field[2] = input(fields, "Hasło MQTT", editor.mqtt_password, 127, true, 124);
        field[3] = input(fields, "Temat temperatury", editor.temperature_topic, 127, false, 184);
        snprintf(number, sizeof(number), "%u", (unsigned)editor.temperature_age);
        field[4] = input(fields, "Ważność (s)", number, 5, false, 244);
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
        if (job.save)
        {
            err = job.wifi ? hb_config_save_wifi(job.value.ssid, job.value.wifi_password)
                           : hb_config_save(&job.value, NULL);
            result.type = CMD_SAVED;
        }
        else
        {
            if (!hb_config_has_pin())
            {
                hb_config_t c;
                hb_config_get(&c);
                err = hb_config_save(&c, job.pin);
            }
            else
                err = ESP_OK;
            if (!err)
                err = hb_config_unlock(job.pin);
            result.type = CMD_FORM;
        }
        if (err)
        {
            result.type = CMD_CONFIG_ERROR;
            strlcpy(result.text, "Błąd PIN / konfiguracji. Po 5 próbach odczekaj minutę.",
                    sizeof(result.text));
        }
        xQueueSend(commands, &result, portMAX_DELAY);
        memset(&job, 0, sizeof(job));
    }
}
bool hb_ui_publish_image(uint16_t *frame)
{
    command c = {.type = CMD_IMAGE, .pixels = frame};
    return commands && xQueueSend(commands, &c, 0) == pdTRUE;
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
            if (c.available)
                lv_obj_clear_flag(test_banner, LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(test_banner, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(empty, LV_OBJ_FLAG_HIDDEN);
            free((void *)old);
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
        else if (c.type == CMD_FORM)
        {
            critical = false;
            last_touch = lv_tick_get();
            show_form();
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
            else if (!strncmp(c.text, "MQTT:", 5))
                strlcpy(service_state[2], c.text, sizeof(service_state[2]));
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
                 "Firmware 0.2.0-dev\n%s\n%s\n%s\nSNTP: %s\n"
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
    button(menu, "MQTT", 540, 68, 230, settings, (void *)2);
    button(menu, "Test JPEG", 24, 136, 230, test_image, NULL);
    button(menu, "Test powitania", 282, 136, 230, test_greeting, NULL);
    button(menu, "Test dotyku", 540, 136, 230, touch_test, NULL);
    label(menu, "Podświetlenie", LV_ALIGN_TOP_LEFT, 24, 218);
    lv_obj_t *slider = lv_slider_create(menu);
    lv_obj_set_size(slider, 440, 22);
    lv_obj_set_pos(slider, 285, 232);
    lv_slider_set_range(slider, 26, 255);
    lv_slider_set_value(slider, 128, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, brightness, LV_EVENT_VALUE_CHANGED, NULL);
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
