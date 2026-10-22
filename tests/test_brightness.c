// SPDX-License-Identifier: Apache-2.0
#include "hb_brightness.h"
#include "hb_camera_preview.h"
#include "hb_frame_pool.h"
#include "hb_light_meter.h"
#include "hb_raw_frame.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "line %d: %s\n", __LINE__, #x);                                        \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static void controller(void)
{
    hb_brightness_config c = hb_brightness_defaults();
    CHECK(hb_brightness_valid(&c));
    CHECK(!hb_brightness_calibrated(&c));
    CHECK(hb_brightness_target(&c, 1) == 128);
    c.day = (hb_light_anchor){2, 240, 1};
    c.night = (hb_light_anchor){-2, 40, 1};
    c.mode = HB_BRIGHT_AUTO;
    CHECK(hb_brightness_calibrated(&c));
    CHECK(hb_brightness_target(&c, 0) == 140);
    CHECK(hb_brightness_target(&c, 100) == 240);
    CHECK(hb_brightness_target(&c, -100) == 40);
    c.day.metric = -2;
    c.night.metric = 2;
    CHECK(hb_brightness_target(&c, -2) == 240);
    CHECK(hb_brightness_target(&c, 2) == 40);
    c.day.metric = 2;
    CHECK(!hb_brightness_calibrated(&c));
    c.day.metric = NAN;
    CHECK(!hb_brightness_valid(&c));
    c.day.metric = -2;
    c.night.pwm = 0;
    CHECK(!hb_brightness_valid(&c));
    c.night.pwm = 40;
    hb_brightness_state s;
    hb_brightness_init(&s, 128, 0);
    unsigned last = 128;
    for (uint64_t now = 100; now <= 60000; now += 100)
    {
        unsigned pwm = hb_brightness_step(&s, &c, true, -2, (now / 5000) * 5000, now);
        CHECK(pwm >= 26 && pwm <= 255);
        CHECK(abs((int)pwm - last) <= 1);
        last = pwm;
    }
    CHECK(last >= 236 && last <= 240);
    CHECK(hb_brightness_step(&s, &c, false, 0, 0, 60100) == last);
    CHECK(hb_brightness_step(&s, &c, true, 0, 0, 60200) == last); // stale hold
    c.mode = HB_BRIGHT_MANUAL;
    c.manual_pwm = 26;
    CHECK(hb_brightness_step(&s, &c, false, 0, 0, 60300) == 26);
    c.mode = HB_BRIGHT_OVERRIDE;
    c.manual_pwm = 200;
    s.override_until_ms = 65000;
    CHECK(hb_brightness_step(&s, &c, false, 0, 0, 60400) == 200);
    CHECK(hb_brightness_step(&s, &c, false, 0, 0, 64900) == 200);
    CHECK(hb_brightness_step(&s, &c, true, -2, 65000, 65000) <= 201); // bounded resume
    hb_brightness_init(&s, 128, 0);
    c.mode = HB_BRIGHT_AUTO;
    hb_brightness_step(&s, &c, true, 0, 100, 100);
    float before = s.filtered;
    hb_brightness_step(&s, &c, true, 10, 5000, 5000);
    CHECK(s.filtered == before);
    hb_brightness_step(&s, &c, true, 0, 10000, 10000);
    CHECK(s.filtered == before);
    c.day.valid = 0;
    CHECK(hb_brightness_step(&s, &c, true, 0, 10100, 10100) == 128);
    c.schema = 2;
    CHECK(!hb_brightness_valid(&c));
    c = hb_brightness_defaults();
    c.sample_seconds = 0;
    CHECK(!hb_brightness_valid(&c));
}
static void leases(void)
{
    hb_frame_pool p;
    hb_frame_pool_init(&p);
    hb_frame_lease a, b, c;
    CHECK(!hb_frame_acquire(&p, &a));
    int i = hb_frame_reserve(&p);
    CHECK(i == 0);
    CHECK(hb_frame_publish(&p, i));
    CHECK(hb_frame_acquire(&p, &a));
    CHECK(hb_frame_acquire(&p, &b));
    CHECK(hb_frame_release(&p, a));
    CHECK(!hb_frame_release(&p, a));
    CHECK(hb_frame_acquire(&p, &c));
    CHECK(!hb_frame_release(&p, a));
    CHECK(hb_frame_release(&p, b));
    CHECK(hb_frame_release(&p, c));
    CHECK(!hb_frame_release(&p, c));
    hb_frame_lease held[3];
    CHECK(hb_frame_acquire(&p, &held[0]));
    for (int n = 1; n < 3; n++)
    {
        i = hb_frame_reserve(&p);
        CHECK(i >= 0);
        CHECK(hb_frame_publish(&p, i));
        CHECK(hb_frame_acquire(&p, &held[n]));
    }
    CHECK(hb_frame_reserve(&p) == -1);
    CHECK(p.dropped == 1);
    for (int n = 0; n < 3; n++)
        CHECK(hb_frame_release(&p, held[n]));
    i = hb_frame_reserve(&p);
    CHECK(i >= 0);
    CHECK(hb_frame_abort(&p, i));
    CHECK(!hb_frame_publish(&p, i));
    CHECK(!hb_frame_abort(&p, i));
    for (int n = 0; n < 10000; n++)
    {
        i = hb_frame_reserve(&p);
        CHECK(i >= 0);
        CHECK(hb_frame_publish(&p, i));
        CHECK(hb_frame_acquire(&p, &a));
        CHECK(hb_frame_release(&p, a));
    }
}
static void meter(void)
{
    uint16_t pixels[64 * 64];
    for (unsigned i = 0; i < 64 * 64; i++)
        pixels[i] = 0x7bef;
    hb_light_reading a = hb_light_measure(pixels, 64, 64, 1000, 1, 1, true);
    hb_light_reading b = hb_light_measure(pixels, 64, 64, 2000, 1, 1, true);
    CHECK(a.valid && b.valid);
    CHECK(fabsf(a.metric - b.metric - 1) < 0.001f);
    CHECK(!hb_light_measure(pixels, 64, 64, 1000, 1, 1, false).valid);
    CHECK(!hb_light_measure(pixels, 64, 64, NAN, 1, 1, true).valid);
    CHECK(!hb_light_measure(NULL, 64, 64, 1000, 1, 1, true).valid);
    memset(pixels, 255, sizeof(pixels));
    CHECK(!hb_light_measure(pixels, 64, 64, 1000, 1, 1, true).valid);
    memset(pixels, 0, sizeof(pixels));
    CHECK(!hb_light_measure(pixels, 64, 64, 1000, 1, 1, true).valid);
}
static void preview(void)
{
    uint16_t source[64 * 64], output[32 * 32];
    memset(source, 0, sizeof(source));
    CHECK(hb_camera_make_preview(source, 64, 64, output, 32, 32));
    CHECK(output[0] == 0);
    memset(source, 255, sizeof(source));
    CHECK(hb_camera_make_preview(source, 64, 64, output, 32, 32));
    CHECK(output[0] == 0xffff);
    for (unsigned i = 0; i < 64 * 64; i++)
        source[i] = 0x3be7;
    CHECK(hb_camera_make_preview(source, 64, 64, output, 32, 32));
    unsigned r = (output[0] >> 11) & 31, g = (output[0] >> 5) & 63, b = output[0] & 31;
    CHECK(abs((int)r - (int)b) <= 1);
    CHECK(abs((int)(g / 2) - (int)r) <= 1);
    CHECK(source[0] == 0x3be7); // leased source remains linear and untouched
    CHECK(!hb_camera_make_preview(NULL, 64, 64, output, 32, 32));
}
static void raw_frame(void)
{
    uint8_t source[64 * 64];
    uint16_t output[32 * 32];
    for (unsigned y = 0; y < 64; y++)
        for (unsigned x = 0; x < 64; x++)
            source[y * 64 + x] = (y & 1) && !(x & 1) ? 255 : 0;
    CHECK(hb_raw_thumbnail(source, 64, 64, output, 32, 32));
    CHECK(output[0] == 0xf800 && output[1023] == 0xf800);
    memset(source, 128, sizeof(source));
    CHECK(hb_raw_thumbnail(source, 64, 64, output, 32, 32));
    hb_light_reading a = hb_raw_light(source, 64, 64, 1000, 1, 1, true);
    hb_light_reading b = hb_raw_light(source, 64, 64, 2000, 1, 1, true);
    CHECK(a.valid && b.valid && fabsf(a.metric - b.metric - 1) < 0.001f);
    // Four colored quadrants establish CCW rotation without relying on symmetric gray frames.
    for (unsigned y = 0; y < 64; y++)
        for (unsigned x = 0; x < 64; x++)
        {
            unsigned q = (y >= 32) * 2 + (x >= 32);
            bool red = (y & 1) && !(x & 1), blue = !(y & 1) && (x & 1);
            source[y * 64 + x] = q == 0   ? (red ? 255 : 16)
                                 : q == 1 ? (blue ? 255 : 16)
                                 : q == 2 ? 255
                                          : 16;
        }
    CHECK(hb_raw_thumbnail_upright(source, 64, 64, output, 32, 32));
    CHECK(output[0] == 0x001f); // native top right -> upright top left
    CHECK(output[31] == 0);     // native bottom right -> upright top right
    CHECK(output[31 * 32] == 0xf800);
    CHECK(output[1023] == 0xffff);
    CHECK(source[0] == 16 && source[64] == 255); // source Bayer phase untouched
    memset(source, 22, sizeof(source));
    CHECK(hb_raw_light(source, 64, 64, 1000, 1, 1, true).valid); // dim signal survives
    memset(source, 16, sizeof(source));
    CHECK(!hb_raw_light(source, 64, 64, 1000, 1, 1, true).valid); // black target alone
    CHECK(!hb_raw_thumbnail(source, 63, 64, output, 32, 32));
}
int main(void)
{
    controller();
    leases();
    meter();
    preview();
    raw_frame();
    puts("brightness, bounded frame leases, robust exposure-compensated meter: OK");
    return 0;
}
