// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct
{
    uint32_t schema;
    char host[128], username[65], password[129];
    uint16_t port;
    bool enabled, publish_identity, tls;
    uint32_t cooldown_seconds;
} hb_mqtt_config;
void hb_mqtt_config_get(hb_mqtt_config *out);
esp_err_t hb_mqtt_config_save(const hb_mqtt_config *value);
esp_err_t hb_mqtt_start(void);
bool hb_mqtt_connected(void);
