// SPDX-License-Identifier: Apache-2.0
#include "board_p4.h"
#include "esp_log.h"
#include "hb_board.h"

esp_err_t hb_board_start(void)
{
    esp_err_t err = board_p4_display_init();
    if (err != ESP_OK)
        return err;
    board_p4_set_brightness(128);
    err = board_p4_touch_init();
    if (err != ESP_OK)
    {
        ESP_LOGE("board", "GT911 unavailable: %s; display remains active", esp_err_to_name(err));
    }
    return ESP_OK;
}
