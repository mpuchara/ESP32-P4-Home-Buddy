// SPDX-License-Identifier: Apache-2.0
#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hb_identity.h"
#include "hb_mqtt.h"
#include "hb_mqtt_payload.h"
#include "mqtt_client.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
static atomic_bool connected, discover;
static atomic_uint connection_epoch;
static bool started;
static uint64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}
bool hb_mqtt_connected(void)
{
    return atomic_load(&connected);
}
static void event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    esp_mqtt_event_handle_t e = data;
    if (id == MQTT_EVENT_CONNECTED)
    {
        atomic_fetch_add(&connection_epoch, 1);
        atomic_store(&discover, true);
        atomic_store(&connected, true);
        esp_mqtt_client_subscribe(e->client, "homeassistant/status", 1);
    }
    else if (id == MQTT_EVENT_DISCONNECTED || id == MQTT_EVENT_ERROR)
        atomic_store(&connected, false);
    else if (id == MQTT_EVENT_DATA && e->topic_len == 20 &&
             !memcmp(e->topic, "homeassistant/status", 20) && e->total_data_len == 6 &&
             e->data_len == 6 && !memcmp(e->data, "online", 6))
        atomic_store(&discover, true);
}
static bool send_message(esp_mqtt_client_handle_t client, const char *topic, const char *payload,
                         bool retained, uint32_t expiry)
{
    if (!atomic_load(&connected))
        return false;
    esp_mqtt5_publish_property_config_t property = {.message_expiry_interval = expiry};
    if (esp_mqtt5_client_set_publish_property(client, &property) != ESP_OK)
        return false;
    // QoS 0 events: never enqueue offline, never retain, never replay through outbox.
    return esp_mqtt_client_publish(client, topic, payload, 0, retained ? 1 : 0, retained) >= 0;
}
static void worker(void *unused)
{
    (void)unused;
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_EFUSE_FACTORY);
    char device[32], availability[100], state_topic[100], events_topic[100];
    snprintf(device, sizeof(device), "hb_%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3],
             mac[4], mac[5]);
    snprintf(availability, sizeof(availability), "home_buddy/%s/availability", device);
    snprintf(state_topic, sizeof(state_topic), "home_buddy/%s/identity/state", device);
    snprintf(events_topic, sizeof(events_topic), "home_buddy/%s/identity/events", device);
    hb_mqtt_config active = {0};
    esp_mqtt_client_handle_t client = NULL;
    uint32_t seen_epoch = 0;
    uint64_t last_person_id = 0;
    struct
    {
        uint64_t id, at;
        hb_id_event_kind kind;
        bool used;
    } cooldown[HB_ID_PEOPLE + 4] = {0};
    for (;;)
    {
        hb_mqtt_config cfg;
        hb_mqtt_config_get(&cfg);
        if (memcmp(&active, &cfg, sizeof(cfg)))
        {
            if (client)
            {
                send_message(client, availability, "offline", true, 0);
                esp_mqtt_client_stop(client);
                esp_mqtt_client_destroy(client);
                client = NULL;
            }
            atomic_store(&connected, false);
            active = cfg;
            if (cfg.enabled)
            {
                esp_mqtt_client_config_t c = {
                    .broker = {.address = {.hostname = active.host,
                                           .port = active.port,
                                           .transport = active.tls ? MQTT_TRANSPORT_OVER_SSL
                                                                   : MQTT_TRANSPORT_OVER_TCP},
                               .verification = {.crt_bundle_attach =
                                                    active.tls ? esp_crt_bundle_attach : NULL}},
                    .credentials = {.client_id = device,
                                    .username = active.username[0] ? active.username : NULL,
                                    .authentication = {.password = active.password[0]
                                                                       ? active.password
                                                                       : NULL}},
                    .session = {.protocol_ver = MQTT_PROTOCOL_V_5,
                                .keepalive = 30,
                                .last_will = {.topic = availability,
                                              .msg = "offline",
                                              .qos = 1,
                                              .retain = 1}},
                    .network = {.timeout_ms = 2000, .reconnect_timeout_ms = 5000},
                    .buffer = {.size = 2048},
                    .outbox = {.limit = 8192}};
                client = esp_mqtt_client_init(&c);
                if (client)
                {
                    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, event, NULL);
                    if (esp_mqtt_client_start(client) != ESP_OK)
                    {
                        esp_mqtt_client_destroy(client);
                        client = NULL;
                    }
                }
            }
        }
        memset(&cfg, 0, sizeof(cfg));
        hb_id_event e;
        uint32_t generation = atomic_load(&connection_epoch);
        // Drop everything accumulated before reconnect or while disabled/disconnected.
        bool drain = !client || !atomic_load(&connected) || generation != seen_epoch ||
                     !active.publish_identity;
        if (drain)
            while (hb_identity_event_read(HB_EVENTS_MQTT, &e))
            {
            }
        seen_epoch = generation;
        if (client && atomic_load(&connected))
        {
            if (atomic_exchange(&discover, false))
            {
                bool ok = true;
                char topic[160], payload[1800];
                for (unsigned i = 0; i < HB_MQTT_ENTITIES; i++)
                    ok = hb_mqtt_discovery_json(device, i, topic, sizeof(topic), payload,
                                                sizeof(payload)) &&
                         send_message(client, topic, payload, true, 0) && ok;
                send_message(client, availability, "online", true, 0);
                if (!ok)
                    atomic_store(&discover, true);
            }
            while (!drain && hb_identity_event_read(HB_EVENTS_MQTT, &e))
            {
                unsigned slot = 0;
                uint64_t now = now_ms();
                for (; slot < HB_ID_PEOPLE + 4; slot++)
                    if (!cooldown[slot].used ||
                        (cooldown[slot].id == e.person_id && cooldown[slot].kind == e.kind))
                        break;
                if (slot == HB_ID_PEOPLE + 4)
                    slot = 0;
                if (cooldown[slot].used && now >= cooldown[slot].at &&
                    now - cooldown[slot].at < (uint64_t)active.cooldown_seconds * 1000)
                    continue;
                char payload[1200];
                if (hb_identity_event_current(&e) &&
                    hb_mqtt_event_json(device, &e, now, payload, sizeof(payload)) &&
                    send_message(client, events_topic, payload, false, 5))
                {
                    cooldown[slot].id = e.person_id;
                    cooldown[slot].at = now;
                    cooldown[slot].kind = e.kind;
                    cooldown[slot].used = true;
                    if (e.kind == HB_ID_EVENT_RECOGNIZED)
                        last_person_id = e.person_id;
                }
            }
            hb_id_status s;
            hb_identity_get(&s);
            bool fresh = active.publish_identity && s.ready && s.mode == HB_ID_BACKGROUND &&
                         s.detection_valid && s.frame_ms && s.frame_ms <= now_ms() &&
                         now_ms() - s.frame_ms <= 5000;
            // Last-person state is separate from current confirmation and is never retained.
            char name[HB_ID_NAME] = "";
            if (active.publish_identity)
                for (unsigned i = 0; i < s.profile_count; i++)
                    if (s.profiles[i].id == last_person_id)
                        snprintf(name, sizeof(name), "%s", s.profiles[i].name);
            cJSON *j = cJSON_CreateObject();
            if (j)
            {
                cJSON_AddStringToObject(j, "last_person", name);
                cJSON_AddStringToObject(j, "status",
                                        !fresh                             ? "unavailable"
                                        : s.confirmed                      ? "confirmed"
                                        : s.match.state == HB_ID_AMBIGUOUS ? "ambiguous"
                                        : s.match.state == HB_ID_UNKNOWN   ? "unknown"
                                                                           : "unavailable");
                cJSON_AddStringToObject(j, "method", fresh ? "face" : "none");
                cJSON_AddStringToObject(j, "face_detected", fresh && s.faces ? "ON" : "OFF");
                cJSON_AddStringToObject(j, "identity_available", fresh ? "ON" : "OFF");
                char payload[512];
                if (cJSON_PrintPreallocated(j, payload, sizeof(payload), false))
                    send_message(client, state_topic, payload, false, 5);
                cJSON_Delete(j);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
esp_err_t hb_mqtt_start(void)
{
    if (started)
        return ESP_OK;
    extern esp_err_t hb_mqtt_config_load(void);
    esp_err_t e = hb_mqtt_config_load();
    if (e)
        return e;
    if (xTaskCreate(worker, "ha_mqtt", 8192, NULL, 1, NULL) != pdPASS)
        return ESP_ERR_NO_MEM;
    started = true;
    return ESP_OK;
}
