// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>
esp_err_t hb_image_decode(const uint8_t *jpeg, size_t bytes, uint16_t **pixels);
