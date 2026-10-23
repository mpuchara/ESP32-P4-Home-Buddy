// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include "hb_weather.h"
#include <stdbool.h>
#include <stdint.h>
// On success ownership moves to UI; on failure caller must free pixels.
bool hb_ui_publish_image(uint16_t *pixels);
bool hb_ui_image_is_test(void);
bool hb_ui_show_greeting(const char *display_name);
void hb_ui_diagnostic(const char *service, const char *message);
void hb_ui_temperature(bool available, double value, int64_t timestamp, uint32_t max_age);
esp_err_t hb_ui_start(void);
void hb_ui_weather_places(const hb_places_t *places);
