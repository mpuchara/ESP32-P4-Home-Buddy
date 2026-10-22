// SPDX-License-Identifier: Apache-2.0
#include "board_p4.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hb_brightness_service.h"
#include "hb_camera.h"
#include "nvs.h"
static SemaphoreHandle_t mutex;
static hb_brightness_config config;
static hb_brightness_state state;
static bool storage;
static uint64_t now_ms(void)
{
    return (uint64_t)esp_timer_get_time() / 1000;
}
static void worker(void *arg)
{
    (void)arg;
    for (;;)
    {
        hb_camera_status camera;
        hb_camera_get_status(&camera);
        xSemaphoreTake(mutex, portMAX_DELAY);
        if (config.mode == HB_BRIGHT_OVERRIDE && now_ms() >= state.override_until_ms)
            config.mode = HB_BRIGHT_AUTO;
        uint8_t pwm = hb_brightness_step(&state, &config, camera.running && camera.light.valid,
                                         camera.light.metric, camera.light_ms, now_ms());
        hb_camera_set_sample_seconds(config.sample_seconds);
        xSemaphoreGive(mutex);
        board_p4_set_brightness(pwm);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
void hb_brightness_get(hb_brightness_config *out, uint8_t *pwm)
{
    if (!mutex)
    {
        if (out)
            *out = hb_brightness_defaults();
        if (pwm)
            *pwm = 128;
        return;
    }
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (out)
        *out = config;
    if (pwm)
        *pwm = (uint8_t)state.output;
    xSemaphoreGive(mutex);
}
static esp_err_t apply(const hb_brightness_config *value, bool begin_override)
{
    if (!hb_brightness_valid(value))
        return ESP_ERR_INVALID_ARG;
    if (!mutex)
        return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(mutex, portMAX_DELAY);
    config = *value;
    if (config.mode == HB_BRIGHT_OVERRIDE)
    {
        if (begin_override)
            state.override_until_ms = now_ms() + (uint64_t)config.override_seconds * 1000;
        else if (now_ms() >= state.override_until_ms)
            config.mode = HB_BRIGHT_AUTO;
    }
    else
        state.override_until_ms = 0;
    xSemaphoreGive(mutex);
    return ESP_OK;
}
esp_err_t hb_brightness_apply(const hb_brightness_config *value)
{
    return apply(value, false);
}
esp_err_t hb_brightness_begin_override(const hb_brightness_config *value)
{
    if (!value || value->mode != HB_BRIGHT_OVERRIDE)
        return ESP_ERR_INVALID_ARG;
    return apply(value, true);
}
esp_err_t hb_brightness_save(const hb_brightness_config *value)
{
    if (!hb_brightness_valid(value))
        return ESP_ERR_INVALID_ARG;
    if (!storage)
        return ESP_ERR_INVALID_STATE;
    nvs_handle_t handle;
    esp_err_t e = nvs_open("hb_display", NVS_READWRITE, &handle);
    if (e != ESP_OK)
        return e;
    hb_brightness_config saved = *value;
    // Override is deliberately temporary, including across reboot.
    if (saved.mode == HB_BRIGHT_OVERRIDE)
        saved.mode = HB_BRIGHT_AUTO;
    e = nvs_set_blob(handle, "brightness", &saved, sizeof(saved));
    if (e == ESP_OK)
        e = nvs_commit(handle);
    nvs_close(handle);
    return e == ESP_OK ? hb_brightness_apply(value) : e;
}
esp_err_t hb_brightness_start(bool available)
{
    storage = available;
    config = hb_brightness_defaults();
    mutex = xSemaphoreCreateMutex();
    if (!mutex)
        return ESP_ERR_NO_MEM;
    if (storage)
    {
        nvs_handle_t handle;
        esp_err_t e = nvs_open("hb_display", NVS_READONLY, &handle);
        if (e == ESP_OK)
        {
            hb_brightness_config loaded;
            size_t size = sizeof(loaded);
            e = nvs_get_blob(handle, "brightness", &loaded, &size);
            if (e == ESP_OK && size == sizeof(loaded) && hb_brightness_valid(&loaded))
            {
                config = loaded;
                if (config.mode == HB_BRIGHT_OVERRIDE)
                    config.mode = HB_BRIGHT_AUTO;
            }
            else if (e != ESP_ERR_NVS_NOT_FOUND)
                ESP_LOGW("brightness", "Invalid calibration record preserved; manual fallback 128");
            nvs_close(handle);
        }
    }
    hb_brightness_init(&state, 128, now_ms());
    return xTaskCreate(worker, "brightness", 4096, NULL, 1, NULL) == pdPASS ? ESP_OK
                                                                            : ESP_ERR_NO_MEM;
}
