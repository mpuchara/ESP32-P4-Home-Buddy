// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_identity_core.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum
    {
        HB_ID_SOURCE_FACE,
        HB_ID_SOURCE_VOICE,
        HB_ID_SOURCE_BOTH
    } hb_id_source;
    typedef enum
    {
        HB_ID_EVENT_RECOGNIZED,
        HB_ID_EVENT_UNKNOWN,
        HB_ID_EVENT_AMBIGUOUS,
        HB_ID_EVENT_ENTERED,
        HB_ID_EVENT_LEFT
    } hb_id_event_kind;
    typedef struct
    {
        uint64_t event_id, person_id, timestamp_ms, frame_ms;
        int64_t unix_ms; // 0 until device clock is synchronized; never fabricated.
        uint64_t boot_id;
        uint32_t session, sequence, encounter;
        hb_id_state state;
        float similarity, second;
        bool confirmed, persistent_id;
        hb_id_source source;
        hb_id_event_kind kind;
        char person_name[HB_ID_NAME];
    } hb_id_event;
#define HB_EVENT_CAPACITY 16
#define HB_EVENT_CONSUMERS 3
    typedef enum
    {
        HB_EVENTS_GREETING,
        HB_EVENTS_MQTT,
        HB_EVENTS_LEGACY
    } hb_event_consumer;
    typedef struct
    {
        hb_id_event events[HB_EVENT_CAPACITY];
        uint64_t next, cursor[HB_EVENT_CONSUMERS];
        uint32_t overflow[HB_EVENT_CONSUMERS], stale[HB_EVENT_CONSUMERS];
        bool seen[5];
        uint64_t recognized_ids[HB_ID_PEOPLE];
        unsigned recognized_count;
        uint32_t session, encounter;
    } hb_event_bus;
    // Pure bounded core; platform owner must hold its lock for all calls.
    void hb_event_bus_init(hb_event_bus *bus);
    bool hb_event_push(hb_event_bus *bus, hb_id_event event);
    bool hb_event_pop(hb_event_bus *bus, hb_event_consumer consumer, uint32_t session,
                      uint64_t now_ms, hb_id_event *out);
    // Platform wrapper: independent consumers, no network work in producer.
    bool hb_identity_event_read(hb_event_consumer consumer, hb_id_event *out);
    bool hb_identity_event_current(const hb_id_event *event);
    void hb_identity_event_diagnostics(uint32_t overflow[HB_EVENT_CONSUMERS],
                                       uint32_t stale[HB_EVENT_CONSUMERS]);
#ifdef __cplusplus
}
#endif
