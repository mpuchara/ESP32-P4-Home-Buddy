// SPDX-License-Identifier: Apache-2.0
#include "hb_ha.h"
#include "nvs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static hb_ha_config disk, staged;
static bool exists, fail_commit, fail_read, corrupt_read;
esp_err_t hb_ha_config_load(void);
esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *h)
{
    assert(!strcmp(name, "hb_ha"));
    *h = 1;
    return !exists && mode == NVS_READONLY ? ESP_ERR_NVS_NOT_FOUND : ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *value, size_t size)
{
    (void)h;
    assert(!strcmp(key, "config") && size == sizeof(staged));
    memcpy(&staged, value, size);
    return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t h)
{
    (void)h;
    if (fail_commit)
        return ESP_FAIL;
    disk = staged;
    exists = true;
    return ESP_OK;
}
esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *value, size_t *size)
{
    (void)h;
    assert(!strcmp(key, "config") && *size == sizeof(disk));
    if (fail_read)
        return ESP_FAIL;
    if (!exists)
        return ESP_ERR_NVS_NOT_FOUND;
    memcpy(value, &disk, sizeof(disk));
    *size = sizeof(disk);
    if (corrupt_read)
        ((hb_ha_config *)value)->port ^= 1;
    return ESP_OK;
}
void nvs_close(nvs_handle_t h)
{
    (void)h;
}
int main(void)
{
    assert(hb_ha_config_load() == ESP_OK);
    hb_ha_config a = {.schema = 1, .port = 8123, .cooldown_seconds = 60, .enabled = true}, got;
    strcpy(a.host, "ha.test");
    strcpy(a.secret, "-AbCdE_fGhIjKlMnO-pQrS-tU");
    assert(hb_ha_config_save(&a) == ESP_OK);
    hb_ha_config_get(&got);
    assert(!memcmp(&got, &a, sizeof(a)));
    hb_ha_config b = a;
    strcpy(b.host, "other.test");
    fail_commit = true;
    assert(hb_ha_config_save(&b) != ESP_OK);
    fail_commit = false;
    hb_ha_config_get(&got);
    assert(!memcmp(&got, &a, sizeof(a)));
    assert(hb_ha_config_load() == ESP_OK);
    hb_ha_config_get(&got);
    assert(!memcmp(&got, &a, sizeof(a)));
    fail_read = true;
    assert(hb_ha_config_save(&b) != ESP_OK);
    fail_read = false;
    hb_ha_config_get(&got);
    assert(!memcmp(&got, &a, sizeof(a)));
    assert(hb_ha_config_load() == ESP_OK);
    hb_ha_config_get(&got);
    assert(!memcmp(&got, &b, sizeof(b)));
    corrupt_read = true;
    assert(hb_ha_config_save(&a) != ESP_OK);
    corrupt_read = false;
    hb_ha_config_get(&got);
    assert(!memcmp(&got, &b, sizeof(b)));
    a.secret[0] = 0;
    assert(hb_ha_config_save(&a) == ESP_ERR_INVALID_ARG);
    hb_ha_config_get(&got);
    assert(!memcmp(&got, &b, sizeof(b)));
    puts("HA short-ID persistence, reload, commit/readback failure checks passed");
    return 0;
}
