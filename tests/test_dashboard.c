// SPDX-License-Identifier: Apache-2.0
#include "hb_dashboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define CHECK(e)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(e))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "line %d: %s\n", __LINE__, #e);                                        \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
int main(void)
{
    hb_transfer_t t;
    CHECK(hb_transfer_begin(&t, 200, 6));
    CHECK(hb_transfer_append(&t, 3));
    CHECK(!hb_transfer_complete(&t));
    CHECK(hb_transfer_append(&t, 3));
    CHECK(hb_transfer_complete(&t));
    CHECK(!hb_transfer_append(&t, 1));
    const int errors[] = {304, 401, 503, 0, -1};
    for (unsigned i = 0; i < sizeof(errors) / sizeof(errors[0]); ++i)
        CHECK(!hb_transfer_begin(&t, errors[i], 8));
    CHECK(!hb_transfer_begin(&t, 200, HB_JPEG_LIMIT + 1));
    CHECK(!hb_transfer_begin(&t, 200, 0));
    CHECK(hb_transfer_begin(&t, 200, -1));
    CHECK(hb_transfer_append(&t, HB_JPEG_LIMIT));
    CHECK(hb_transfer_complete(&t));
    CHECK(!hb_transfer_append(&t, 1));
    char tag[8] = "old";
    CHECK(!hb_etag_commit(tag, sizeof(tag), "new", false, true));
    CHECK(!strcmp(tag, "old"));
    CHECK(!hb_etag_commit(tag, sizeof(tag), "new", true, false));
    CHECK(!strcmp(tag, "old"));
    CHECK(!hb_etag_commit(tag, sizeof(tag), "too-long-tag", true, true));
    CHECK(!strcmp(tag, "old"));
    CHECK(hb_etag_commit(tag, sizeof(tag), "new", true, true));
    CHECK(!strcmp(tag, "new"));
    CHECK(hb_retry_seconds(0, 30) == 30 && hb_retry_seconds(1, 30) == 5 &&
          hb_retry_seconds(2, 30) == 10 && hb_retry_seconds(99, 30) == 300);
    uint16_t w, h;
    const uint8_t jpg[] = {255, 216, 255, 192, 0, 11, 8, 0, 16, 0, 32, 1, 1, 17, 0, 255, 217};
    CHECK(hb_jpeg_bounds(jpg, sizeof(jpg), &w, &h) && w == 32 && h == 16);
    CHECK(!hb_jpeg_bounds(jpg, sizeof(jpg) - 1, &w, &h));
    uint8_t bad[sizeof(jpg)];
    memcpy(bad, jpg, sizeof(jpg));
    bad[3] = 194;
    CHECK(!hb_jpeg_bounds(bad, sizeof(bad), &w, &h));
    bad[3] = 192;
    bad[9] = 0xff;
    CHECK(!hb_jpeg_bounds(bad, sizeof(bad), &w, &h));
    uint16_t source[2] = {0xf800, 0x001f};
    uint16_t *out = malloc(800 * 480 * 2);
    CHECK(out);
    CHECK(hb_scale_cover(source, 2, 1, 2, out));
    CHECK(out[0] == 0xf800 && out[799] == 0x001f && out[479 * 800] == 0xf800);
    CHECK(!hb_scale_cover(source, 2, 1, 1, out));
    free(out);
    int64_t now, stamp;
    CHECK(hb_timestamp("2026-10-22T12:00:00+02:00", &now));
    CHECK(hb_timestamp("2026-10-22T10:00:00Z", &stamp) && stamp == now);
    CHECK(!hb_timestamp("2026-02-30T12:00:00Z", &stamp));
    CHECK(!hb_timestamp("2026-10-22T12:00:00+14:01", &stamp));
    CHECK(!hb_fresh(now + 1, now, 900) && !hb_fresh(now - 901, now, 900) &&
          hb_fresh(now - 900, now, 900));
    hb_temperature_t temperature;
    const char *valid = "{\"temperature\":12.5,\"unit\":\"C\",\"timestamp\":\"2026-10-22T12:00:00+"
                        "02:00\",\"available\":true}";
    CHECK(hb_temperature_parse(valid, strlen(valid), now, 900, &temperature) &&
          temperature.available && temperature.value == 12.5);
    CHECK(!hb_temperature_parse(valid, strlen(valid), now + 901, 900, &temperature));
    CHECK(!hb_temperature_parse(valid, strlen(valid), 0, 900, &temperature));
    const char *invalid[] = {"{", "{}", "{\"available\":true}",
                             "{\"temperature\":\"12\",\"available\":true}",
                             "{\"temperature\":200,\"unit\":\"C\",\"available\":true}"};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        CHECK(!hb_temperature_parse(invalid[i], strlen(invalid[i]), now, 900, &temperature));
    const char *unavailable = "{\"available\":false}";
    CHECK(hb_temperature_parse(unavailable, strlen(unavailable), now, 900, &temperature) &&
          !temperature.available);
    char text[6];
    hb_clock_text(false, now, text);
    CHECK(!strcmp(text, "--:--"));
#ifndef _WIN32
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    CHECK(hb_timestamp("2026-03-29T00:59:00Z", &stamp));
    hb_clock_text(true, stamp, text);
    CHECK(!strcmp(text, "01:59"));
    CHECK(hb_timestamp("2026-03-29T01:00:00Z", &stamp));
    hb_clock_text(true, stamp, text);
    CHECK(!strcmp(text, "03:00"));
    CHECK(hb_timestamp("2026-10-25T00:59:00Z", &stamp));
    hb_clock_text(true, stamp, text);
    CHECK(!strcmp(text, "02:59"));
    CHECK(hb_timestamp("2026-10-25T01:00:00Z", &stamp));
    hb_clock_text(true, stamp, text);
    CHECK(!strcmp(text, "02:00"));
#endif
    puts("dashboard: transfer/errors/ETag rollback/JPEG bounds/cover/temperature/time passed");
    return 0;
}
