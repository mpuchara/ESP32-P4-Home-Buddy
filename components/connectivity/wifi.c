// SPDX-License-Identifier: Apache-2.0
#include "hb_config.h"
#include "hb_ui.h"
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
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
static QueueHandle_t requests;
static atomic_bool connected;
static atomic_uint_fast32_t station_ip;
static bool manual_disconnect;
static char networks[300];
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static void event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        atomic_store(&connected, true);
        ip_event_got_ip_t *ip = data;
        atomic_store(&station_ip, ip->ip_info.ip.addr);
        char msg[80];
        snprintf(msg, sizeof(msg), "Połączono: " IPSTR, IP2STR(&ip->ip_info.ip));
        hb_ui_diagnostic("Wi-Fi", msg);
    }
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
    {
        atomic_store(&connected, false);
        hb_ui_diagnostic("Wi-Fi", "Rozłączono");
    }
}
static void worker(void *arg)
{
    (void)arg;
    hb_config_t previous = {0};
    unsigned failures = 0;
    uint32_t next = 0;
    for (;;)
    {
        uint8_t request = 0;
        xQueueReceive(requests, &request, pdMS_TO_TICKS(1000));
        hb_config_t c;
        hb_config_get(&c);
        if (strcmp(c.ssid, previous.ssid) || strcmp(c.wifi_password, previous.wifi_password))
        {
            esp_wifi_disconnect();
            previous = c;
            manual_disconnect = false;
            next = 0;
            failures = 0;
            wifi_config_t cfg = {0};
            memcpy(cfg.sta.ssid, c.ssid, strlen(c.ssid));
            memcpy(cfg.sta.password, c.wifi_password, strlen(c.wifi_password));
            esp_err_t e = esp_wifi_set_config(WIFI_IF_STA, &cfg);
            if (e)
            {
                manual_disconnect = true;
                hb_ui_diagnostic("Wi-Fi", "Błąd konfiguracji stacji");
            }
        }
        if (request == 2)
        {
            manual_disconnect = true;
            esp_wifi_disconnect();
            atomic_store(&connected, false);
        }
        if (request == 1)
        {
            esp_err_t e = esp_wifi_scan_start(NULL, true);
            uint16_t count = 8;
            static wifi_ap_record_t records[8];
            if (!e)
                e = esp_wifi_scan_get_ap_records(&count, records);
            char result[300] = "";
            if (!e)
                for (unsigned i = 0; i < count; ++i)
                {
                    records[i].ssid[32] = 0;
                    if (i)
                        strlcat(result, "\n", sizeof(result));
                    strlcat(result, (char *)records[i].ssid, sizeof(result));
                }
            esp_wifi_clear_ap_list();
            portENTER_CRITICAL(&lock);
            strcpy(networks, result);
            portEXIT_CRITICAL(&lock);
            hb_ui_diagnostic("Wi-Fi", e ? "Błąd skanowania" : "Skan gotowy; lista w konfiguracji");
            ESP_LOGI("wifi", "Scan %s; networks=%u (SSIDs omitted)", esp_err_to_name(e),
                     e ? 0 : count);
        }
        uint32_t now = xTaskGetTickCount();
        if (c.ssid[0] && !manual_disconnect && !atomic_load(&connected) &&
            (int32_t)(now - next) >= 0)
        {
            esp_err_t e = esp_wifi_connect();
            failures++;
            next = now + pdMS_TO_TICKS((failures < 6 ? failures * 5 : 30) * 1000);
            if (e)
                hb_ui_diagnostic("Wi-Fi", "Ponowna próba połączenia");
        }
        if (atomic_load(&connected))
        {
            failures = 0;
            wifi_ap_record_t ap;
            if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK)
            {
                static unsigned ticks;
                if (++ticks % 30 == 0)
                {
                    esp_ip4_addr_t ip = {.addr = atomic_load(&station_ip)};
                    char msg[80];
                    snprintf(msg, sizeof(msg), "IP " IPSTR ", RSSI %d dBm", IP2STR(&ip), ap.rssi);
                    hb_ui_diagnostic("Wi-Fi", msg);
                }
            }
        }
    }
}
esp_err_t hb_wifi_start(void)
{
    // This pinned upstream tag otherwise logs the SSID at warning level.
    esp_log_level_set("rpc_utils", ESP_LOG_ERROR);
    esp_err_t e = esp_netif_init();
    if (e)
        return e;
    e = esp_event_loop_create_default();
    if (e && e != ESP_ERR_INVALID_STATE)
        return e;
    e = esp_hosted_connect_to_slave();
    if (e)
        return e;
    esp_hosted_coprocessor_fwver_t version = {0};
    e = esp_hosted_get_coprocessor_fwversion(&version);
    if (e)
        ESP_LOGW("wifi", "Firmware metadata RPC unavailable; version UNKNOWN; legacy scan verified "
                         "on this unit");
    else
        ESP_LOGI("wifi", "C6 firmware %u.%u.%u", (unsigned)version.major1, (unsigned)version.minor1,
                 (unsigned)version.patch1);
    if (!esp_netif_create_default_wifi_sta())
        return ESP_ERR_NO_MEM;
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event, NULL);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    e = esp_wifi_init(&cfg);
    if (e)
        return e;
    e = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (e)
        return e;
    e = esp_wifi_set_mode(WIFI_MODE_STA);
    if (e)
        return e;
    e = esp_wifi_start();
    if (e)
        return e;
    requests = xQueueCreate(2, sizeof(uint8_t));
    if (!requests)
        return ESP_ERR_NO_MEM;
    if (xTaskCreate(worker, "wifi", 6144, NULL, 2, NULL) != pdPASS)
        return ESP_ERR_NO_MEM;
    hb_wifi_request_scan();
    return ESP_OK;
}
bool hb_wifi_request_scan(void)
{
    uint8_t r = 1;
    return requests && xQueueSend(requests, &r, 0) == pdTRUE;
}
void hb_wifi_disconnect(void)
{
    uint8_t r = 2;
    if (requests)
        xQueueSend(requests, &r, 0);
}
void hb_wifi_networks(char *out, size_t size)
{
    portENTER_CRITICAL(&lock);
    strlcpy(out, networks, size);
    portEXIT_CRITICAL(&lock);
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
void hb_wifi_disconnect(void)
{
}
void hb_wifi_networks(char *out, size_t size)
{
    if (size)
        out[0] = 0;
}
#endif
