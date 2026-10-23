// SPDX-License-Identifier: Apache-2.0
#include "cJSON.h"
#include "hb_mqtt_payload.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    char out[2048], topic[200];
    for (unsigned i = 0; i < HB_MQTT_ENTITIES; i++)
    {
        assert(hb_mqtt_discovery_json("hb_123", i, topic, sizeof(topic), out, sizeof(out)));
        cJSON *j = cJSON_Parse(out);
        assert(j);
        assert(strstr(cJSON_GetStringValue(cJSON_GetObjectItem(j, "unique_id")), "hb_123"));
        assert(cJSON_GetObjectItem(j, "availability_topic"));
        assert(cJSON_GetObjectItem(j, "default_entity_id"));
        if (!i)
            assert(cJSON_GetArraySize(cJSON_GetObjectItem(j, "event_types")) == 5);
        cJSON_Delete(j);
    }
    hb_id_event e = {.event_id = 7,
                     .boot_id = 3,
                     .person_id = 123,
                     .persistent_id = true,
                     .frame_ms = 100,
                     .timestamp_ms = 200,
                     .kind = HB_ID_EVENT_RECOGNIZED,
                     .state = HB_ID_KNOWN,
                     .confirmed = true};
    strcpy(e.person_name, "Name \"UTF-8: ł\"");
    assert(hb_mqtt_event_json("hb_123", &e, 300, out, sizeof(out)));
    cJSON *j = cJSON_Parse(out);
    assert(j);
    assert(!strcmp(cJSON_GetStringValue(cJSON_GetObjectItem(j, "person_name")), e.person_name));
    assert(!strcmp(cJSON_GetStringValue(cJSON_GetObjectItem(j, "event_type")), "recognized"));
    assert(cJSON_IsNull(cJSON_GetObjectItem(j, "timestamp_unix_ms")));
    assert(!cJSON_GetObjectItem(j, "embedding"));
    cJSON_Delete(j);
    e.persistent_id = false;
    assert(hb_mqtt_event_json("hb_123", &e, 300, out, sizeof(out)));
    assert(!strstr(out, "person_id")); // No permanent IDs for RAM-only profiles.
    assert(!hb_mqtt_event_json("hb_123", &e, 5101, out, sizeof(out)));
    assert(!hb_mqtt_event_json("hb_123", &e, 99, out, sizeof(out)));
    e.confirmed = false;
    assert(!hb_mqtt_event_json("hb_123", &e, 300, out, sizeof(out)));
    e.kind = HB_ID_EVENT_UNKNOWN;
    e.person_id = 0;
    e.state = HB_ID_UNKNOWN;
    assert(hb_mqtt_event_json("hb_123", &e, 300, out, sizeof(out)));
    assert(!hb_mqtt_event_json("hb_123", &e, 300, out, 20));
    puts("MQTT discovery, JSON escaping, privacy, clock and freshness passed");
    return 0;
}
