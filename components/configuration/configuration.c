// SPDX-License-Identifier: Apache-2.0
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "hb_config.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/platform_util.h"
#include "nvs.h"
#include <stdatomic.h>
#include <string.h>
static hb_config_t current;
static SemaphoreHandle_t mutex;
static atomic_bool has_pin;
static atomic_bool ready;
typedef struct
{
    hb_config_t settings;
    uint8_t salt[16], hash[32];
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
static uint8_t salt[16], digest[32];
static int64_t unlocked_until, locked_until;
static unsigned failures;
static bool pin_valid(const char *p)
{
    size_t n = p ? strlen(p) : 0;
    if (n < 6 || n > 12)
        return false;
    for (size_t i = 0; i < n; ++i)
        if (p[i] < '0' || p[i] > '9')
            return false;
    return true;
}
static int hash(const char *p, const uint8_t *s, uint8_t *out)
{
    return mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256, (const unsigned char *)p, strlen(p), s,
                                         16, 20000, 32, out);
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
        memcpy(salt, loaded.salt, 16);
        memcpy(digest, loaded.hash, 32);
        unsigned any = 0;
        for (unsigned i = 0; i < sizeof(digest); ++i)
            any |= digest[i];
        has_pin = any != 0;
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
bool hb_config_has_pin(void)
{
    return has_pin;
}
bool hb_config_ready(void)
{
    return ready;
}
esp_err_t hb_config_unlock(const char *pin)
{
    int64_t now = esp_timer_get_time();
    if (now < locked_until || !has_pin || !pin_valid(pin))
        return ESP_ERR_INVALID_STATE;
    uint8_t got[32];
    if (hash(pin, salt, got))
        return ESP_FAIL;
    unsigned diff = 0;
    for (unsigned i = 0; i < 32; ++i)
        diff |= got[i] ^ digest[i];
    mbedtls_platform_zeroize(got, sizeof(got));
    if (diff)
    {
        if (++failures >= 5)
        {
            locked_until = now + 60000000;
            failures = 0;
        }
        return ESP_ERR_INVALID_ARG;
    }
    failures = 0;
    unlocked_until = now + 120000000;
    return ESP_OK;
}
static esp_err_t write_config(const hb_config_t *value, const char *pin, bool require_pin)
{
    if (!valid(value))
        return ESP_ERR_INVALID_ARG;
    if (require_pin && has_pin && esp_timer_get_time() > unlocked_until)
        return ESP_ERR_INVALID_STATE;
    uint8_t newsalt[16] = {0}, newhash[32] = {0};
    bool create_pin = require_pin && !has_pin;
    if (create_pin)
    {
        if (!pin_valid(pin))
            return ESP_ERR_INVALID_ARG;
        esp_fill_random(newsalt, 16);
        if (hash(pin, newsalt, newhash))
            return ESP_FAIL;
    }
    nvs_handle_t h;
    esp_err_t e = nvs_open("hb_config", NVS_READWRITE, &h);
    if (e)
        return e;
    stored_config record = {.settings = *value};
    memcpy(record.salt, has_pin ? salt : newsalt, 16);
    memcpy(record.hash, has_pin ? digest : newhash, 32);
    e = nvs_set_blob(h, "record", &record, sizeof(record));
    if (!e)
        e = nvs_commit(h);
    nvs_close(h);
    if (!e)
    {
        xSemaphoreTake(mutex, portMAX_DELAY);
        current = *value;
        xSemaphoreGive(mutex);
        if (create_pin)
        {
            memcpy(salt, newsalt, 16);
            memcpy(digest, newhash, 32);
            has_pin = true;
        }
        unlocked_until = 0;
    }
    mbedtls_platform_zeroize(newhash, sizeof(newhash));
    mbedtls_platform_zeroize(&record, sizeof(record));
    return e;
}
esp_err_t hb_config_save(const hb_config_t *value, const char *pin)
{
    return write_config(value, pin, true);
}
esp_err_t hb_config_save_wifi(const char *ssid, const char *password)
{
    if (!ssid || !password || strlen(ssid) > 32 || strlen(password) > 64)
        return ESP_ERR_INVALID_ARG;
    hb_config_t value;
    hb_config_get(&value);
    strcpy(value.ssid, ssid);
    strcpy(value.wifi_password, password);
    esp_err_t e = write_config(&value, NULL, false);
    mbedtls_platform_zeroize(&value, sizeof(value));
    return e;
}
