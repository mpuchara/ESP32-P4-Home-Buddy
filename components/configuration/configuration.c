// SPDX-License-Identifier: Apache-2.0
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "hb_config.h"
#include "mbedtls/platform_util.h"
#include "nvs.h"
#include <stdatomic.h>
#include <string.h>
static hb_config_t current;
static SemaphoreHandle_t mutex;
static atomic_bool ready;
typedef struct
{
    hb_config_t settings;
    uint8_t reserved[48]; // Preserve the existing schema-1 record size; old PIN is ignored.
} stored_config;
static bool valid(const hb_config_t *v)
{
    if (!v || v->schema != 1 || v->poll_seconds < 5 || v->poll_seconds > 3600 || !v->ubox_port ||
        v->temperature_age < 30 || v->temperature_age > 86400)
        return false;
#define TERMINATED(field)                                                                          \
    if (!memchr(v->field, 0, sizeof(v->field)))                                                    \
    return false
    TERMINATED(ssid);
    TERMINATED(wifi_password);
    TERMINATED(ubox_host);
    TERMINATED(ubox_token);
    TERMINATED(mqtt_uri);
    TERMINATED(mqtt_user);
    TERMINATED(mqtt_password);
    TERMINATED(temperature_topic);
#undef TERMINATED
    return !strpbrk(v->ubox_host, "/@?# ") && !strchr(v->ubox_host, 92) &&
           !strchr(v->ubox_host, 13) && !strchr(v->ubox_host, 10) && !strchr(v->ubox_token, 13) &&
           !strchr(v->ubox_token, 10) && !strpbrk(v->mqtt_uri, "@?# ") &&
           !strchr(v->mqtt_uri, 13) && !strchr(v->mqtt_uri, 10) &&
           !strpbrk(v->temperature_topic, "+#") && !strchr(v->temperature_topic, 13) &&
           !strchr(v->temperature_topic, 10);
}
esp_err_t hb_config_start(void)
{
    mutex = xSemaphoreCreateMutex();
    if (!mutex)
        return ESP_ERR_NO_MEM;
    current =
        (hb_config_t){.schema = 1, .ubox_port = 8100, .poll_seconds = 30, .temperature_age = 900};
    nvs_handle_t h;
    esp_err_t e = nvs_open("hb_config", NVS_READWRITE, &h);
    if (e != ESP_OK)
        return e;
    stored_config loaded;
    size_t n = sizeof(loaded);
    e = nvs_get_blob(h, "record", &loaded, &n);
    if (e == ESP_OK)
    {
        if (n != sizeof(loaded) || !valid(&loaded.settings))
        {
            nvs_close(h);
            return ESP_ERR_INVALID_STATE;
        }
        current = loaded.settings;
    }
    else if (e != ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_close(h);
        return e;
    }
    nvs_close(h);
    mbedtls_platform_zeroize(&loaded, sizeof(loaded));
    ready = true;
    return ESP_OK;
}
void hb_config_get(hb_config_t *out)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    *out = current;
    xSemaphoreGive(mutex);
}
bool hb_config_ready(void)
{
    return ready;
}
esp_err_t hb_config_save(const hb_config_t *value)
{
    if (!valid(value))
        return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t e = nvs_open("hb_config", NVS_READWRITE, &h);
    if (e)
        return e;
    stored_config record = {.settings = *value};
    e = nvs_set_blob(h, "record", &record, sizeof(record));
    if (!e)
        e = nvs_commit(h);
    nvs_close(h);
    if (!e)
    {
        xSemaphoreTake(mutex, portMAX_DELAY);
        current = *value;
        xSemaphoreGive(mutex);
    }
    mbedtls_platform_zeroize(&record, sizeof(record));
    return e;
}
esp_err_t hb_config_save_wifi(const char *ssid, const char *password)
{
    if (!ssid || !password || strlen(ssid) > 32 || strlen(password) > 64)
        return ESP_ERR_INVALID_ARG;
    hb_config_t value;
    hb_config_get(&value);
    strcpy(value.ssid, ssid);
    strcpy(value.wifi_password, password);
    esp_err_t e = hb_config_save(&value);
    mbedtls_platform_zeroize(&value, sizeof(value));
    return e;
}
