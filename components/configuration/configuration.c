// SPDX-License-Identifier: Apache-2.0
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "hb_config.h"
#include "mbedtls/platform_util.h"
#include "nvs.h"
#include <math.h>
#include <stdatomic.h>
#include <string.h>
static hb_config_t current;
static hb_weather_config_t weather = {.schema = 1};
static SemaphoreHandle_t mutex;
static atomic_bool ready;
static atomic_bool identity_auto;
typedef struct
{
    hb_config_t settings;
    uint8_t reserved[48]; // Preserve the existing schema-1 record size; old PIN is ignored.
} stored_config;
_Static_assert(sizeof(hb_config_t) == 852, "Preserve schema-1 field layout");
_Static_assert(sizeof(stored_config) == 900, "Preserve existing NVS record size");
static bool valid_weather(const hb_weather_config_t *v)
{
    return v && v->schema == 1 && memchr(v->city, 0, sizeof(v->city)) &&
           (!v->has_location ||
            (v->city[0] && isfinite(v->latitude) && isfinite(v->longitude) && v->latitude >= -90 &&
             v->latitude <= 90 && v->longitude >= -180 && v->longitude <= 180));
}
static bool valid(const hb_config_t *v)
{
    if (!v || v->schema != 1 || v->poll_seconds < 5 || v->poll_seconds > 3600 || !v->ubox_port)
        return false;
#define TERMINATED(field)                                                                          \
    if (!memchr(v->field, 0, sizeof(v->field)))                                                    \
    return false
    TERMINATED(ssid);
    TERMINATED(wifi_password);
    TERMINATED(ubox_host);
    TERMINATED(ubox_token);
#undef TERMINATED
    return !strpbrk(v->ubox_host, "/@?# ") && !strchr(v->ubox_host, 92) &&
           !strchr(v->ubox_host, 13) && !strchr(v->ubox_host, 10) && !strchr(v->ubox_token, 13) &&
           !strchr(v->ubox_token, 10);
}
esp_err_t hb_config_start(void)
{
    mutex = xSemaphoreCreateMutex();
    if (!mutex)
        return ESP_ERR_NO_MEM;
    current = (hb_config_t){
        .schema = 1, .ubox_port = 8100, .poll_seconds = 30, .legacy_temperature_age = 900};
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
        memset(current.legacy_transport, 0, sizeof(current.legacy_transport));
    }
    else if (e != ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_close(h);
        return e;
    }
    hb_weather_config_t saved_weather;
    n = sizeof(saved_weather);
    esp_err_t we = nvs_get_blob(h, "weather", &saved_weather, &n);
    if (we == ESP_OK && n == sizeof(saved_weather) && valid_weather(&saved_weather))
        weather = saved_weather;
    else if (we != ESP_ERR_NVS_NOT_FOUND)
        ESP_LOGW("configuration", "Weather settings invalid; preserved until explicit resave");
    uint8_t automatic = 0;
    esp_err_t ae = nvs_get_u8(h, "identity_auto", &automatic);
    identity_auto = ae == ESP_OK && automatic == 1;
    if (ae != ESP_OK && ae != ESP_ERR_NVS_NOT_FOUND)
        ESP_LOGW("configuration", "AUTO preference unavailable; disabled");
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
    memset(record.settings.legacy_transport, 0, sizeof(record.settings.legacy_transport));
    e = nvs_set_blob(h, "record", &record, sizeof(record));
    if (!e)
        e = nvs_commit(h);
    nvs_close(h);
    if (!e)
    {
        xSemaphoreTake(mutex, portMAX_DELAY);
        current = record.settings;
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
void hb_config_weather_get(hb_weather_config_t *out)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    *out = weather;
    xSemaphoreGive(mutex);
}
esp_err_t hb_config_weather_save(const hb_weather_config_t *value)
{
    if (!valid_weather(value))
        return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t e = nvs_open("hb_config", NVS_READWRITE, &h);
    if (e)
        return e;
    e = nvs_set_blob(h, "weather", value, sizeof(*value));
    if (!e)
        e = nvs_commit(h);
    nvs_close(h);
    if (!e)
    {
        xSemaphoreTake(mutex, portMAX_DELAY);
        weather = *value;
        xSemaphoreGive(mutex);
    }
    return e;
}

bool hb_config_identity_auto(void)
{
    return ready && identity_auto;
}
esp_err_t hb_config_identity_auto_save(bool enabled)
{
    if (!ready)
        return ESP_ERR_INVALID_STATE;
    nvs_handle_t h;
    esp_err_t e = nvs_open("hb_config", NVS_READWRITE, &h);
    if (e != ESP_OK)
        return e;
    e = nvs_set_u8(h, "identity_auto", enabled ? 1 : 0);
    if (e == ESP_OK)
        e = nvs_commit(h);
    uint8_t check = 2;
    if (e == ESP_OK)
        e = nvs_get_u8(h, "identity_auto", &check);
    if (e == ESP_OK && check != (enabled ? 1 : 0))
        e = ESP_FAIL;
    nvs_close(h);
    if (e == ESP_OK)
        identity_auto = enabled;
    return e;
}
