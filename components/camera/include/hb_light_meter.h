// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct
{
    float metric, luminance, rejected_fraction;
    bool valid;
    bool below_range; // Dark threshold, not a resolved illumination measurement.
} hb_light_reading;
// Robust central ROI; log2(luminance / exposure_us / analogue_gain / digital_gain).
// Linear RGB565 before gamma; metadata must match this exact frame.
hb_light_reading hb_light_measure(const uint16_t *rgb, unsigned width, unsigned height,
                                  float exposure_us, float analogue_gain, float digital_gain,
                                  bool metadata_valid);
hb_light_reading hb_light_histogram(const unsigned histogram[256], float exposure_us,
                                    float analogue_gain, float digital_gain, bool metadata_valid);

// A dark frame is usable as a bounded reference only at settled configured AE limits.
hb_light_reading hb_light_apply_dark_limit(hb_light_reading reading, float exposure_us,
                                           float analogue_gain, float digital_gain,
                                           bool settled_at_limit);

// Relative ambient estimate from gamma-corrected ISP BGR888, not calibrated lux.
hb_light_reading hb_light_measure_isp(const uint8_t *bgr, unsigned width, unsigned height,
                                      float exposure_us, float analogue_gain, float digital_gain,
                                      bool metadata_valid);

// A measurement window publishes only after three distinct, consistent frames.
typedef struct
{
    float candidate;
    uint64_t last_ms;
    unsigned count;
} hb_light_confirmation;
bool hb_light_confirm(hb_light_confirmation *state, hb_light_reading reading, uint64_t frame_ms);
#define HB_LIGHT_WINDOW_MS 20000U
