// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include "hb_frame_pool.h"
#include "hb_light_meter.h"
#include <stdbool.h>
#include <stdint.h>
// Read-only identification on the existing board I2C bus. No reset/power GPIO writes.
esp_err_t hb_camera_probe(uint16_t *sensor_id);
typedef struct
{
    const uint16_t *pixels;
    unsigned width, height;
    uint32_t sequence;
    uint64_t timestamp_ms;
    hb_frame_lease lease;
} hb_camera_frame;
typedef struct
{
    uint16_t sensor_id;
    unsigned width, height;
    uint32_t frames, dropped, errors, restarts;
    float fps, exposure_us, analogue_gain, digital_gain;
    uint64_t light_ms;
    hb_light_reading light;
    esp_err_t error;
    bool running;
} hb_camera_status;
esp_err_t hb_camera_start(void);
void hb_camera_get_status(hb_camera_status *status);
bool hb_camera_acquire(hb_camera_frame *frame);
void hb_camera_release(hb_camera_frame *frame);
void hb_camera_set_sample_seconds(uint32_t seconds);
