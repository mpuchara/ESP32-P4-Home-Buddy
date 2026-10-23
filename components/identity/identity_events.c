// SPDX-License-Identifier: Apache-2.0
#include "hb_identity_events.h"
#include <string.h>
void hb_event_bus_init(hb_event_bus *bus)
{
    memset(bus, 0, sizeof(*bus));
    bus->next = 1;
}
bool hb_event_push(hb_event_bus *b, hb_id_event e)
{
    if (!b || (unsigned)e.kind > HB_ID_EVENT_LEFT || e.source != HB_ID_SOURCE_FACE || !e.session ||
        !e.sequence || !e.frame_ms || e.timestamp_ms < e.frame_ms ||
        e.timestamp_ms - e.frame_ms > 5000 ||
        (e.kind == HB_ID_EVENT_RECOGNIZED &&
         (!e.confirmed || !e.person_id || e.state != HB_ID_KNOWN)))
        return false;
    if (b->session != e.session)
    {
        memset(b->seen, 0, sizeof(b->seen));
        b->recognized_count = 0;
        b->session = e.session;
        ++b->encounter;
    }
    if (e.kind == HB_ID_EVENT_ENTERED)
    {
        if (b->seen[HB_ID_EVENT_ENTERED])
            return false;
    }
    else if (e.kind == HB_ID_EVENT_RECOGNIZED)
    {
        for (unsigned i = 0; i < b->recognized_count; i++)
            if (b->recognized_ids[i] == e.person_id)
                return false;
        if (b->recognized_count >= HB_ID_PEOPLE)
            return false;
        b->seen[HB_ID_EVENT_RECOGNIZED] = false;
    }
    if (e.kind == HB_ID_EVENT_LEFT && !b->seen[HB_ID_EVENT_ENTERED])
        return false;
    if (b->seen[e.kind])
        return false;
    b->seen[e.kind] = true;
    if (e.kind == HB_ID_EVENT_RECOGNIZED)
        b->recognized_ids[b->recognized_count++] = e.person_id;
    if (b->next == UINT64_MAX)
        return false;
    e.event_id = b->next++;
    e.encounter = b->encounter;
    b->events[(e.event_id - 1) % HB_EVENT_CAPACITY] = e;
    for (unsigned i = 0; i < HB_EVENT_CONSUMERS; i++)
    {
        if (e.event_id - b->cursor[i] > HB_EVENT_CAPACITY)
        {
            ++b->cursor[i];
            ++b->overflow[i];
        }
    }
    if (e.kind == HB_ID_EVENT_LEFT)
    {
        memset(b->seen, 0, sizeof(b->seen));
        b->recognized_count = 0;
        ++b->encounter;
    }
    return true;
}
bool hb_event_pop(hb_event_bus *b, hb_event_consumer c, uint32_t session, uint64_t now,
                  hb_id_event *out)
{
    if (!b || !out || (unsigned)c >= HB_EVENT_CONSUMERS)
        return false;
    while (b->cursor[c] + 1 < b->next)
    {
        uint64_t id = ++b->cursor[c];
        const hb_id_event *e = &b->events[(id - 1) % HB_EVENT_CAPACITY];
        if (e->event_id != id || e->session != session || e->timestamp_ms > now ||
            e->frame_ms > now || now - e->frame_ms > 5000)
        {
            ++b->stale[c];
            continue;
        }
        *out = *e;
        return true;
    }
    return false;
}
