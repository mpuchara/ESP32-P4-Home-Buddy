// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_config.h"
typedef enum
{
    CMD_IMAGE,
    CMD_GREETING,
    CMD_DIAGNOSTIC,
    CMD_TEMPERATURE,
    CMD_FORM,
    CMD_SAVED,
    CMD_CONFIG_ERROR
} command_type;
typedef struct
{
    command_type type;
    uint16_t *pixels;
    char text[128];
    double temperature;
    int64_t timestamp;
    uint32_t max_age;
    bool available;
} command;
typedef struct
{
    bool save;
    bool wifi;
    char pin[13];
    hb_config_t value;
} config_job;
