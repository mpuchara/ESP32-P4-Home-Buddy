// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_brightness.h"
#include "hb_config.h"
#include "hb_ha.h"
typedef enum
{
    CMD_IMAGE,
    CMD_GREETING,
    CMD_DIAGNOSTIC,
    CMD_TEMPERATURE,
    CMD_PLACES,
    CMD_SAVED,
    CMD_CONFIG_ERROR
} command_type;
typedef struct
{
    command_type type;
    uint16_t *pixels;
    hb_places_t *places;
    char text[128];
    double temperature;
    int64_t timestamp;
    uint32_t max_age;
    bool available;
} command;
typedef struct
{
    bool wifi;
    bool weather;
    bool brightness;
    bool ha;
    hb_ha_config ha_value;
    hb_brightness_config brightness_value;
    hb_weather_config_t weather_value;
    hb_config_t value;
} config_job;
