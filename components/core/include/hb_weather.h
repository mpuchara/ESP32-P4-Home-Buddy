// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_dashboard.h"
typedef struct
{
    char label[128];
    double latitude, longitude;
} hb_place_t;
typedef struct
{
    char query[128];
    unsigned count;
    hb_place_t place[5];
} hb_places_t;
bool hb_url_encode(const char *input, char *output, size_t capacity);
bool hb_places_parse(const char *json, size_t n, hb_places_t *out);
bool hb_weather_parse(const char *json, size_t n, int64_t now, hb_temperature_t *out);
