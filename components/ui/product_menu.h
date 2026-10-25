// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct
{
    const char *title;
    lv_event_cb_t action;
    void *context;
} hb_menu_item;
// Shared by firmware and the real LVGL host renderer. All targets are >=48 px high.
void hb_product_cards(lv_obj_t *parent, const hb_menu_item items[4]);
lv_obj_t *hb_product_home(lv_obj_t *parent, const hb_menu_item items[4], lv_event_cb_t back);
void hb_product_rows(lv_obj_t *parent, const hb_menu_item *items, unsigned count);
bool hb_product_timeout(uint32_t now, uint32_t touched, bool busy, bool editing, bool enrollment,
                        bool confirmation);
