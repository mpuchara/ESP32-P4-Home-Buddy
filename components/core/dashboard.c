// SPDX-License-Identifier: Apache-2.0
#include "hb_dashboard.h"
#include "hb_geometry.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

bool hb_transfer_begin(hb_transfer_t *t, int status, int64_t length)
{
    if (!t)
        return false;
    *t = (hb_transfer_t){0};
    if (status != 200 || length == 0 || length < -1 || length > HB_JPEG_LIMIT)
    {
        t->failed = true;
        return false;
    }
    t->unknown_length = length == -1;
    t->expected = length > 0 ? (size_t)length : 0;
    return true;
}
bool hb_transfer_append(hb_transfer_t *t, size_t length)
{
    if (!t || t->failed || length > HB_JPEG_LIMIT - t->received ||
        (!t->unknown_length && length > t->expected - t->received))
    {
        if (t)
            t->failed = true;
        return false;
    }
    t->received += length;
    return true;
}
bool hb_transfer_complete(const hb_transfer_t *t)
{
    return t && !t->failed && t->received > 0 && (t->unknown_length || t->received == t->expected);
}
bool hb_jpeg_bounds(const uint8_t *d, size_t n, uint16_t *w, uint16_t *h)
{
    if (!d || !w || !h || n < 10 || n > HB_JPEG_LIMIT || d[0] != 255 || d[1] != 216 ||
        d[n - 2] != 255 || d[n - 1] != 217)
        return false;
    size_t p = 2;
    while (p + 4 <= n)
    {
        if (d[p++] != 255)
            return false;
        while (p < n && d[p] == 255)
            ++p;
        if (p >= n)
            return false;
        unsigned marker = d[p++];
        if (marker == 0xda || marker == 0xd9)
            return false;
        if (marker == 0x01 || (marker >= 0xd0 && marker <= 0xd7))
            continue;
        if (p + 2 > n)
            return false;
        size_t len = ((size_t)d[p] << 8) | d[p + 1];
        if (len < 2 || len > n - p)
            return false;
        if (marker == 0xc0)
        {
            if (len < 8 || d[p + 2] != 8)
                return false;
            *h = (uint16_t)((d[p + 3] << 8) | d[p + 4]);
            *w = (uint16_t)((d[p + 5] << 8) | d[p + 6]);
            return *w && *h && (uint32_t)*w * *h <= HB_PIXEL_LIMIT && *w <= 2048 && *h <= 2048;
        }
        if (marker >= 0xc1 && marker <= 0xcf && marker != 0xc4 && marker != 0xc8 && marker != 0xcc)
            return false;
        p += len;
    }
    return false;
}
bool hb_scale_cover(const uint16_t *src, uint32_t sw, uint32_t sh, uint32_t stride, uint16_t *dst)
{
    hb_cover_t c;
    if (!src || !dst || stride < sw || !hb_cover(sw, sh, 800, 480, &c))
        return false;
    for (uint32_t y = 0; y < 480; ++y)
    {
        uint32_t sy = (uint32_t)(((uint64_t)(y + c.crop_y) * sh) / c.height);
        for (uint32_t x = 0; x < 800; ++x)
        {
            uint32_t sx = (uint32_t)(((uint64_t)(x + c.crop_x) * sw) / c.width);
            dst[y * 800 + x] = src[(size_t)sy * stride + sx];
        }
    }
    return true;
}
uint32_t hb_retry_seconds(unsigned failures, uint32_t normal)
{
    if (!failures)
        return normal;
    if (failures > 7)
        failures = 7;
    uint32_t delay = 5u << (failures - 1);
    return delay > 300 ? 300 : delay;
}
bool hb_etag_commit(char *stored, size_t capacity, const char *candidate, bool decoded,
                    bool accepted)
{
    if (!stored || !candidate || !decoded || !accepted || strlen(candidate) >= capacity)
        return false;
    memcpy(stored, candidate, strlen(candidate) + 1);
    return true;
}
static int number(const char *s, unsigned n)
{
    int v = 0;
    for (unsigned i = 0; i < n; ++i)
    {
        if (s[i] < '0' || s[i] > '9')
            return -1;
        v = v * 10 + s[i] - '0';
    }
    return v;
}
bool hb_timestamp(const char *s, int64_t *epoch)
{
    if (!s || !epoch)
        return false;
    size_t n = strlen(s);
    if ((n != 20 && n != 25) || s[4] != '-' || s[7] != '-' || s[10] != 'T' || s[13] != ':' ||
        s[16] != ':')
        return false;
    int y = number(s, 4), m = number(s + 5, 2), d = number(s + 8, 2), h = number(s + 11, 2),
        mi = number(s + 14, 2), sec = number(s + 17, 2);
    if (y < 2020 || y > 2099 || m < 1 || m > 12 || d < 1 || h < 0 || h > 23 || mi < 0 || mi > 59 ||
        sec < 0 || sec > 59)
        return false;
    static const int md[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool leap = y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
    if (d > md[m - 1] + (m == 2 && leap))
        return false;
    int offset = 0;
    if (n == 20)
    {
        if (s[19] != 'Z')
            return false;
    }
    else
    {
        int oh = number(s + 20, 2), om = number(s + 23, 2);
        if ((s[19] != '+' && s[19] != '-') || s[22] != ':' || oh < 0 || oh > 14 || om < 0 ||
            om > 59 || (oh == 14 && om))
            return false;
        offset = (oh * 60 + om) * 60 * (s[19] == '+' ? 1 : -1);
    }
    int64_t days = 0;
    for (int a = 1970; a < y; ++a)
        days += 365 + (a % 4 == 0 && (a % 100 != 0 || a % 400 == 0));
    for (int a = 1; a < m; ++a)
        days += md[a - 1] + (a == 2 && leap);
    *epoch = (days + d - 1) * 86400 + h * 3600 + mi * 60 + sec - offset;
    return true;
}
bool hb_fresh(int64_t t, int64_t now, uint32_t age)
{
    return t > 0 && now >= t && now - t <= age;
}
void hb_clock_text(bool valid, int64_t epoch, char text[6])
{
    if (!valid)
    {
        memcpy(text, "--:--", 6);
        return;
    }
    time_t t = (time_t)epoch;
    struct tm local;
#ifdef _WIN32
    if (localtime_s(&local, &t) != 0)
    {
        memcpy(text, "--:--", 6);
        return;
    }
#else
    if (!localtime_r(&t, &local))
    {
        memcpy(text, "--:--", 6);
        return;
    }
#endif
    strftime(text, 6, "%H:%M", &local);
}
