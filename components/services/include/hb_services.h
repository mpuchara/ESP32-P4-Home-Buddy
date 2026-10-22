// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include "hb_dashboard.h"
#include "hb_weather.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
bool hb_time_valid(void);
esp_err_t hb_services_start(void);
bool hb_weather_search(const char *query);
