// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef enum
{
    HB_BRIGHT_AUTO,
    HB_BRIGHT_MANUAL,
    HB_BRIGHT_OVERRIDE
} hb_bright_mode;
typedef struct
{
    float metric;
    uint32_t pwm;
    uint32_t valid;
} hb_light_anchor;
typedef struct
{
    uint32_t schema, mode, manual_pwm, fallback_pwm;
    uint32_t sample_seconds, transition_seconds, override_seconds;
    hb_light_anchor day, night;
} hb_brightness_config;
typedef struct
{
    float filtered, candidate, output;
    uint32_t confirmations;
    uint64_t updated_ms, sample_ms, override_until_ms;
    bool initialized;
} hb_brightness_state;
hb_brightness_config hb_brightness_defaults(void);
bool hb_brightness_valid(const hb_brightness_config *config);
bool hb_brightness_calibrated(const hb_brightness_config *config);
uint8_t hb_brightness_target(const hb_brightness_config *config, float metric);
void hb_brightness_init(hb_brightness_state *state, uint8_t pwm, uint64_t now_ms);
// One owner calls this at >=100 ms intervals. Invalid/stale samples hold the last safe level.
uint8_t hb_brightness_step(hb_brightness_state *state, const hb_brightness_config *config,
                           bool valid_sample, float metric, uint64_t sample_ms, uint64_t now_ms);
