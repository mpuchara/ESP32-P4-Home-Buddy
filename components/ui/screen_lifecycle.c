// SPDX-License-Identifier: Apache-2.0
#include "screen_lifecycle.h"
void hb_screen_delete(lv_obj_t *screen, lv_obj_t *fallback)
{
    if (!screen || screen == fallback)
        return;
    // LVGL 8.3.11 screen loading sends unload events to the previous active screen.
    // Never leave act_scr NULL by deleting it before the next load.
    if (lv_scr_act() == screen)
        lv_scr_load(fallback);
    lv_obj_del(screen);
}
