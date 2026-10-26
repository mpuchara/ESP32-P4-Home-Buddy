// SPDX-License-Identifier: Apache-2.0
#include "hb_frame_pool.h"
#include <limits.h>
#include <string.h>
#ifdef ESP_PLATFORM
#include "esp_attr.h"
#define HB_FRAME_ISR IRAM_ATTR __attribute__((noinline))
#else
#define HB_FRAME_ISR
#endif
void hb_frame_pool_init(hb_frame_pool *p)
{
    memset(p, 0, sizeof(*p));
    p->latest = -1;
}
int HB_FRAME_ISR hb_frame_reserve(hb_frame_pool *p)
{
    for (int i = 0; i < HB_CAMERA_SLOTS; i++)
        if (!p->slot[i].references && !p->slot[i].writing)
        {
            p->slot[i].writing = true;
            p->slot[i].issued = 0;
            p->slot[i].generation++;
            return i;
        }
    p->dropped++;
    return -1;
}
bool HB_FRAME_ISR hb_frame_abort(hb_frame_pool *p, int i)
{
    if (i < 0 || i >= HB_CAMERA_SLOTS || !p->slot[i].writing)
        return false;
    p->slot[i].writing = false;
    return true;
}
bool HB_FRAME_ISR hb_frame_publish(hb_frame_pool *p, int i)
{
    if (i < 0 || i >= HB_CAMERA_SLOTS || !p->slot[i].writing || p->slot[i].references)
        return false;
    if (p->latest >= 0)
        p->slot[p->latest].references--;
    p->slot[i].writing = false;
    p->slot[i].references = 1;
    p->latest = i;
    return true;
}
bool hb_frame_acquire(hb_frame_pool *p, hb_frame_lease *l)
{
    if (!l || p->latest < 0 || p->slot[p->latest].issued == UINT32_MAX)
        return false;
    hb_frame_slot *s = &p->slot[p->latest];
    uint32_t token = 1;
    while (s->issued & token)
        token <<= 1;
    s->issued |= token;
    s->leases |= token;
    s->references++;
    *l = (hb_frame_lease){p->latest, s->generation, token};
    return true;
}
bool hb_frame_release(hb_frame_pool *p, hb_frame_lease l)
{
    if (l.slot < 0 || l.slot >= HB_CAMERA_SLOTS)
        return false;
    hb_frame_slot *s = &p->slot[l.slot];
    uint32_t owner = p->latest == l.slot ? 1 : 0;
    if (s->generation != l.generation || s->writing || s->references <= owner || !l.token ||
        (l.token & (l.token - 1)) || !(s->leases & l.token))
        return false;
    s->leases &= ~l.token;
    s->references--;
    return true;
}

bool HB_FRAME_ISR hb_frame_complete(hb_frame_pool *p, int slot, size_t received, size_t expected)
{
    if (!expected || received != expected)
    {
        hb_frame_abort(p, slot);
        return false;
    }
    return hb_frame_publish(p, slot);
}
void hb_frame_pool_stop(hb_frame_pool *p)
{
    if (p->latest >= 0)
        p->slot[p->latest].references--;
    p->latest = -1;
    for (int i = 0; i < HB_CAMERA_SLOTS; i++)
        hb_frame_abort(p, i);
}
