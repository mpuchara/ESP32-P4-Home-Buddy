// SPDX-License-Identifier: Apache-2.0
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "hb_ha.h"
#include "nvs.h"
#include <string.h>
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static hb_ha_config current = {.schema = 1, .port = 8123, .cooldown_seconds = 60};
void hb_ha_config_get(hb_ha_config *out)
{
    portENTER_CRITICAL(&lock);
    *out = current;
    portEXIT_CRITICAL(&lock);
}
esp_err_t hb_ha_config_save(const hb_ha_config *v)
{
    if (!hb_ha_config_valid(v, v && v->enabled))
        return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t e = nvs_open("hb_ha", NVS_READWRITE, &h);
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
esp_err_t hb_ha_config_load(void)
{
    nvs_handle_t h;
    esp_err_t e = nvs_open("hb_ha", NVS_READONLY, &h);
    if (e == ESP_ERR_NVS_NOT_FOUND)
        return ESP_OK;
    if (e)
        return e;
    hb_ha_config v;
    size_t size = sizeof(v);
    e = nvs_get_blob(h, "config", &v, &size);
    nvs_close(h);
    if (!e && (size != sizeof(v) || !hb_ha_config_valid(&v, v.enabled)))
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
