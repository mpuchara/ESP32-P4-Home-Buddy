// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_identity_events.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif
    const char *hb_mqtt_event_type(hb_id_event_kind kind);
    bool hb_mqtt_event_json(const char *device, const hb_id_event *event, uint64_t now,
                            char *output, size_t capacity);
    bool hb_mqtt_discovery_json(const char *device, unsigned entity, char *topic, size_t topic_size,
                                char *output, size_t capacity);
#define HB_MQTT_ENTITIES 6
#ifdef __cplusplus
}
#endif
