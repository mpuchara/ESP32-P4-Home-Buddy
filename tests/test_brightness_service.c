// SPDX-License-Identifier: Apache-2.0
// Exercise the real service with deterministic OS/clock/NVS boundaries, never device data.
#include "hb_brightness_service.h"
#include "hb_camera.h"
#include "platform.h"
#include <math.h>
#include <setjmp.h>
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
static uint8_t blob[sizeof(hb_brightness_config) + 1], pending[sizeof(blob)];
static size_t blob_size, pending_size;
static unsigned commits, writes, opens, interval;
static bool fail_commit, fail_open, fail_set;
static uint64_t clock_ms;
static uint8_t output;
static jmp_buf yield;
static void (*service_task)(void *);
SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    return &output;
}
int xSemaphoreTake(SemaphoreHandle_t m, uint32_t t)
{
    (void)m;
    (void)t;
    return 1;
}
int xSemaphoreGive(SemaphoreHandle_t m)
{
    (void)m;
    return 1;
}
int xTaskCreate(void (*task)(void *), const char *name, unsigned stack, void *arg,
                unsigned priority, void *handle)
{
    (void)name;
    (void)stack;
    (void)arg;
    (void)priority;
    (void)handle;
    service_task = task;
    return pdPASS;
}
void vTaskDelay(uint32_t ticks)
{
    CHECK(ticks == 100);
    longjmp(yield, 1);
}
int64_t esp_timer_get_time(void)
{
    return (int64_t)clock_ms * 1000;
}
void board_p4_set_brightness(uint8_t pwm)
{
    output = pwm;
    CHECK(pwm >= 26);
}
void hb_camera_get_status(hb_camera_status *camera)
{
    *camera = (hb_camera_status){
        .running = true, .light_ms = clock_ms, .light = {.valid = true, .metric = 0}};
}
void hb_camera_set_sample_seconds(uint32_t seconds)
{
    interval = seconds;
}
esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle)
{
    CHECK(!strcmp(name, "hb_display"));
    (void)mode;
    if (fail_open)
        return ESP_FAIL;
    opens++;
    pending_size = 0;
    *handle = 1;
    return ESP_OK;
}
esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *value, size_t *size)
{
    CHECK(handle == 1 && !strcmp(key, "brightness"));
    if (!blob_size)
        return ESP_ERR_NVS_NOT_FOUND;
    if (*size < blob_size)
    {
        *size = blob_size;
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    memcpy(value, blob, blob_size);
    *size = blob_size;
    return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *value, size_t size)
{
    CHECK(handle == 1 && !strcmp(key, "brightness") && size <= sizeof(pending));
    if (fail_set)
        return ESP_FAIL;
    memcpy(pending, value, size);
    pending_size = size;
    writes++;
    return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t handle)
{
    CHECK(handle == 1);
    if (fail_commit)
        return ESP_FAIL;
    memcpy(blob, pending, pending_size);
    blob_size = pending_size;
    commits++;
    return ESP_OK;
}
void nvs_close(nvs_handle_t handle)
{
    CHECK(handle == 1);
    pending_size = 0;
}
static void tick(uint64_t time)
{
    clock_ms = time;
    if (!setjmp(yield))
        service_task(NULL);
}
static hb_brightness_config current(void)
{
    hb_brightness_config c;
    hb_brightness_get(&c, NULL);
    return c;
}
static void install(const hb_brightness_config *c)
{
    memcpy(blob, c, sizeof(*c));
    blob_size = sizeof(*c);
}
int main(void)
{
    CHECK(hb_brightness_start(true) == ESP_OK);
    hb_brightness_config c = current();
    CHECK(c.mode == HB_BRIGHT_MANUAL && c.manual_pwm == 128);
    c.day = (hb_light_anchor){2, 230, 1};
    c.night = (hb_light_anchor){-2, 30, 1};
    c.mode = HB_BRIGHT_AUTO;
    c.sample_seconds = 10;
    CHECK(hb_brightness_save(&c) == ESP_OK && commits == 1);
    CHECK(hb_brightness_start(true) == ESP_OK);
    c = current();
    CHECK(c.day.metric == 2 && c.night.pwm == 30 && c.mode == HB_BRIGHT_AUTO);
    tick(100);
    CHECK(interval == 10);
    unsigned saved_writes = writes;
    for (unsigned i = 2; i < 100; i++)
        tick(i * 100);
    CHECK(writes == saved_writes); // controller ticks never write flash
    c.mode = HB_BRIGHT_OVERRIDE;
    c.manual_pwm = 200;
    clock_ms = 10000;
    CHECK(hb_brightness_begin_override(&c) == ESP_OK);
    tick(10000);
    CHECK(output == 200);
    // Live edits and a save preserve the original deadline, including near its end.
    clock_ms = 300000;
    c.manual_pwm = 180;
    CHECK(hb_brightness_apply(&c) == ESP_OK);
    CHECK(hb_brightness_save(&c) == ESP_OK);
    tick(309999);
    CHECK(output == 180);
    tick(310000);
    CHECK(current().mode == HB_BRIGHT_AUTO);
    CHECK(hb_brightness_apply(&c) == ESP_OK && current().mode == HB_BRIGHT_AUTO);
    CHECK(hb_brightness_save(&c) == ESP_OK && current().mode == HB_BRIGHT_AUTO);
    // Only explicitly selecting temporary mode again can start another interval.
    CHECK(hb_brightness_begin_override(&c) == ESP_OK);
    tick(310100);
    CHECK(output == 180);
    CHECK(hb_brightness_start(true) == ESP_OK && current().mode == HB_BRIGHT_AUTO);
    // Even a legacy raw OVERRIDE record cannot renew an interval across reboot.
    install(&c);
    CHECK(hb_brightness_start(true) == ESP_OK && current().mode == HB_BRIGHT_AUTO);
    hb_brightness_config stable = current(), edit = stable;
    edit.manual_pwm = 77;
    fail_commit = true;
    CHECK(hb_brightness_save(&edit) == ESP_FAIL);
    CHECK(current().manual_pwm == stable.manual_pwm);
    fail_commit = false;
    // NVS commit errors do not prove rollback on real hardware; test only RAM admission.
    fail_open = true;
    CHECK(hb_brightness_save(&edit) == ESP_FAIL && current().manual_pwm == stable.manual_pwm);
    fail_open = false;
    fail_set = true;
    CHECK(hb_brightness_save(&edit) == ESP_FAIL && current().manual_pwm == stable.manual_pwm);
    fail_set = false;
    // Invalid/short/oversized records fall back without rewriting or erasing the blob.
    unsigned before = writes;
    c.schema = 99;
    install(&c);
    CHECK(hb_brightness_start(true) == ESP_OK);
    CHECK(current().manual_pwm == 128 && blob_size == sizeof(c) && writes == before);
    c = stable;
    c.day.metric = NAN;
    install(&c);
    CHECK(hb_brightness_start(true) == ESP_OK);
    CHECK(!current().day.valid && writes == before);
    blob_size = sizeof(c) - 1;
    CHECK(hb_brightness_start(true) == ESP_OK && !current().day.valid);
    blob_size = sizeof(c) + 1;
    CHECK(hb_brightness_start(true) == ESP_OK && !current().day.valid);
    CHECK(writes == before && blob_size == sizeof(c) + 1);
    unsigned before_open = opens;
    CHECK(hb_brightness_start(false) == ESP_OK);
    c = current();
    c.manual_pwm = 26;
    CHECK(hb_brightness_apply(&c) == ESP_OK);
    tick(320000);
    CHECK(output == 26);
    CHECK(hb_brightness_save(&c) == ESP_ERR_INVALID_STATE && opens == before_open);
    CHECK(hb_brightness_begin_override(NULL) == ESP_ERR_INVALID_ARG);
    CHECK(hb_brightness_begin_override(&c) == ESP_ERR_INVALID_ARG);
    puts("brightness service: restart, NVS failures, corrupt records, explicit override expiry OK");
    return 0;
}
