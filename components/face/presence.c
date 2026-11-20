// SPDX-License-Identifier: Apache-2.0
#include "hb_presence.h"
#include <stddef.h>
void hb_presence_reset(hb_presence_tracker *t)
{
    if (t)
        *t = (hb_presence_tracker){.value = HB_PRESENCE_UNKNOWN};
}
bool hb_presence_observe(hb_presence_tracker *t, uint32_t seq, uint64_t frame, uint64_t completed,
                         bool detected, uint64_t now)
{
    if (!t || !frame || frame > completed || completed > now || now - frame > 2500 ||
        now - completed > 1200)
        return false;
    if (t->frame_ms && (seq == t->sequence || frame <= t->frame_ms || completed <= t->completed_ms))
        return false;
    if (t->completed_ms && completed - t->completed_ms > HB_PRESENCE_DATA_TIMEOUT_MS)
        hb_presence_reset(t);
    t->sequence = seq;
    t->frame_ms = frame;
    t->completed_ms = completed;
    if (detected)
    {
        t->misses = 0;
        t->streak_start_ms = 0;
        if (t->hits < HB_PRESENCE_ENTER_HITS)
            t->hits++;
        if (t->hits >= HB_PRESENCE_ENTER_HITS)
            t->value = HB_PRESENCE_PRESENT;
    }
    else
    {
        t->hits = 0;
        if (!t->misses)
            t->streak_start_ms = frame;
        if (t->misses < HB_PRESENCE_EXIT_MISSES)
            t->misses++;
        if (t->misses >= HB_PRESENCE_EXIT_MISSES &&
            frame - t->streak_start_ms >= HB_PRESENCE_EXIT_MS)
            t->value = HB_PRESENCE_ABSENT;
    }
    return true;
}
hb_presence_status hb_presence_read(const hb_presence_tracker *t, bool enabled, bool healthy,
                                    uint64_t now)
{
    hb_presence_status s = {.value = enabled ? HB_PRESENCE_UNKNOWN : HB_PRESENCE_OFF};
    if (!enabled || !healthy || !t || !t->completed_ms || t->completed_ms > now ||
        now - t->completed_ms > HB_PRESENCE_DATA_TIMEOUT_MS)
        return s;
    s.value = t->value;
    s.hits = t->hits;
    s.misses = t->misses;
    s.fresh = true;
    return s;
}
