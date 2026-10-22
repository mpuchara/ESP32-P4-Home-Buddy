// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct
{
    uint32_t schema;
    char ssid[33], wifi_password[65], ubox_host[128], ubox_token[128];
    uint16_t ubox_port;
    uint32_t poll_seconds, temperature_age;
    char mqtt_uri[160], mqtt_user[65], mqtt_password[128], temperature_topic[128];
} hb_config_t;
esp_err_t hb_config_start(void);
void hb_config_get(hb_config_t *out);
bool hb_config_has_pin(void);
bool hb_config_ready(void);
esp_err_t hb_config_unlock(const char *pin);
esp_err_t hb_config_save(const hb_config_t *value, const char *first_pin);
// Operator-requested exception: change only Wi-Fi fields, preserving protected settings/PIN.
esp_err_t hb_config_save_wifi(const char *ssid, const char *password);
