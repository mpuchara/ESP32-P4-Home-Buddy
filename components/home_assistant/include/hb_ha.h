// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include "hb_ha_core.h"
typedef struct
{
    hb_ha_result result;
    int http_status;
    uint32_t sent, failed, expired;
} hb_ha_status;
void hb_ha_config_get(hb_ha_config *out);
esp_err_t hb_ha_config_save(const hb_ha_config *value);
esp_err_t hb_ha_start(void);
void hb_ha_get_status(hb_ha_status *out);
// Saved config only. POST is explicit and uses connection_test, never recognized.
void hb_ha_test(bool send_diagnostic_event);
