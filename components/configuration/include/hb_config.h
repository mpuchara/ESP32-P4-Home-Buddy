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
    uint32_t poll_seconds, legacy_temperature_age;
    // Preserve schema-1 size/offsets without exposing removed MQTT fields.
    uint8_t legacy_transport[481];
} hb_config_t;
typedef struct
{
    uint32_t schema;
    char city[128];
    double latitude, longitude;
    bool has_location;
} hb_weather_config_t;
void hb_config_weather_get(hb_weather_config_t *out);
esp_err_t hb_config_weather_save(const hb_weather_config_t *value);
esp_err_t hb_config_start(void);
void hb_config_get(hb_config_t *out);
bool hb_config_ready(void);
esp_err_t hb_config_save(const hb_config_t *value);
// A Wi-Fi-only update preserves other independently edited settings.
esp_err_t hb_config_save_wifi(const char *ssid, const char *password);
