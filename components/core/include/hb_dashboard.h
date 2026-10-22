// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define HB_JPEG_LIMIT (2u * 1024u * 1024u)
#define HB_PIXEL_LIMIT (2048u * 2048u)
typedef struct
{
    bool available;
    double value;
    int64_t timestamp;
} hb_temperature_t;
bool hb_etag_commit(char *stored, size_t capacity, const char *candidate, bool decoded,
                    bool accepted);
typedef struct
{
    size_t received, expected;
    bool unknown_length, failed;
} hb_transfer_t;
bool hb_transfer_begin(hb_transfer_t *t, int status, int64_t length);
bool hb_transfer_append(hb_transfer_t *t, size_t length);
bool hb_transfer_complete(const hb_transfer_t *t);
bool hb_jpeg_bounds(const uint8_t *data, size_t n, uint16_t *w, uint16_t *h);
bool hb_scale_cover(const uint16_t *src, uint32_t sw, uint32_t sh, uint32_t stride, uint16_t *dst);
uint32_t hb_retry_seconds(unsigned failures, uint32_t normal);
bool hb_timestamp(const char *text, int64_t *epoch);
bool hb_fresh(int64_t measurement, int64_t now, uint32_t age);
void hb_clock_text(bool synchronized, int64_t epoch, char text[6]);
