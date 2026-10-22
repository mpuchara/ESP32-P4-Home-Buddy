// SPDX-License-Identifier: Apache-2.0
#include "hb_wifi.h"
#include "sdkconfig.h"
#if CONFIG_HB_WIFI_DIAGNOSTICS
#include "esp_event.h"
#include "esp_hosted.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hb_wifi.h"
#include <inttypes.h>
#include <stdbool.h>

static QueueHandle_t requests;
static void worker(void *arg)
{
    (void)arg;
    uint8_t request;
    for (;;)
    {
        if (xQueueReceive(requests, &request, portMAX_DELAY) != pdTRUE)
            continue;
        esp_err_t err = esp_wifi_scan_start(NULL, true);
        uint16_t count = 0;
        if (err == ESP_OK)
        {
            err = esp_wifi_scan_get_ap_num(&count);
            esp_wifi_clear_ap_list();
        }
        ESP_LOGI("wifi", "Scan %s; networks=%u (SSIDs omitted)", esp_err_to_name(err), count);
    }
}
esp_err_t hb_wifi_start(void)
{
    esp_err_t err = esp_hosted_connect_to_slave();
    if (err != ESP_OK)
        return err;
    esp_hosted_coprocessor_fwver_t version = {0};
    err = esp_hosted_get_coprocessor_fwversion(&version);
    if (err != ESP_OK)
        return err;
    ESP_LOGI("wifi", "C6 firmware %" PRIu32 ".%" PRIu32 ".%" PRIu32, version.major1, version.minor1,
             version.patch1);
    // Exact pairing is the initial target. Other versions remain
    // diagnostic-only until interoperability testing expands this allowlist.
    if (version.major1 != 2 || version.minor1 != 12 || version.patch1 != 2)
    {
        ESP_LOGE("wifi", "Unverified C6/host pairing; scanning disabled; no coprocessor update");
        return ESP_ERR_NOT_SUPPORTED;
    }
    err = esp_netif_init();
    if (err != ESP_OK)
        return err;
    err = esp_event_loop_create_default();
    if (err != ESP_OK)
        return err;
    if (!esp_netif_create_default_wifi_sta())
        return ESP_ERR_NO_MEM;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK)
        return err;
    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK)
        return err;
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK)
        return err;
    err = esp_wifi_start();
    if (err != ESP_OK)
        return err;
    requests = xQueueCreate(1, sizeof(uint8_t));
    if (!requests)
        return ESP_ERR_NO_MEM;
    if (xTaskCreate(worker, "wifi_diag", 4096, NULL, 2, NULL) != pdPASS)
    {
        vQueueDelete(requests);
        requests = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
bool hb_wifi_request_scan(void)
{
    uint8_t request = 1;
    return requests && xQueueSend(requests, &request, 0) == pdTRUE;
}
#else
esp_err_t hb_wifi_start(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}
bool hb_wifi_request_scan(void)
{
    return false;
}
#endif
