// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_light_meter.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
bool hb_raw_thumbnail(const uint8_t *source, unsigned width, unsigned height, uint16_t *target,
                      unsigned out_width, unsigned out_height);
// Fixed module mounting: head points right in sensor coordinates; upright is CCW90.
bool hb_raw_thumbnail_upright(const uint8_t *source, unsigned width, unsigned height,
                              uint16_t *target, unsigned out_width, unsigned out_height);
hb_light_reading hb_raw_light(const uint8_t *source, unsigned width, unsigned height,
                              float exposure_us, float analogue_gain, float digital_gain,
                              bool metadata);

// ISP RGB565 display/AI path; preserve the legacy Bayer coordinate convention.
bool hb_rgb_thumbnail_upright(const uint8_t *source, unsigned width, unsigned height,
                              uint16_t *target, unsigned out_width, unsigned out_height);

// Sensor black pedestal removed before hardware gamma; input/output in [0, 255].
uint32_t hb_isp_gamma_black_corrected(uint32_t x);

// Initial fixed post-CCM white-balance curves; preserve black and white endpoints.
uint32_t hb_isp_gamma_red(uint32_t x);
uint32_t hb_isp_gamma_blue(uint32_t x);

// ESP-IDF RGB888 native packed bytes are B,G,R. Convert only at consumer boundaries.
typedef struct
{
    float mean, contrast, detail, clipped;
    unsigned width, height;
    bool valid;
} hb_rgb888_stats;
bool hb_rgb888_thumbnail(const uint8_t *src, unsigned w, unsigned h, uint8_t *dst, unsigned ow,
                         unsigned oh);
bool hb_rgb888_preview(const uint8_t *src, unsigned w, unsigned h, uint16_t *dst, unsigned ow,
                       unsigned oh);
bool hb_rgb888_crop(const uint8_t *src, unsigned w, unsigned h, unsigned x, unsigned y, unsigned cw,
                    unsigned ch, uint8_t *dst, size_t capacity);
hb_rgb888_stats hb_rgb888_quality(const uint8_t *src, unsigned w, unsigned h, unsigned x,
                                  unsigned y, unsigned cw, unsigned ch);
float hb_rgb888_rank(hb_rgb888_stats q);
bool hb_rgb888_neutral(const uint8_t *src, unsigned w, unsigned h, float *red, float *blue);
uint32_t hb_isp_gamma_gain(uint32_t x, float gain);
void hb_camera_exposure_plan(float line_us, unsigned max_lines, unsigned old_lines,
                             unsigned old_gain, float luma, bool face, unsigned *lines,
                             unsigned *gain);

// Bounded downsample of an upright native RGB888 crop; RGB output for ESP-DL.
bool hb_rgb888_crop_scaled(const uint8_t *src, unsigned w, unsigned h, unsigned x, unsigned y,
                           unsigned cw, unsigned ch, uint8_t *dst, unsigned ow, unsigned oh,
                           size_t capacity);
