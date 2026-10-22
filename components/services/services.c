// SPDX-License-Identifier: Apache-2.0
#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hb_config.h"
#include "hb_dashboard.h"
#include "hb_services.h"
#include "hb_ui.h"
#include "mqtt_client.h"
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static atomic_bool synchronized;
static char incoming[768];
static size_t received;
static bool assembling;
static hb_config_t active;
static esp_mqtt_client_handle_t client;
bool hb_time_valid(void)
{
    return atomic_load(&synchronized);
}
static void time_sync(struct timeval *tv)
{
    (void)tv;
    atomic_store(&synchronized, true);
}
static void mqtt_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    esp_mqtt_event_handle_t e = data;
    if (id == MQTT_EVENT_CONNECTED)
    {
        esp_mqtt_client_subscribe(e->client, active.temperature_topic, 1);
        hb_ui_diagnostic("MQTT", "Połączono");
    }
    else if (id == MQTT_EVENT_DISCONNECTED)
        hb_ui_diagnostic("MQTT", "Rozłączono");
    else if (id == MQTT_EVENT_DATA)
    {
        if (e->current_data_offset == 0)
        {
            received = 0;
            assembling = e->total_data_len > 0 && e->total_data_len < (int)sizeof(incoming) &&
                         e->topic_len == (int)strlen(active.temperature_topic) &&
                         !memcmp(e->topic, active.temperature_topic, e->topic_len);
        }
        if (!assembling || e->data_len < 0 || e->current_data_offset != (int)received ||
            (size_t)e->data_len > sizeof(incoming) - 1 - received)
        {
            assembling = false;
            return;
        }
        memcpy(incoming + received, e->data, e->data_len);
        received += e->data_len;
        if (received == (size_t)e->total_data_len)
        {
            hb_temperature_t value;
            if (hb_temperature_parse(incoming, received, hb_time_valid() ? time(NULL) : 0,
                                     active.temperature_age, &value))
            {
                hb_ui_temperature(value.available, value.value, value.timestamp,
                                  active.temperature_age);
            }
            assembling = false;
        }
    }
}
static void worker(void *arg)
{
    (void)arg;
    bool sntp_started = false;
    for (;;)
    {
        hb_config_t c;
        hb_config_get(&c);
        if (!sntp_started)
        {
            esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
            esp_sntp_setservername(0, "pool.ntp.org");
            esp_sntp_set_time_sync_notification_cb(time_sync);
            esp_sntp_init();
            sntp_started = true;
        }
        if (strcmp(active.mqtt_uri, c.mqtt_uri) || strcmp(active.mqtt_user, c.mqtt_user) ||
            strcmp(active.mqtt_password, c.mqtt_password) ||
            strcmp(active.temperature_topic, c.temperature_topic) ||
            active.temperature_age != c.temperature_age)
        {
            if (client)
            {
                esp_mqtt_client_stop(client);
                esp_mqtt_client_destroy(client);
                client = NULL;
            }
            received = 0;
            assembling = false;
            hb_ui_temperature(false, 0, 0, c.temperature_age);
            active = c;
            if (c.temperature_topic[0] && !strchr(c.mqtt_uri, '@') &&
                (!strncmp(c.mqtt_uri, "mqtt://", 7) || !strncmp(c.mqtt_uri, "mqtts://", 8)))
            {
                esp_mqtt_client_config_t cfg = {
                    .broker.address.uri = active.mqtt_uri,
                    .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
                    .credentials.username = active.mqtt_user,
                    .credentials.authentication.password = active.mqtt_password,
                    .network.reconnect_timeout_ms = 10000,
                    .buffer.size = 1024,
                    .buffer.out_size = 1024};
                client = esp_mqtt_client_init(&cfg);
                if (client)
                {
                    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event, NULL);
                    esp_mqtt_client_start(client);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
esp_err_t hb_services_start(void)
{
    esp_netif_init();
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    return xTaskCreate(worker, "network_services", 6144, NULL, 2, NULL) == pdPASS ? ESP_OK
                                                                                  : ESP_ERR_NO_MEM;
}
