// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct
{
    float metric, luminance, rejected_fraction;
    bool valid;
} hb_light_reading;
// Robust central ROI; log2(luminance / exposure_us / analogue_gain / digital_gain).
// Linear RGB565 before gamma; metadata must match this exact frame.
hb_light_reading hb_light_measure(const uint16_t *rgb, unsigned width, unsigned height,
                                  float exposure_us, float analogue_gain, float digital_gain,
                                  bool metadata_valid);
