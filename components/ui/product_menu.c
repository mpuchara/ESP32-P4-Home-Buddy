// SPDX-License-Identifier: Apache-2.0
#include "product_menu.h"
static void item(lv_obj_t *parent, const hb_menu_item *item, int x, int y, int w, int h)
{
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_radius(b, 16, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x243a47), 0);
    lv_obj_t *text = lv_label_create(b);
    lv_label_set_text(text, item->title);
    lv_obj_center(text);
    lv_obj_add_event_cb(b, item->action, LV_EVENT_CLICKED, item->context);
}
void hb_product_cards(lv_obj_t *parent, const hb_menu_item items[4])
{
    for (unsigned i = 0; i < 4; ++i)
        item(parent, &items[i], 24 + (i % 2) * 388, 78 + (i / 2) * 146, 364, 126);
}
lv_obj_t *hb_product_home(lv_obj_t *parent, const hb_menu_item items[4], lv_event_cb_t back)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x10171c), 0);
    lv_obj_set_style_text_color(parent, lv_color_white(), 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    hb_product_cards(parent, items);
    const hb_menu_item photo = {"Wróć do zdjęcia", back, NULL};
    item(parent, &photo, 500, 410, 276, 48);
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Home Buddy");
    lv_obj_set_pos(title, 24, 20);
    lv_obj_t *state = lv_label_create(parent);
    lv_label_set_text(state, "Oczekiwanie");
    lv_obj_set_pos(state, 550, 20);
    lv_obj_set_width(state, 226);
    return state;
}
void hb_product_rows(lv_obj_t *parent, const hb_menu_item *items, unsigned count)
{
    if (count > 10)
        return;
    for (unsigned i = 0; i < count; ++i)
        item(parent, &items[i], count > 5 ? 24 + (i % 2) * 388 : 24,
             70 + (count > 5 ? i / 2 : i) * 68, count > 5 ? 364 : 752, 52);
}
bool hb_product_timeout(uint32_t now, uint32_t touched, bool busy, bool editing, bool enrollment,
                        bool confirmation)
{
    return !busy && !editing && !enrollment && !confirmation && (uint32_t)(now - touched) >= 30000;
}
