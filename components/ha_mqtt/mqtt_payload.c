// SPDX-License-Identifier: Apache-2.0
#include "cJSON.h"
#include "hb_mqtt_payload.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
const char *hb_mqtt_event_type(hb_id_event_kind k)
{
    static const char *types[] = {"recognized", "unknown", "ambiguous", "presence_enter",
                                  "presence_leave"};
    return (unsigned)k < 5 ? types[k] : "invalid";
}
static bool serialize(cJSON *j, char *out, size_t n)
{
    bool ok = j && out && n <= INT32_MAX && cJSON_PrintPreallocated(j, out, (int)n, false);
    cJSON_Delete(j);
    return ok;
}
bool hb_mqtt_event_json(const char *device, const hb_id_event *e, uint64_t now, char *out, size_t n)
{
    if (!device || !e || e->frame_ms > now || now - e->frame_ms > 5000 || (unsigned)e->kind >= 5 ||
        e->source != HB_ID_SOURCE_FACE ||
        (e->kind == HB_ID_EVENT_RECOGNIZED && (!e->confirmed || e->state != HB_ID_KNOWN)) ||
        !memchr(e->person_name, 0, HB_ID_NAME))
        return false;
    cJSON *j = cJSON_CreateObject();
    if (!j)
        return false;
    char id[100];
    snprintf(id, sizeof(id), "%s-%016" PRIx64 "-%" PRIu64, device, e->boot_id, e->event_id);
    cJSON_AddNumberToObject(j, "schema", 1);
    cJSON_AddStringToObject(j, "event_type", hb_mqtt_event_type(e->kind));
    cJSON_AddStringToObject(j, "event_id", id);
    if (e->persistent_id && e->person_id)
    {
        snprintf(id, sizeof(id), "%016" PRIx64, e->person_id);
        cJSON_AddStringToObject(j, "person_id", id);
    }
    if (e->person_id)
        cJSON_AddStringToObject(j, "person_name", e->person_name);
    cJSON_AddStringToObject(j, "method", "face");
    cJSON_AddStringToObject(j, "status",
                            e->confirmed                  ? "confirmed"
                            : e->state == HB_ID_AMBIGUOUS ? "ambiguous"
                            : e->state == HB_ID_UNKNOWN   ? "unknown"
                                                          : "unavailable");
    // Epoch milliseconds are omitted until SNTP time is available.
    if (e->unix_ms > 0)
    {
        cJSON_AddNumberToObject(j, "timestamp_unix_ms", (double)e->unix_ms);
        time_t seconds = (time_t)(e->unix_ms / 1000);
        struct tm *utc = gmtime(&seconds);
        char timestamp[32];
        if (utc && strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", utc))
            cJSON_AddStringToObject(j, "timestamp", timestamp);
        else
            cJSON_AddNullToObject(j, "timestamp");
    }
    else
    {
        cJSON_AddNullToObject(j, "timestamp_unix_ms");
        cJSON_AddNullToObject(j, "timestamp");
    }
    cJSON_AddNumberToObject(j, "uptime_ms", (double)e->timestamp_ms);
    cJSON_AddNumberToObject(j, "source_age_ms", (double)(now - e->frame_ms));
    cJSON_AddNumberToObject(j, "session", e->session);
    cJSON_AddNumberToObject(j, "encounter", e->encounter);
    cJSON_AddNumberToObject(j, "similarity", e->similarity);
    cJSON_AddNumberToObject(j, "second_similarity", e->second);
    return serialize(j, out, n);
}
bool hb_mqtt_discovery_json(const char *device, unsigned i, char *topic, size_t tn, char *out,
                            size_t n)
{
    static const char *domains[] = {"event",  "sensor",        "sensor",
                                    "sensor", "binary_sensor", "binary_sensor"};
    static const char *keys[] = {"recognition",     "last_person",   "identity_status",
                                 "identity_method", "face_detected", "identity_available"};
    static const char *values[] = {NULL,
                                   "{{ value_json.last_person }}",
                                   "{{ value_json.status }}",
                                   "{{ value_json.method }}",
                                   "{{ value_json.face_detected }}",
                                   "{{ value_json.identity_available }}"};
    if (!device || i >= HB_MQTT_ENTITIES)
        return false;
    int len = snprintf(topic, tn, "homeassistant/%s/%s/%s/config", domains[i], device, keys[i]);
    if (len < 0 || (size_t)len >= tn)
        return false;
    cJSON *j = cJSON_CreateObject();
    if (!j)
        return false;
    char b[160];
    snprintf(b, sizeof(b), "%s_%s", device, keys[i]);
    cJSON_AddStringToObject(j, "unique_id", b);
    snprintf(b, sizeof(b), "%s.home_buddy_%s", domains[i], keys[i]);
    cJSON_AddStringToObject(j, "default_entity_id", b);
    cJSON_AddStringToObject(j, "name", keys[i]);
    snprintf(b, sizeof(b), "home_buddy/%s/identity/%s", device, i ? "state" : "events");
    cJSON_AddStringToObject(j, "state_topic", b);
    snprintf(b, sizeof(b), "home_buddy/%s/availability", device);
    cJSON_AddStringToObject(j, "availability_topic", b);
    if (i)
    {
        cJSON_AddStringToObject(j, "value_template", values[i]);
        cJSON_AddNumberToObject(j, "expire_after", 15);
    }
    if (i >= 4)
    {
        cJSON_AddStringToObject(j, "payload_on", "ON");
        cJSON_AddStringToObject(j, "payload_off", "OFF");
    }
    if (!i)
    {
        cJSON *types = cJSON_AddArrayToObject(j, "event_types");
        for (unsigned k = 0; k < 5; k++)
            cJSON_AddItemToArray(types, cJSON_CreateString(hb_mqtt_event_type(k)));
    }
    cJSON *d = cJSON_AddObjectToObject(j, "device");
    cJSON *ids = cJSON_AddArrayToObject(d, "identifiers");
    cJSON_AddItemToArray(ids, cJSON_CreateString(device));
    cJSON_AddStringToObject(d, "name", "Home Buddy");
    cJSON_AddStringToObject(d, "manufacturer", "Home Buddy");
    cJSON_AddStringToObject(d, "model", "ESP32-P4");
    return serialize(j, out, n);
}
