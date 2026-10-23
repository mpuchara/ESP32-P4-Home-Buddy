// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct
{
    uint32_t width, height, crop_x, crop_y;
} hb_cover_t;
// Scaled dimensions are rounded up, so all target pixels are covered.
bool hb_cover(uint32_t sw, uint32_t sh, uint32_t tw, uint32_t th, hb_cover_t *out);
bool hb_touch_landscape(int px, int py, int *x, int *y);
