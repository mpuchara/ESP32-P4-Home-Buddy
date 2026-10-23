// SPDX-License-Identifier: Apache-2.0
// Private settings-page implementation, included by ui.c after its form helpers.
static void save_ha(lv_event_t *e)
{
    (void)e;
    if (critical)
        return;
    config_job job = {.ha = true};
    job.ha_value.schema = 1;
    uint32_t port, cooldown;
    if (strlen(lv_textarea_get_text(field[0])) >= sizeof(job.ha_value.host) ||
        strlen(lv_textarea_get_text(field[2])) >= sizeof(job.ha_value.secret))
    {
        form_error("Pole jest za długie (limit bajtów UTF-8).");
        return;
    }
    if (!number_field(1, 1, 65535, &port) || !number_field(3, 0, 3600, &cooldown))
    {
        form_error("Port: 1-65535; cooldown: 0-3600 s");
        return;
    }
    strlcpy(job.ha_value.host, lv_textarea_get_text(field[0]), sizeof(job.ha_value.host));
    strlcpy(job.ha_value.secret, lv_textarea_get_text(field[2]), sizeof(job.ha_value.secret));
    job.ha_value.port = port;
    job.ha_value.cooldown_seconds = cooldown;
    job.ha_value.enabled = lv_obj_has_state(ha_enable, LV_STATE_CHECKED);
    job.ha_value.send_names = lv_obj_has_state(ha_publish, LV_STATE_CHECKED);
    job.ha_value.tls = lv_obj_has_state(ha_tls, LV_STATE_CHECKED);
    job.ha_value.mode = (hb_ha_mode)lv_dropdown_get_selected(ha_mode);
    if (!hb_ha_config_valid(&job.ha_value, job.ha_value.enabled))
    {
        form_error("Sprawdź host, tryb i sekret. Webhook: 32-128 znaków.");
        memset(&job, 0, sizeof(job));
        return;
    }
    if (xQueueSend(jobs, &job, 0) == pdTRUE)
        critical = true;
    memset(&job, 0, sizeof(job));
}
static lv_obj_t *ha_checkbox(lv_obj_t *parent, const char *text, int y, bool checked)
{
    lv_obj_t *box = lv_checkbox_create(parent);
    lv_checkbox_set_text(box, text);
    lv_obj_set_style_text_color(box, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(box, &hb_font_20, LV_PART_MAIN);
    lv_obj_set_pos(box, 0, y);
    if (checked)
        lv_obj_add_state(box, LV_STATE_CHECKED);
    return box;
}
static void ha_test_action(lv_event_t *e)
{
    hb_ha_test((uintptr_t)lv_event_get_user_data(e) != 0);
    form_error("Test używa zapisanych ustawień. POST ma typ connection_test.");
}
static void ha_page(lv_event_t *e)
{
    (void)e;
    if (!hb_config_ready())
        return;
    page_base("Połączenie z Home Assistant");
    hb_ha_config cfg;
    hb_ha_config_get(&cfg);
    lv_obj_t *fields = lv_obj_create(page);
    lv_obj_set_pos(fields, 12, 70);
    lv_obj_set_size(fields, 776, 226);
    lv_obj_set_style_bg_color(fields, lv_color_hex(0x10171c), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(fields, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(fields, 8, LV_PART_MAIN);
    char number[16];
    field[0] = input(fields, "Host HA (bez http://)", cfg.host, 127, false, 4);
    snprintf(number, sizeof(number), "%u", cfg.port);
    field[1] = input(fields, "Port", number, 5, false, 64);
    field[2] = input(fields, "Webhook ID / token REST", cfg.secret, 512, true, 124);
    snprintf(number, sizeof(number), "%u", (unsigned)cfg.cooldown_seconds);
    field[3] = input(fields, "Cooldown zdarzeń (s)", number, 4, false, 184);
    ha_mode = lv_dropdown_create(fields);
    lv_obj_set_pos(ha_mode, 0, 250);
    lv_obj_set_width(ha_mode, 690);
    lv_obj_set_style_text_color(ha_mode, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ha_mode, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(ha_mode, &hb_font_20, LV_PART_MAIN);
    lv_dropdown_set_options(ha_mode, "Webhook (zalecany)\nREST API (zaawansowany)");
    lv_dropdown_set_selected(ha_mode, cfg.mode);
    ha_enable = ha_checkbox(fields, "Włącz publikację zdarzeń", 310, cfg.enabled);
    ha_publish = ha_checkbox(fields, "Wysyłaj imiona domowników", 350, cfg.send_names);
    ha_tls = ha_checkbox(fields, "HTTPS: weryfikuj CA i nazwę hosta", 390, cfg.tls);
    label(fields, "HTTP i NVS w trybie deweloperskim nie szyfrują sekretu.", LV_ALIGN_TOP_LEFT, 0,
          435);
    button(fields, "Test połączenia", 0, 475, 320, ha_test_action, NULL);
    button(fields, "Wyślij test diagnostyczny", 345, 475, 350, ha_test_action, (void *)1);
    ha_info = label(fields, "", LV_ALIGN_TOP_LEFT, 0, 540);
    lv_obj_set_width(ha_info, 690);
    button(page, "Zapisz", 490, 8, 140, save_ha, NULL);
    memset(&cfg, 0, sizeof(cfg));
    lv_scr_load(page);
}
