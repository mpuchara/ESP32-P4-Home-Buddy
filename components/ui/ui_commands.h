// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_config.h"
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
    hb_weather_config_t weather_value;
    hb_config_t value;
} config_job;
