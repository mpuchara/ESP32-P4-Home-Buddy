// SPDX-License-Identifier: Apache-2.0
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "hb_mqtt.h"
#include "nvs.h"
#include <string.h>
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static hb_mqtt_config current = {.schema = 1, .port = 1883, .cooldown_seconds = 60};
static bool valid(const hb_mqtt_config *v)
{
    return v && v->schema == 1 && v->port && v->cooldown_seconds <= 3600 &&
           memchr(v->host, 0, sizeof(v->host)) && memchr(v->username, 0, sizeof(v->username)) &&
           memchr(v->password, 0, sizeof(v->password)) && (!v->enabled || v->host[0]) &&
           !strpbrk(v->host, "/@?# \r\n\\");
}
void hb_mqtt_config_get(hb_mqtt_config *out)
{
    portENTER_CRITICAL(&lock);
    *out = current;
    portEXIT_CRITICAL(&lock);
}
esp_err_t hb_mqtt_config_save(const hb_mqtt_config *v)
{
    if (!valid(v))
        return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t e = nvs_open("hb_mqtt", NVS_READWRITE, &h);
    if (e)
        return e;
    e = nvs_set_blob(h, "config", v, sizeof(*v));
    if (!e)
        e = nvs_commit(h);
    nvs_close(h);
    if (!e)
    {
        portENTER_CRITICAL(&lock);
        current = *v;
        portEXIT_CRITICAL(&lock);
    }
    return e;
}
esp_err_t hb_mqtt_config_load(void)
{
    nvs_handle_t h;
    esp_err_t e = nvs_open("hb_mqtt", NVS_READONLY, &h);
    if (e == ESP_ERR_NVS_NOT_FOUND)
        return ESP_OK;
    if (e)
        return e;
    hb_mqtt_config v;
    size_t size = sizeof(v);
    e = nvs_get_blob(h, "config", &v, &size);
    nvs_close(h);
    if (!e && (size != sizeof(v) || !valid(&v)))
        e = ESP_ERR_INVALID_STATE;
    if (!e)
    {
        portENTER_CRITICAL(&lock);
        current = v;
        portEXIT_CRITICAL(&lock);
    }
    memset(&v, 0, sizeof(v));
    return e;
}
