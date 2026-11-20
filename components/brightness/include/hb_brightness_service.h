// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include "hb_brightness.h"
esp_err_t hb_brightness_start(bool storage_available);
void hb_brightness_get(hb_brightness_config *config, uint8_t *pwm);
// Apply live edits without starting or extending a temporary override.
esp_err_t hb_brightness_apply(const hb_brightness_config *config);
// Only an explicit user mode selection starts/restarts the override deadline.
esp_err_t hb_brightness_begin_override(const hb_brightness_config *config);
// Worker-only NVS transaction. Existing hb_config/weather records are untouched.
esp_err_t hb_brightness_save(const hb_brightness_config *config);
