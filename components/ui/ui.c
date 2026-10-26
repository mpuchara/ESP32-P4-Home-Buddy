// SPDX-License-Identifier: Apache-2.0
#include "board_p4.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hb_audio.h"
#include "hb_brightness_service.h"
#include "hb_camera.h"
#include "hb_camera_preview.h"
#include "hb_dashboard.h"
#include "hb_face.h"
#include "hb_geometry.h"
#include "hb_ha.h"
#include "hb_household_voice.h"
#include "hb_identity.h"
#include "hb_image.h"
#include "hb_raw_frame.h"
#include "hb_services.h"
#include "hb_ui.h"
#include "hb_voice.h"
#include "hb_wifi.h"
#include "identity_navigation.h"
#include "lvgl.h"
#include "product_menu.h"
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
static lv_obj_t *page_error, *profile_enable_button;
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
static lv_obj_t *monitor_info, *quality_info;
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
static lv_obj_t *identity_info, *identity_choice, *identity_marks[5], *identity_box;
// Only the LVGL task accesses these snapshots. Page transitions may be nested
// inside refresh, so page, refresh and preview must never alias each other.
static struct
{
    hb_id_status page, refresh, preview;
} identity_workspace;
_Static_assert(sizeof(identity_workspace) <= 6144, "UI identity workspace exceeds fixed budget");
static uint64_t identity_selected;
static bool identity_screen;
static bool voice_screen;
static bool recognition_settings;
static lv_obj_t *audio_info, *audio_meter, *product_info, *enroll_progress;
static lv_obj_t *product_states[5];
static unsigned nav_group, back_group;
static bool household_list, guided_enrollment, confirming, creating_profile;
static uint64_t created_before, household_signature;
static uint32_t create_started;
static uint64_t profile_ids[HB_ID_PEOPLE];
static bool enrollment_observed, resume_auto;
static lv_obj_t *commit_button;
static bool audio_testing;
static lv_obj_t *brightness_manual, *brightness_day, *brightness_night;
static void brightness_tuning(lv_event_t *e);
static void settings_page(lv_event_t *e);
static void information_page(lv_event_t *e);
static void advanced_page(lv_event_t *e);
static void profile_page(lv_event_t *e);
static void voice_enrollment_page(lv_event_t *e);
static void return_parent(lv_event_t *e);
static bool identity_saver_intent;
static uint32_t identity_tick;
static void identity_page(lv_event_t *e);
static void household_page(lv_event_t *e);
static lv_obj_t *ha_enable, *ha_publish, *ha_tls, *ha_info, *ha_mode;

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
    lv_obj_t *caption = label(b, text, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_width(caption, width - 24);
    lv_obj_set_height(caption, 28);
    lv_label_set_long_mode(caption, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
    if (cb)
        lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, user);
    return b;
}
static void return_menu(lv_event_t *event)
{
    (void)event;
    if (critical)
        return;
    close_page();
    nav_group = 0;
    in_menu = true;
    last_touch = lv_tick_get();
    lv_scr_load(menu);
}
static void open_menu(lv_event_t *event)
{
    (void)event;
    nav_group = 0;
    in_menu = true;
    last_touch = lv_tick_get();
    lv_scr_load(menu);
}
static void close_menu(lv_event_t *event)
{
    (void)event;
    if (critical)
        return;
    if (identity_screen && identity_saver_intent)
        identity_screen = false;
    close_page();
    in_menu = false;
    lv_scr_load(saver);
}
static void close_page(void)
{
    if (identity_screen || voice_screen)
    {
        ESP_LOGI("identity_ui", "close identity screen stack_min=%u",
                 (unsigned)uxTaskGetStackHighWaterMark(NULL));
        hb_id_command stop = {.type = HB_ID_CMD_CANCEL};
        // Cancellation is admitted outside the bounded queue and invalidates stale work.
        hb_identity_command(&stop);
        if (resume_auto)
        {
            hb_id_command resume = {.type = HB_ID_CMD_AUTO};
            hb_identity_command(&resume);
        }
    }
    identity_screen = false;
    voice_screen = false;
    resume_auto = false;
    recognition_settings = false;
    household_list = guided_enrollment = confirming = creating_profile = false;
    product_info = audio_meter = enroll_progress = commit_button = NULL;
    memset(product_states, 0, sizeof(product_states));
    brightness_manual = brightness_day = brightness_night = NULL;
    audio_testing = false;
    hb_voice_diagnostic_stop();
    hb_audio_stop();
    audio_info = NULL;
    identity_saver_intent = false;
    ha_enable = ha_publish = ha_tls = ha_info = ha_mode = NULL;
    identity_info = identity_choice = identity_box = profile_enable_button = NULL;
    memset(identity_marks, 0, sizeof(identity_marks));
    hb_face_enable(false);
    face_label = NULL;
    monitor_info = NULL;
    quality_info = NULL;
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
static void brightness_visibility(void)
{
    if (!brightness_manual)
        return;
    if (brightness_editor.mode == HB_BRIGHT_AUTO)
    {
        lv_obj_add_flag(brightness_manual, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(brightness_day, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(brightness_night, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_flag(brightness_manual, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(brightness_day, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(brightness_night, LV_OBJ_FLAG_HIDDEN);
    }
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
    brightness_visibility();
}
static void focus(lv_event_t *e)
{
    if (keyboard)
    {
        lv_keyboard_set_textarea(keyboard, lv_event_get_target(e));
        // Forms are built after the keyboard; keep their labels/buttons below it.
        lv_obj_move_foreground(keyboard);
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}
static void keyboard_done(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_READY && code != LV_EVENT_CANCEL)
        return;
    lv_obj_t *kb = lv_event_get_target(e);
    lv_keyboard_set_textarea(kb, NULL);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
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
    lv_obj_add_event_cb(ta, focus, LV_EVENT_CLICKED, NULL);
    return ta;
}
static void page_base(const char *title)
{
    close_page();
    page = screen();
    label(page, title, LV_ALIGN_TOP_LEFT, 20, 18);
    back_group = nav_group;
    button(page, "Wstecz", 650, 8, 130, return_parent, NULL);
    keyboard = lv_keyboard_create(page);
    lv_obj_set_size(keyboard, 800, 170);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x202a32), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(keyboard, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_font(keyboard, LV_FONT_DEFAULT, LV_PART_ITEMS);
    lv_obj_add_event_cb(keyboard, keyboard_done, LV_EVENT_ALL, NULL);
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
static void identity_send(lv_event_t *e)
{
    hb_id_command command = {.type = (hb_id_command_type)(uintptr_t)lv_event_get_user_data(e),
                             .person_id = identity_selected};
    if (command.type == HB_ID_CMD_SAVE && commit_button)
        lv_obj_add_state(commit_button, LV_STATE_DISABLED);
    if (command.type == HB_ID_CMD_BEGIN)
        command.guided = true;
    if (field[0])
        snprintf(command.name, sizeof(command.name), "%s", lv_textarea_get_text(field[0]));
    bool accepted = hb_identity_command(&command) == ESP_OK;
    identity_saver_intent = hb_identity_saver_intent(identity_saver_intent, command.type, accepted);
    if (!accepted)
        form_error("Polecenie nieprzyjęte. Spróbuj ponownie.");
    else
    {
        if (command.type == HB_ID_CMD_STOP)
            resume_auto = false;
        if (keyboard)
            lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        if (command.type == HB_ID_CMD_BACKGROUND)
            form_error("Tło przyjęte. Możesz otworzyć wygaszacz.");
        else if (command.type == HB_ID_CMD_STOP || command.type == HB_ID_CMD_TEST)
            form_error("Polecenie przyjęte.");
    }
}
static void audio_start(lv_event_t *e)
{
    (void)e;
    if (hb_audio_start() != ESP_OK)
        form_error("Nie można uruchomić mikrofonu");
    else
        audio_testing = true;
}
static void audio_stop(lv_event_t *e)
{
    (void)e;
    hb_voice_diagnostic_stop();
    hb_audio_stop();
}
static void voice_test(lv_event_t *e)
{
    unsigned slot = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    if (hb_voice_diagnostic_start(slot) != ESP_OK)
        form_error("Test modelu głosu niedostępny");
    else
        audio_testing = true;
}
static void audio_page(lv_event_t *e)
{
    (void)e;
    page_base("Mikrofon: diagnostyka");
    button(page, "Uruchom test 60 s", 24, 70, 350, audio_start, NULL);
    button(page, "Stop", 400, 70, 230, audio_stop, NULL);
    audio_meter = lv_bar_create(page);
    lv_obj_set_pos(audio_meter, 24, 120);
    lv_obj_set_size(audio_meter, 745, 18);
    lv_bar_set_range(audio_meter, 0, 32768);
    audio_info = label(page, "Test wyłączony. Dźwięk pozostaje lokalny; bez zapisu i wysyłania.",
                       LV_ALIGN_TOP_LEFT, 24, 150);
    lv_obj_set_width(audio_info, 745);
    button(page, "Model 3 s: slot L", 24, 350, 350, voice_test, (void *)(uintptr_t)0);
    button(page, "Model 3 s: slot R", 412, 350, 364, voice_test, (void *)(uintptr_t)1);
}
static void recognition_page(lv_event_t *e)
{
    (void)e;
    page_base("Rozpoznawanie");
    recognition_settings = true;
    button(page, "Wyłącz", 24, 78, 350, identity_send, (void *)(uintptr_t)HB_ID_CMD_STOP);
    button(page, "Włącz automatyczne", 412, 78, 364, identity_send,
           (void *)(uintptr_t)HB_ID_CMD_AUTO);
    button(page, "Dodaj domownika", 24, 350, 350, household_page, NULL);
    lv_obj_t *voice_switch = button(page, "Głos: oczekuje na walidację", 412, 350, 364, NULL, NULL);
    lv_obj_add_state(voice_switch, LV_STATE_DISABLED);
    identity_info = label(page, "Oczekiwanie na stan", LV_ALIGN_TOP_LEFT, 24, 160);
    lv_obj_set_width(identity_info, 745);
    lv_obj_set_pos(page_error, 24, 410);
    lv_label_set_text(page_error, "Rozpoznawanie włączasz świadomie na tym urządzeniu.");
    lv_scr_load(page);
}
static void identity_guest(lv_event_t *e)
{
    (void)e;
    hb_id_command command = {.type = HB_ID_CMD_EXPECTED};
    if (hb_identity_command(&command) != ESP_OK)
        form_error("Polecenie nieprzyjęte");
}
static uint64_t profile_signature(const hb_id_status *s)
{
    uint64_t hash = s->profile_count + s->storage_saved + 3 * s->storage_saving +
                    7 * hb_config_biometric_notice();
    for (unsigned i = 0; i < s->profile_count; ++i)
    {
        hash =
            hash * 31 + s->profiles[i].id + s->profiles[i].samples + s->profiles[i].voice_samples;
        for (const unsigned char *p = (const unsigned char *)s->profiles[i].name; *p; ++p)
            hash = hash * 31 + *p;
    }
    return hash;
}
static void identity_delete(lv_event_t *e)
{
    (void)e;
    uint64_t id = identity_selected;
    page_base("Usunąć domownika?");
    identity_selected = id;
    confirming = true;
    label(page, "Usuniesz profil i wszystkie jego próbki.", LV_ALIGN_TOP_LEFT, 24, 120);
    button(page, "Potwierdzam usunięcie", 24, 200, 350, identity_send,
           (void *)(uintptr_t)HB_ID_CMD_DELETE);
    button(page, "Anuluj", 420, 200, 300, household_page, NULL);
    identity_info = label(page, "", LV_ALIGN_TOP_LEFT, 24, 270);
    lv_scr_load(page);
}
static void add_person(lv_event_t *e)
{
    (void)e;
    if (!hb_config_biometric_notice())
    {
        page_base("Lokalny zapis profili");
        lv_obj_t *notice = label(
            page,
            "Profile twarzy i głosu będą zapisywane lokalnie w pamięci urządzenia bez szyfrowania.",
            LV_ALIGN_TOP_LEFT, 24, 90);
        lv_obj_set_width(notice, 740);
        button(page, "Rozumiem i akceptuję", 24, 250, 460, identity_send,
               (void *)(uintptr_t)HB_ID_CMD_NOTICE_ACK);
        button(page, "Dalej", 24, 350, 350, add_person, NULL);
        identity_info = label(page, "", LV_ALIGN_TOP_LEFT, 24, 190);
        lv_scr_load(page);
        return;
    }
    page_base("Dodaj domownika");
    field[0] = input(page, "Imię", "", 60, false, 100);
    button(page, "Dalej: rejestracja twarzy", 24, 200, 420, profile_page, (void *)1);
    lv_scr_load(page);
}
static void household_page(lv_event_t *e)
{
    (void)e;
    page_base("Domownicy");
    nav_group = 4;
    back_group = 0;
    household_list = true;
    hb_identity_start();
    hb_id_status *s = &identity_workspace.page;
    hb_identity_get(s);
    household_signature = profile_signature(s);
    label(page, s->persistence_message, LV_ALIGN_TOP_LEFT, 24, 52);
    lv_obj_t *list = lv_obj_create(page);
    lv_obj_set_pos(list, 24, 90);
    lv_obj_set_size(list, 752, 244);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x10171c), 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    for (unsigned i = 0; i < s->profile_count; ++i)
    {
        char row[HB_ID_NAME + 48];
        snprintf(row, sizeof(row), "%s     %s", s->profiles[i].name,
                 s->profiles[i].samples ? "Twarz: gotowa" : "Brak rejestracji");
        profile_ids[i] = s->profiles[i].id;
        button(list, row, 0, i * 60, 700, profile_page, (void *)(uintptr_t)(i + 2));
    }
    if (!s->profile_count)
        label(list, "Dodaj pierwszego domownika.", LV_ALIGN_TOP_LEFT, 0, 0);
    button(page, "+ Dodaj domownika", 24, 356, 420, add_person, NULL);
    button(page, "Ponów zapis", 470, 356, 306, identity_send,
           (void *)(uintptr_t)HB_ID_CMD_RETRY_SAVE);
    lv_scr_load(page);
}
static void profile_enable(lv_event_t *e)
{
    (void)e;
    hb_id_status *s = &identity_workspace.page;
    hb_identity_get(s);
    for (unsigned i = 0; i < s->profile_count; ++i)
        if (s->profiles[i].id == identity_selected)
        {
            hb_id_command c = {.type = HB_ID_CMD_ENABLE,
                               .person_id = identity_selected,
                               .guided = !s->profiles[i].enabled};
            if (hb_identity_command(&c) != ESP_OK)
                form_error("Polecenie nieprzyjęte");
            break;
        }
}
static void profile_page(lv_event_t *e)
{
    unsigned action = (uintptr_t)lv_event_get_user_data(e);
    uint64_t id = action >= 2 && action - 2 < HB_ID_PEOPLE ? profile_ids[action - 2] : 0;
    // Add workflow waits for the worker receipt; no profile index/name guessing.
    if (action == 1 && field[0])
    {
        if (creating_profile)
            return;
        hb_id_status *s = &identity_workspace.page;
        hb_identity_get(s);
        hb_id_command c = {.type = HB_ID_CMD_CREATE};
        snprintf(c.name, sizeof(c.name), "%s", lv_textarea_get_text(field[0]));
        if (hb_identity_command(&c) != ESP_OK)
        {
            form_error("Nie można przyjąć polecenia. Spróbuj ponownie.");
            return;
        }
        created_before = s->created_id;
        creating_profile = true;
        create_started = lv_tick_get();
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        form_error("Przygotowywanie profilu...");
        return;
    }
    page_base("Domownik");
    identity_selected = id;
    hb_id_status *s = &identity_workspace.page;
    hb_identity_get(s);
    const char *name = "";
    unsigned samples = 0, voice_samples = 0;
    bool enabled = false;
    for (unsigned i = 0; i < s->profile_count; ++i)
        if (s->profiles[i].id == id)
        {
            name = s->profiles[i].name;
            samples = s->profiles[i].samples;
            voice_samples = s->profiles[i].voice_samples;
            enabled = s->profiles[i].enabled;
        }
    label(page, samples ? "Twarz: zarejestrowana" : "Twarz: brak rejestracji", LV_ALIGN_TOP_LEFT,
          24, 70);
    label(page, voice_samples ? "Głos: próbki lokalne" : "Głos: brak rejestracji",
          LV_ALIGN_TOP_LEFT, 412, 70);
    field[0] = input(page, "Imię", name, 60, false, 130);
    button(page, "Zmień nazwę", 24, 210, 350, identity_send, (void *)(uintptr_t)HB_ID_CMD_RENAME);
    button(page, "Uzupełnij rejestrację", 412, 210, 364, identity_page, (void *)1);
    button(page, "Usuń domownika", 24, 280, 350, identity_delete, NULL);
    lv_obj_t *voice_button =
        button(page, s->voice_available ? "Rejestruj głos" : "Głos: oczekuje na walidację", 412,
               280, 364, voice_enrollment_page, NULL);
    if (!s->voice_available)
        lv_obj_add_state(voice_button, LV_STATE_DISABLED);
    profile_enable_button = button(page, enabled ? "Wyłącz profil" : "Włącz profil", 24, 345, 350,
                                   profile_enable, (void *)(uintptr_t)!enabled);
    button(page, "Ponów zapis", 412, 345, 364, identity_send,
           (void *)(uintptr_t)HB_ID_CMD_RETRY_SAVE);
    identity_info = label(page, "", LV_ALIGN_TOP_LEFT, 24, 400);
    lv_obj_set_width(identity_info, 745);
    lv_scr_load(page);
}
static void voice_enrollment_page(lv_event_t *e)
{
    (void)e;
    hb_id_status *s = &identity_workspace.page;
    hb_identity_get(s);
    if (!s->voice_available)
    {
        form_error("Głos oczekuje na walidację mikrofonu i modelu");
        return;
    }
    uint64_t id = identity_selected;
    page_base("Rejestracja głosu");
    identity_selected = id;
    voice_screen = true;
    resume_auto = s->auto_enabled;
    label(page, "Cztery różne wypowiedzi, potem nowa do weryfikacji.", LV_ALIGN_TOP_LEFT, 24, 78);
    label(page, "Mów samodzielnie, naturalnie. Zapis profilu po potwierdzeniu.", LV_ALIGN_TOP_LEFT,
          24, 110);
    button(page, "Rozpocznij", 24, 160, 350, identity_send,
           (void *)(uintptr_t)HB_ID_CMD_VOICE_BEGIN);
    button(page, "Pobierz próbkę", 412, 160, 364, identity_send,
           (void *)(uintptr_t)HB_ID_CMD_VOICE_SAMPLE);
    button(page, "Weryfikuj nową wypowiedzią", 24, 230, 350, identity_send,
           (void *)(uintptr_t)HB_ID_CMD_VOICE_VERIFY);
    button(page, "Potwierdź zapis profilu", 412, 230, 364, identity_send,
           (void *)(uintptr_t)HB_ID_CMD_VOICE_SAVE);
    identity_info =
        label(page, "Naciśnij Rozpocznij. Mikrofon jest wyłączony.", LV_ALIGN_TOP_LEFT, 24, 320);
    lv_obj_set_width(identity_info, 745);
    lv_scr_load(page);
}
static void identity_page(lv_event_t *e)
{
    bool enroll = e && (uintptr_t)lv_event_get_user_data(e) == 1;
    hb_id_status *previous = &identity_workspace.page;
    hb_identity_get(previous);
    bool restore = resume_auto || previous->auto_enabled;
    uint64_t id = identity_selected;
    ESP_LOGI("identity_ui", "open identity screen enroll=%d stack_min=%u", enroll,
             (unsigned)uxTaskGetStackHighWaterMark(NULL));
    page_base(enroll ? "Rejestracja twarzy" : "Rozpoznawanie twarzy: eksperyment");
    identity_selected = id;
    identity_screen = true;
    resume_auto = restore;
    guided_enrollment = enroll;
    enrollment_observed = false;
    diagnostic = true;
    hb_id_command start = {
        .type = enroll ? HB_ID_CMD_BEGIN : HB_ID_CMD_TEST, .person_id = id, .guided = enroll};
    if (hb_identity_command(&start) != ESP_OK)
        form_error("Nie można uruchomić sesji");
    hb_id_command expected = {.type = HB_ID_CMD_EXPECTED, .person_id = id};
    hb_identity_command(&expected);
    if (enroll)
    {
        enroll_progress = lv_bar_create(page);
        lv_obj_set_pos(enroll_progress, 205, 90);
        lv_obj_set_size(enroll_progress, 575, 24);
        lv_bar_set_range(enroll_progress, 0, HB_ID_MIN_SAMPLES);
        commit_button = button(page, "Potwierdź zapis profilu", 205, 320, 575, identity_send,
                               (void *)(uintptr_t)HB_ID_CMD_SAVE);
        lv_obj_add_state(commit_button, LV_STATE_DISABLED);
    }
    else
    {
        button(page, "Oceń próbę", 205, 68, 180, identity_send,
               (void *)(uintptr_t)HB_ID_CMD_EVALUATE);
        button(page, "Gość", 398, 68, 180, identity_guest, NULL);
        button(page, "Wybrana osoba", 590, 68, 190, identity_send,
               (void *)(uintptr_t)HB_ID_CMD_EXPECTED);
        button(page, "MSR / ESPDet", 398, 125, 180, identity_send,
               (void *)(uintptr_t)HB_ID_CMD_LANDMARK_METHOD);
        button(page, "Stop", 590, 125, 190, identity_send, (void *)(uintptr_t)HB_ID_CMD_STOP);
    }
    preview_pixels = heap_caps_calloc(180 * 318, 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (preview_pixels)
    {
        camera_image = (lv_img_dsc_t){.header = {.cf = LV_IMG_CF_TRUE_COLOR, .w = 180, .h = 318},
                                      .data_size = 180 * 318 * 2,
                                      .data = (const uint8_t *)preview_pixels};
        camera_photo = lv_img_create(page);
        lv_obj_set_pos(camera_photo, 12, 70);
        lv_img_set_src(camera_photo, &camera_image);
    }
    identity_box = lv_obj_create(page);
    lv_obj_set_style_bg_opa(identity_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(identity_box, lv_color_hex(0x33ff77), 0);
    lv_obj_set_style_border_width(identity_box, 2, 0);
    lv_obj_clear_flag(identity_box, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(identity_box, LV_OBJ_FLAG_HIDDEN);
    for (unsigned i = 0; i < 5; ++i)
    {
        identity_marks[i] = lv_obj_create(page);
        lv_obj_set_size(identity_marks[i], 7, 7);
        lv_obj_set_style_bg_color(identity_marks[i], lv_color_hex(0xffcc33), 0);
        lv_obj_set_style_bg_opa(identity_marks[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(identity_marks[i], 0, 0);
        lv_obj_clear_flag(identity_marks[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(identity_marks[i], LV_OBJ_FLAG_HIDDEN);
    }
    identity_info = label(page, "Oczekiwanie na model lokalny", LV_ALIGN_TOP_LEFT, 205, 188);
    lv_obj_set_width(identity_info, 575);
    lv_obj_set_pos(page_error, 205, 390);
    lv_obj_set_width(page_error, 575);
    lv_label_set_text(page_error, "Profile lokalne. Progi niekalibrowane.");
    if (!enroll)
        button(page, "Włącz tło na 10 min", 205, 420, 280, identity_send,
               (void *)(uintptr_t)HB_ID_CMD_BACKGROUND);
    // Deliberate background start followed by saver preserves this independent session.
    button(page, "Wygaszacz UBox", 500, 420, 280, close_menu, NULL);
    lv_scr_load(page);
}
static void identity_update(void)
{
    if ((uint32_t)(lv_tick_get() - identity_tick) < 500)
        return;
    identity_tick = lv_tick_get();
    hb_id_status *s = &identity_workspace.refresh;
    hb_identity_get(s);
    if (creating_profile)
    {
        if (s->created_id && s->created_id != created_before)
        {
            identity_selected = s->created_id;
            creating_profile = false;
            lv_event_t event = {.user_data = (void *)1};
            identity_page(&event);
            return;
        }
        if ((uint32_t)(lv_tick_get() - create_started) > 5000)
        {
            creating_profile = false;
            form_error("Nie dodano profilu. Sprawdź imię i liczbę domowników.");
        }
    }
    if (household_list && household_signature != profile_signature(s))
    {
        household_page(NULL);
        return;
    }
    if (confirming && s->deleted_id == identity_selected && s->storage_saved)
    {
        household_page(NULL);
        return;
    }
    if (guided_enrollment && s->mode != HB_ID_ENROLL && !s->storage_saved && enrollment_observed)
    {
        form_error(s->persistence_message);
        if (commit_button)
        {
            lv_obj_clear_state(commit_button, LV_STATE_DISABLED);
            lv_obj_remove_event_cb(commit_button, identity_send);
            lv_obj_add_event_cb(commit_button, identity_send, LV_EVENT_CLICKED,
                                (void *)(uintptr_t)HB_ID_CMD_RETRY_SAVE);
        }
    }
    if (guided_enrollment && s->mode == HB_ID_ENROLL)
        enrollment_observed = true;
    if (guided_enrollment && enrollment_observed && s->saved_id == identity_selected &&
        s->storage_saved && s->mode != HB_ID_ENROLL)
    {
        household_page(NULL);
        return;
    }
    if (product_info)
    {
        hb_camera_status camera;
        hb_camera_get_status(&camera);
        hb_ha_status ha;
        hb_ha_get_status(&ha);
        lv_label_set_text(product_info, "Home Buddy | firmware 0.11.2-dev");
        const char *names[] = {"Wi-Fi", "UBox", "Kamera", "Rozpoznawanie", "Home Assistant"};
        const char *values[] = {network_state, image.data ? "Obraz dostępny" : "Oczekiwanie",
                                camera.running ? "Dostępna" : "Oczekiwanie",
                                s->mode == HB_ID_AUTO && s->ready ? "Aktywne"
                                                                  : "Wyłączone / oczekiwanie",
                                hb_ha_result_text(ha.result)};
        bool good[] = {strstr(network_state, "IP ") != NULL ||
                           strstr(network_state, "Połączono") != NULL,
                       image.data != NULL, camera.running, s->mode == HB_ID_AUTO && s->ready,
                       ha.result == HB_HA_HOST_OK || ha.result == HB_HA_DELIVERED};
        for (unsigned i = 0; i < 5; ++i)
        {
            lv_label_set_text_fmt(product_states[i], "%s: %s", names[i], values[i]);
            lv_obj_set_style_text_color(product_states[i],
                                        lv_color_hex(good[i] ? 0x71dba2 : 0xb5bdc4), 0);
        }
    }
    if (!in_menu && !hb_ui_image_is_test())
    {
        char name[HB_ID_NAME];
        if (hb_identity_greeting((uint64_t)esp_timer_get_time() / 1000, name))
            hb_ui_show_greeting(name);
    }
    if (audio_info)
    {
        hb_audio_status a;
        hb_audio_get(&a);
        if (a.frames || a.errors)
            audio_testing = a.active;
        char voice_status[160];
        hb_voice_diagnostic_status(voice_status, sizeof(voice_status));
        lv_bar_set_value(audio_meter, a.rms, LV_ANIM_OFF);
        lv_label_set_text_fmt(
            audio_info,
            "%s | układ: %s\nPCM mono: %u Hz | RMS %u peak %u clip %u\n"
            "Bufor: %u B | DMA: %u B\nRamki: %u | błędy: %u | utracone: %u\n"
            "DC: %ld | drugi slot RMS/peak: %u/%u | próbki: %llu\n%s\n%s",
            a.active ? "AKTYWNY" : "Wyłączony", a.available ? "dostępny" : "niezweryfikowany",
            (unsigned)a.sample_rate, (unsigned)a.rms, (unsigned)a.peak, a.clipped,
            (unsigned)a.app_bytes, (unsigned)a.dma_bytes, (unsigned)a.frames, (unsigned)a.errors,
            (unsigned)a.dropped_samples, (long)a.dc_offset, (unsigned)a.right_rms,
            (unsigned)a.right_peak, (unsigned long long)a.sample_count,
            a.suspicious ? "Sygnał podejrzany: stały lub powtarzalny" : a.message, voice_status);
    }
    if (profile_enable_button)
        for (unsigned i = 0; i < s->profile_count; ++i)
            if (s->profiles[i].id == identity_selected)
                lv_label_set_text(lv_obj_get_child(profile_enable_button, 0),
                                  s->profiles[i].enabled ? "Wyłącz profil" : "Włącz profil");
    if (!identity_info)
        return;
    if (recognition_settings)
    {
        const char *mode = !s->usable_profiles                 ? "Oczekiwanie na domowników"
                           : s->mode == HB_ID_AUTO && s->ready ? "Aktywne"
                           : s->mode == HB_ID_AUTO             ? "Niedostępne / uruchamianie"
                                                               : "Wyłączone";
        lv_label_set_text_fmt(identity_info,
                              "Automatyczne rozpoznawanie: %s\nStatus: %s\nDomownicy: %u\nOstatnio "
                              "rozpoznano: %s\nModel: %s\n%s",
                              s->auto_enabled ? "Włączone" : "Wyłączone", mode, s->profile_count,
                              s->last_recognized_ms ? s->last_recognized : "Brak",
                              s->ready ? "Dostępny" : "Niezaładowany",
                              s->usable_profiles ? ""
                                                 : "Dodaj domownika, aby rozpocząć rozpoznawanie.");
        return;
    }
    if (!identity_screen)
    {
        if (voice_screen)
            lv_label_set_text_fmt(identity_info, "Próbki: %u/4\n%s", s->voice_enrollment_samples,
                                  s->message);
        else
            lv_label_set_text_fmt(identity_info, "%s\n%s", s->persistence_message, s->message);
        return;
    }
    const char *states[] = {"UNAVAILABLE", "UNKNOWN", "AMBIGUOUS", "KNOWN"};
    const char *candidate = "brak";
    for (unsigned i = 0; i < s->profile_count; ++i)
        if (s->profiles[i].id == s->match.person_id)
            candidate = s->profiles[i].name;
    char best[20] = "brak", second[20] = "brak";
    if (s->match.state != HB_ID_UNAVAILABLE && s->match.best > -1.0f)
        snprintf(best, sizeof(best), "%.3f", (double)s->match.best);
    if (s->match.state != HB_ID_UNAVAILABLE && s->match.second > -1.0f)
        snprintf(second, sizeof(second), "%.3f", (double)s->match.second);
    char text[750];
    snprintf(text, sizeof(text),
             "MBF: %s | twarze: %u | %s\n%s | %s\nPodobieństwo: %s / inna osoba: %s\n"
             "D/L/MBF+align: %u/%u/%u ms | razem %u\n"
             "KiB PSRAM %u/%u | RAM %u/%u\nQ: %.0f / %.0f / %.0f | próbki: %u\n"
             "Próby znane %u: TA %u FR %u mylne %u\nGość %u: FA %u CR %u | AMB %u brak %u\n%s",
             s->ready ? "gotowy" : "brak", s->faces, states[s->match.state], candidate,
             s->confirmed ? "potwierdzony" : "kandydat", best, second,
             (unsigned)(s->detection_us / 1000), (unsigned)(s->landmarks_us / 1000),
             (unsigned)(s->embedding_us / 1000), (unsigned)(s->total_us / 1000),
             (unsigned)(s->psram_free / 1024), (unsigned)(s->psram_largest / 1024),
             (unsigned)(s->internal_free / 1024), (unsigned)(s->internal_largest / 1024),
             (double)s->quality.mean, (double)s->quality.contrast, (double)s->quality.detail,
             s->samples, s->evaluation.known_trials, s->evaluation.true_accept,
             s->evaluation.false_reject, s->evaluation.confusion, s->evaluation.unknown_trials,
             s->evaluation.unknown_accept, s->evaluation.unknown_reject, s->evaluation.ambiguous,
             s->evaluation.unavailable, s->message);
    if (guided_enrollment)
    {
        lv_label_set_text_fmt(identity_info, "Rejestracja twarzy: %u/%u próbek\n%s\n%s", s->samples,
                              HB_ID_MIN_SAMPLES,
                              s->faces > 1  ? "W kadrze może być tylko jedna osoba."
                              : s->verified ? "Weryfikacja zakończona. Potwierdź zapis."
                                            : "Spójrz w kamerę. Zmieniaj lekko ułożenie głowy.",
                              s->samples >= HB_ID_MIN_SAMPLES && !s->verified
                                  ? "Poczekaj na weryfikację nową klatką."
                                  : "");
        lv_bar_set_value(enroll_progress, s->samples, LV_ANIM_OFF);
        if (s->verified || (s->mode != HB_ID_ENROLL && !s->storage_saved))
            lv_obj_clear_state(commit_button, LV_STATE_DISABLED);
        else
            lv_obj_add_state(commit_button, LV_STATE_DISABLED);
        lv_label_set_text(page_error, s->persistence_message);
        lv_label_set_text(lv_obj_get_child(commit_button, 0),
                          s->mode != HB_ID_ENROLL && !s->storage_saved ? "Ponów zapis"
                                                                       : "Potwierdź zapis profilu");
    }
    else
        lv_label_set_text(identity_info, text);
    if (preview_pixels)
    {
        hb_id_status *preview = &identity_workspace.preview;
        bool same = hb_identity_preview(preview_pixels, 180 * 318, preview);
        if (!same)
        {
            hb_camera_frame frame = {0};
            if (hb_camera_acquire(&frame))
            {
                hb_raw_thumbnail_upright(frame.pixels, frame.width, frame.height, preview_pixels,
                                         180, 318);
                hb_camera_make_preview(preview_pixels, 180, 318, preview_pixels, 180, 318);
                hb_camera_release(&frame);
            }
        }
        lv_img_cache_invalidate_src(&camera_image);
        lv_obj_invalidate(camera_photo);
        // Native sensor dimensions are represented by the ROI mapping's fixed admitted format.
        // Reject overlay if the format changes instead of applying guessed coordinates.
        hb_camera_status cs;
        hb_camera_get_status(&cs);
        for (unsigned i = 0; i < 5; ++i)
        {
            if (same && cs.width && cs.height && preview->landmarks.p[i].x > 0)
            {
                lv_obj_set_pos(identity_marks[i],
                               12 + preview->landmarks.p[i].x * 180 / cs.height - 3,
                               70 + preview->landmarks.p[i].y * 318 / cs.width - 3);
                lv_obj_clear_flag(identity_marks[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
                lv_obj_add_flag(identity_marks[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (same && cs.width && cs.height && preview->roi.w > 0)
        {
            lv_obj_set_pos(identity_box, 12 + preview->roi.x * 180 / cs.height,
                           70 + preview->roi.y * 318 / cs.width);
            lv_obj_set_size(identity_box, preview->roi.w * 180 / cs.height,
                            preview->roi.h * 318 / cs.width);
            lv_obj_clear_flag(identity_box, LV_OBJ_FLAG_HIDDEN);
        }
        else
            lv_obj_add_flag(identity_box, LV_OBJ_FLAG_HIDDEN);
    }
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
#include "ha_settings.h"
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
    lv_obj_set_scroll_dir(fields, LV_DIR_VER);
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
        err = job.ha           ? hb_ha_config_save(&job.ha_value)
              : job.brightness ? hb_brightness_save(&job.brightness_value)
              : job.weather    ? hb_config_weather_save(&job.weather_value)
              : job.wifi       ? hb_config_save_wifi(job.value.ssid, job.value.wifi_password)
                               : hb_config_save(&job.value);
        result.type = job.ha ? CMD_HA_SAVED : CMD_SAVED;
        if (err)
        {
            result.type = job.ha ? CMD_HA_CONFIG_ERROR : CMD_CONFIG_ERROR;
            snprintf(result.text, sizeof(result.text), "Błąd zapisu konfiguracji: %s.",
                     esp_err_to_name(err));
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
    hb_face_monitor(false);
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
    if (hb_face_enable(!state.diagnostic_enabled) != ESP_OK)
        form_error("Brak pamięci na zadanie AI.");
    if (!state.diagnostic_enabled)
    {
        hb_camera_test_pattern(false);
        preview_enabled = true;
        preview_count = 0;
        preview_started = lv_tick_get();
    }
}
static void monitor_action(lv_event_t *e)
{
    bool enabled = (bool)(uintptr_t)lv_event_get_user_data(e);
    if (enabled)
        hb_camera_test_pattern(false);
    if (hb_face_monitor(enabled) != ESP_OK)
        form_error("Nie można uruchomić testu obecności.");
}
static void monitor_page(lv_event_t *e)
{
    (void)e;
    page_base("Obecność: test pracy w tle");
    button(page, "Uruchom 10 minut", 24, 70, 350, monitor_action, (void *)1);
    button(page, "Zatrzymaj", 410, 70, 350, monitor_action, NULL);
    monitor_info = label(page, "Test jest wyłączony", LV_ALIGN_TOP_LEFT, 24, 145);
    lv_obj_set_width(monitor_info, 740);
    label(page, "Test działa po wyjściu do wygaszacza. Restart kończy test.", LV_ALIGN_TOP_LEFT, 24,
          350);
    label(page, "Obecność na podstawie twarzy. Bez zapisu i wysyłania zdjęć.", LV_ALIGN_TOP_LEFT,
          24, 385);
    button(page, "Wygaszacz UBox", 500, 425, 270, close_menu, NULL);
    lv_scr_load(page);
}
static void quality_action(lv_event_t *e)
{
    bool enabled = (bool)(uintptr_t)lv_event_get_user_data(e);
    if (enabled)
        hb_camera_test_pattern(false);
    if (hb_face_enable(enabled) != ESP_OK)
        form_error("Brak pamięci na pomiar twarzy.");
}
static void quality_page(lv_event_t *e)
{
    (void)e;
    page_base("Jakość obrazu twarzy: diagnostyka");
    button(page, "Uruchom pomiar", 24, 70, 350, quality_action, (void *)1);
    button(page, "Zatrzymaj pomiar", 410, 70, 350, quality_action, NULL);
    quality_info = label(page, "Uruchom pomiar i spójrz w kamerę.", LV_ALIGN_TOP_LEFT, 24, 140);
    lv_obj_set_width(quality_info, 740);
    label(page, "Progi orientacyjne. Pomiar nie odrzuca wykrytej twarzy.", LV_ALIGN_TOP_LEFT, 24,
          385);
    button(page, "Wygaszacz UBox", 500, 425, 270, close_menu, NULL);
    lv_scr_load(page);
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
    brightness_visibility();
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
    brightness_visibility();
}
static void fallback_changed(lv_event_t *e)
{
    if (critical)
        return;
    brightness_sync_mode();
    brightness_editor.fallback_pwm = (uint32_t)lv_slider_get_value(lv_event_get_target(e));
    hb_brightness_apply(&brightness_editor);
    brightness_visibility();
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
    brightness_manual = slider;
    lv_obj_set_pos(slider, 24, 150);
    lv_obj_set_size(slider, 620, 22);
    lv_slider_set_range(slider, 26, 255);
    lv_slider_set_value(slider, brightness_editor.manual_pwm, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, brightness, LV_EVENT_VALUE_CHANGED, NULL);
    pwm_info = label(page, "PWM", LV_ALIGN_TOP_LEFT, 670, 140);
    brightness_info = label(page, "", LV_ALIGN_TOP_LEFT, 24, 192);
    lv_obj_set_width(brightness_info, 750);
    brightness_day = button(page, "Kalibruj dzień", 24, 250, 220, calibrate, (void *)1);
    brightness_night = button(page, "Kalibruj noc", 262, 250, 220, calibrate, (void *)2);
    button(page, "Zapisz", 500, 250, 270, save_brightness, NULL);
    brightness_visibility();
    button(page, "Ustawienia pomiaru", 24, 355, 420, brightness_tuning, NULL);
    lv_scr_load(page);
}
static void brightness_tuning(lv_event_t *e)
{
    (void)e;
    page_base("Ustawienia pomiaru jasności");
    back_group = 5;
    hb_brightness_get(&brightness_editor, NULL);
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
    button(page, "Zapisz", 500, 250, 270, save_brightness, NULL);
    lv_scr_load(page);
}

static void camera_brightness_update(void)
{
    if (!camera_info && !brightness_info && !monitor_info && !quality_info)
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
    if (quality_info)
    {
        hb_face_status face;
        hb_face_get(&face);
        char text[600];
        if (face.error != ESP_OK)
            snprintf(text, sizeof(text), "Pomiar niedostępny: %s", esp_err_to_name(face.error));
        else if (!face.diagnostic_enabled)
            snprintf(text, sizeof(text), "Uruchom pomiar i spójrz w kamerę.");
        else if (!hb_face_fresh(&face, now))
            snprintf(text, sizeof(text), "Oczekiwanie na nowy pomiar...");
        else if (!face.count)
            snprintf(text, sizeof(text), "Nie wykryto twarzy.");
        else
        {
            unsigned best = 0;
            for (unsigned i = 1; i < face.count; i++)
                if ((unsigned)face.quality[i].width * face.quality[i].height >
                    (unsigned)face.quality[best].width * face.quality[best].height)
                    best = i;
            hb_face_quality q = face.quality[best];
            const char *hint = q.flags & HB_FQ_EDGE           ? "Twarz przy brzegu kadru"
                               : q.flags & HB_FQ_SMALL        ? "Mała twarz - przybliż się"
                               : q.flags & HB_FQ_DARK         ? "Ciemny wycinek twarzy"
                               : q.flags & HB_FQ_BRIGHT       ? "Jasny wycinek twarzy"
                               : q.flags & HB_FQ_LOW_CONTRAST ? "Niski kontrast"
                               : q.flags & HB_FQ_LOW_DETAIL   ? "Mało detali"
                                                              : "Brak uwag testowych";
            if (!q.valid)
                snprintf(text, sizeof(text), "Brak poprawnego wycinka twarzy.");
            else
                snprintf(text, sizeof(text),
                         "Największa twarz: %u x %u px (wejście AI)\nJasność: %.1f / "
                         "255\nKontrast: %.1f\nDetal: %.1f\nSkrajne piksele: %.1f %%\nUwagi: "
                         "%s\nPomiar: %u us | Model: %u ms",
                         (unsigned)q.width, (unsigned)q.height, (double)q.mean, (double)q.contrast,
                         (double)q.detail, (double)q.near_limit_percent, hint,
                         (unsigned)face.quality_us, (unsigned)(face.inference_us / 1000));
        }
        if (strcmp(lv_label_get_text(quality_info), text))
            lv_label_set_text(quality_info, text);
    }
    if (monitor_info)
    {
        hb_face_status face;
        hb_face_get(&face);
        const char *value = face.presence.value == HB_PRESENCE_PRESENT  ? "TAK"
                            : face.presence.value == HB_PRESENCE_ABSENT ? "NIE"
                            : face.presence.value == HB_PRESENCE_OFF    ? "wyłączona"
                                                                        : "nieustalona";
        char text[400];
        unsigned seconds = (face.monitor_remaining_ms + 999) / 1000;
        snprintf(text, sizeof(text),
                 "Test w tle: %s\nPozostało: %u:%02u\nObecność: %s\nAnaliza: %u ms | %s\nPamięć "
                 "PSRAM: %u MB",
                 face.monitor_remaining_ms ? "WŁĄCZONY" : "wyłączony", seconds / 60, seconds % 60,
                 value, (unsigned)(face.inference_us / 1000), esp_err_to_name(face.error),
                 (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / (1024 * 1024)));
        if (strcmp(lv_label_get_text(monitor_info), text))
            lv_label_set_text(monitor_info, text);
    }
    if (face_label)
    {
        hb_face_status face;
        hb_face_get(&face);
        char caption[80];
        if (face.error != ESP_OK)
            snprintf(caption, sizeof(caption), "%s",
                     face.error == ESP_ERR_NO_MEM ? "AI: brak pamięci" : "AI: błąd");
        else if (!face.diagnostic_enabled)
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
        bool visible = preview_enabled && face.diagnostic_enabled && !camera.test_pattern &&
                       hb_face_fresh(&face, now);
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
        lv_label_set_text_fmt(
            brightness_info, "Jasność: %s | Kalibracja: %s",
            current.mode == HB_BRIGHT_AUTO ? "Automatyczna" : "Ręczna / tymczasowa",
            hb_brightness_calibrated(&brightness_editor) ? "Gotowa" : "Ustaw dzień i noc");
        brightness_visibility();
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
    identity_update();
    if (ha_info)
    {
        hb_ha_status hs;
        hb_ha_get_status(&hs);
        lv_label_set_text(ha_info, hb_ha_result_text(hs.result));
    }
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
        else if (c.type == CMD_HA_SAVED || c.type == CMD_HA_CONFIG_ERROR)
        {
            critical = false;
            if (ha_info)
            {
                if (keyboard)
                    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
                form_error(c.type == CMD_HA_SAVED ? "Zapisano ustawienia HA w pamięci urządzenia."
                                                  : c.text);
            }
        }
        else if (c.type == CMD_SAVED)
        {
            critical = false;
            form_error("Zapisano konfigurację.");
        }
        else if (c.type == CMD_CONFIG_ERROR)
        {
            critical = false;
            form_error(c.text);
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
    lv_label_set_text(status, image.data ? "Obraz gotowy" : "Oczekiwanie na obraz");
    if (network_info && strcmp(lv_label_get_text(network_info), network_state))
        lv_label_set_text(network_info, network_state);
    refresh_networks();
    if (system_info)
    {
        char info[768];
        snprintf(info, sizeof(info),
                 "Firmware 0.4.5-dev\n%s\n%s\n%s\nSNTP: %s\n"
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
    if (in_menu && hb_product_timeout(lv_tick_get(), last_touch, critical || creating_profile,
                                      keyboard && !lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN),
                                      diagnostic || guided_enrollment || audio_testing, confirming))
        close_menu(NULL);
}
static void return_parent(lv_event_t *e)
{
    switch (back_group)
    {
    case 1:
        settings_page(e);
        break;
    case 2:
        information_page(e);
        break;
    case 3:
        advanced_page(e);
        break;
    case 4:
        household_page(e);
        break;
    case 5:
        brightness_page(e);
        break;
    default:
        return_menu(e);
        break;
    }
}
static void settings_page(lv_event_t *e)
{
    (void)e;
    page_base("Ustawienia");
    nav_group = 1;
    back_group = 0;
    const hb_menu_item items[] = {{"Sieć Wi-Fi", settings, (void *)0},
                                  {"Obraz z kamery UBox", settings, (void *)1},
                                  {"Pogoda", settings, (void *)2},
                                  {"Wyświetlacz", brightness_page, NULL},
                                  {"Home Assistant", ha_page, NULL}};
    hb_product_rows(page, items, 5);
    lv_scr_load(page);
}
static void information_page(lv_event_t *e)
{
    (void)e;
    page_base("Informacje");
    nav_group = 2;
    back_group = 0;
    product_info = label(page, "Odczytywanie stanu urządzenia...", LV_ALIGN_TOP_LEFT, 24, 80);
    lv_obj_set_width(product_info, 745);
    for (unsigned i = 0; i < 5; ++i)
    {
        product_states[i] = label(page, "Oczekiwanie", LV_ALIGN_TOP_LEFT, 24, 125 + i * 40);
        lv_obj_set_width(product_states[i], 745);
    }
    button(page, "Diagnostyka zaawansowana", 24, 370, 752, advanced_page, NULL);
    lv_scr_load(page);
}
static void storage_page(lv_event_t *e)
{
    (void)e;
    page_base("Pamięć profili");
    hb_id_status *s = &identity_workspace.page;
    hb_identity_get(s);
    char info[512];
    snprintf(info, sizeof(info),
             "Tryb: %s\nSchemat: 2 | bank: %d | generacja: %llu\nDomownicy: %u | ostatni zapis: "
             "%llu ms od startu\nKod pamięci: %u | dostępna: %s | zapis trwały: %s\n%s",
             s->storage_mode == 2   ? "LOCAL_UNENCRYPTED"
             : s->storage_mode == 1 ? "HARDWARE_PROTECTED"
                                    : "RAM_ONLY",
             s->storage_bank, (unsigned long long)s->storage_generation, s->profile_count,
             (unsigned long long)s->storage_last_save_ms, s->storage_result,
             s->persistence_ready ? "tak" : "nie", s->storage_saved ? "tak" : "nie",
             s->persistence_message);
    lv_obj_t *text = label(page, info, LV_ALIGN_TOP_LEFT, 24, 80);
    lv_obj_set_width(text, 745);
    button(page, "Ponów zapis", 24, 350, 350, identity_send,
           (void *)(uintptr_t)HB_ID_CMD_RETRY_SAVE);
    identity_info = label(page, "", LV_ALIGN_TOP_LEFT, 24, 285);
    lv_scr_load(page);
}
static void advanced_page(lv_event_t *e)
{
    (void)e;
    page_base("Diagnostyka zaawansowana");
    nav_group = 3;
    back_group = 2;
    const hb_menu_item items[] = {
        {"Test obrazu", test_image, NULL},      {"Test powitania", test_greeting, NULL},
        {"Test dotyku", touch_test, NULL},      {"Kamera lokalna", local_camera, NULL},
        {"Obecność w tle", monitor_page, NULL}, {"Jakość twarzy", quality_page, NULL},
        {"System", system_test, NULL},          {"Ocena rozpoznawania", identity_page, (void *)2},
        {"Mikrofon", audio_page, NULL},         {"Połączenie HA", ha_diagnostics_page, NULL},
        {"Pamięć profili", storage_page, NULL}};
    hb_product_rows(page, items, 11);
    lv_scr_load(page);
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
    const hb_menu_item main_items[4] = {{"Domownicy", household_page, NULL},
                                        {"Rozpoznawanie", recognition_page, NULL},
                                        {"Ustawienia", settings_page, NULL},
                                        {"Informacje", information_page, NULL}};
    status = hb_product_home(menu, main_items, close_menu);
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
