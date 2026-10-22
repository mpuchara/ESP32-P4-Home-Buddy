// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include "hb_brightness.h"
esp_err_t hb_brightness_start(bool storage_available);
void hb_brightness_get(hb_brightness_config *config, uint8_t *pwm);
esp_err_t hb_brightness_apply(const hb_brightness_config *config);
// Worker-only NVS transaction. Existing hb_config/weather records are untouched.
esp_err_t hb_brightness_save(const hb_brightness_config *config);
