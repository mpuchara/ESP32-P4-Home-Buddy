// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
void hb_wifi_disconnect(void);
void hb_wifi_networks(char *out, size_t size);
esp_err_t hb_wifi_start(void);
bool hb_wifi_request_scan(void);
