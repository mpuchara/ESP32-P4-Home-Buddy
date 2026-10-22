// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_light_meter.h"
#include <stdbool.h>
#include <stdint.h>
bool hb_raw_thumbnail(const uint8_t *source, unsigned width, unsigned height, uint16_t *target,
                      unsigned out_width, unsigned out_height);
// Fixed module mounting: head points right in sensor coordinates; upright is CCW90.
bool hb_raw_thumbnail_upright(const uint8_t *source, unsigned width, unsigned height,
                              uint16_t *target, unsigned out_width, unsigned out_height);
hb_light_reading hb_raw_light(const uint8_t *source, unsigned width, unsigned height,
                              float exposure_us, float analogue_gain, float digital_gain,
                              bool metadata);
