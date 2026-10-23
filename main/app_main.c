// SPDX-License-Identifier: Apache-2.0
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hb_board.h"
#include "hb_brightness_service.h"
#include "hb_camera.h"
#include "hb_compute_budget.h"
#include "hb_config.h"
#include "hb_ha.h"
#include "hb_identity.h"
#include "hb_services.h"
#include "hb_ubox.h"
#include "hb_ui.h"
#include "hb_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include <inttypes.h>

static const char *TAG = "home_buddy";

void app_main(void)
{
    hb_compute_start();
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    uint32_t flash = 0;
    esp_err_t flash_err = esp_flash_get_size(NULL, &flash);
    ESP_LOGI(TAG, "Home Buddy %s; IDF %s", esp_app_get_description()->version,
             esp_app_get_description()->idf_ver);
    ESP_LOGI(TAG, "P4 revision %d.%d; cores=%d; flash=%" PRIu32 " (%s); PSRAM=%u",
             chip.revision / 100, chip.revision % 100, chip.cores, flash,
             esp_err_to_name(flash_err), (unsigned)esp_psram_get_size());
    // Never erase NVS automatically: errors may be recoverable user data.
    esp_err_t nvs = nvs_flash_init();
    if (nvs != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS unavailable: %s; preserved for recovery", esp_err_to_name(nvs));
    }
#if CONFIG_HB_BOARD_PROFILE_CONFIRMED
    esp_err_t config_err = nvs == ESP_OK ? hb_config_start() : nvs;
    if (config_err != ESP_OK)
    {
        ESP_LOGE(TAG, "Configuration unavailable; settings disabled: %s",
                 esp_err_to_name(config_err));
    }
    esp_err_t err = hb_board_start();
    if (err == ESP_OK)
    {
        uint16_t sensor_id;
        esp_err_t camera_err = hb_camera_probe(&sensor_id);
        ESP_LOGI(TAG, "Local camera read-only probe: %s; ID=0x%04x", esp_err_to_name(camera_err),
                 sensor_id);
    }
    if (err == ESP_OK)
    {
        hb_brightness_start(nvs == ESP_OK);
#if CONFIG_HB_IDENTITY_EXPERIMENTAL
        if (hb_identity_start() != ESP_OK)
            ESP_LOGW(TAG, "Identity infrastructure unavailable");
#endif
        err = hb_ui_start();
    }
    bool ui_ready = err == ESP_OK;
    if (ui_ready)
        hb_camera_start();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "UI unavailable: %s; serial diagnostics remain active", esp_err_to_name(err));
    }
#if CONFIG_HB_WIFI_DIAGNOSTICS
    if (config_err == ESP_OK)
    {
        err = hb_wifi_start();
        if (err != ESP_OK)
            ESP_LOGE(TAG, "Wi-Fi diagnostics: %s", esp_err_to_name(err));
    }
#endif
    if (config_err == ESP_OK && ui_ready)
    {
        hb_services_start();
        hb_ubox_start();
        if (hb_ha_start() != ESP_OK)
            ESP_LOGW(TAG, "Home Assistant configuration/start failed");
    }
#else
    ESP_LOGW(TAG, "Board profile unconfirmed; peripheral outputs disabled. See docs/HARDWARE.md");
#endif
    for (;;)
    {
        ESP_LOGI(TAG,
                 "uptime=%" PRIi64 "s internal_free=%u internal_largest=%u internal_min=%u "
                 "psram_free=%u psram_largest=%u dma_free=%u dma_largest=%u",
                 esp_timer_get_time() / 1000000,
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                 (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
