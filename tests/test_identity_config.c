// SPDX-License-Identifier: Apache-2.0
#include "freertos/semphr.h"
#include "hb_config.h"
#include "nvs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static bool exists, failed_commit, failed_read;
static uint8_t disk, staged;
static unsigned char record[900];
static bool record_exists;
SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    return (void *)1;
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
esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *h)
{
    assert(!strcmp(name, "hb_config") && mode == NVS_READWRITE);
    *h = 1;
    return ESP_OK;
}
void nvs_close(nvs_handle_t h)
{
    (void)h;
}
esp_err_t nvs_get_u8(nvs_handle_t h, const char *k, uint8_t *v)
{
    (void)h;
    assert(!strcmp(k, "identity_auto"));
    if (failed_read)
        return ESP_FAIL;
    if (!exists)
        return ESP_ERR_NVS_NOT_FOUND;
    *v = disk;
    return ESP_OK;
}
esp_err_t nvs_set_u8(nvs_handle_t h, const char *k, uint8_t v)
{
    (void)h;
    assert(!strcmp(k, "identity_auto"));
    staged = v;
    return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t h)
{
    (void)h;
    if (failed_commit)
        return ESP_FAIL;
    disk = staged;
    exists = true;
    return ESP_OK;
}
esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *v, size_t *n)
{
    (void)h;
    if (strcmp(key, "record") || !record_exists)
        return ESP_ERR_NVS_NOT_FOUND;
    assert(*n == sizeof(record));
    memcpy(v, record, *n);
    return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *v, size_t n)
{
    (void)h;
    assert(!strcmp(key, "record") && n == sizeof(record));
    memcpy(record, v, n);
    record_exists = true;
    return ESP_OK;
}
int main(void)
{
    assert(hb_config_start() == ESP_OK && !hb_config_identity_auto());
    assert(hb_config_identity_auto_save(true) == ESP_OK && hb_config_identity_auto());
    assert(hb_config_start() == ESP_OK && hb_config_identity_auto());
    failed_commit = true;
    assert(hb_config_identity_auto_save(false) != ESP_OK && hb_config_identity_auto());
    failed_commit = false;
    assert(hb_config_start() == ESP_OK && hb_config_identity_auto());
    failed_read = true;
    assert(hb_config_identity_auto_save(false) != ESP_OK && hb_config_identity_auto());
    failed_read = false;
    assert(hb_config_start() == ESP_OK && !hb_config_identity_auto());
    disk = 2;
    assert(hb_config_start() == ESP_OK && !hb_config_identity_auto());
    assert(hb_config_identity_auto_save(true) == ESP_OK);
    hb_config_t cfg;
    hb_config_get(&cfg);
    strcpy(cfg.ssid, "example");
    assert(hb_config_save(&cfg) == ESP_OK);
    assert(hb_config_start() == ESP_OK && hb_config_identity_auto());
    hb_config_get(&cfg);
    assert(!strcmp(cfg.ssid, "example"));
    puts("AUTO opt-in default, reboot, commit/readback failure, invalid key, schema preservation "
         "passed");
    return 0;
}
