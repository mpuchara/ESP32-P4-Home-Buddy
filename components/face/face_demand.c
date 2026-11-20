// SPDX-License-Identifier: Apache-2.0
#include "hb_face_demand.h"
#include <stddef.h>
uint32_t hb_face_demand_remaining(const hb_face_demand *d, uint64_t now)
{
    if (!d || d->monitor_until_ms <= now)
        return 0;
    uint64_t remaining = d->monitor_until_ms - now;
    return remaining <= HB_FACE_MONITOR_MS ? (uint32_t)remaining : 0;
}
bool hb_face_demand_active(const hb_face_demand *d, uint64_t now)
{
    return d && (d->diagnostic || hb_face_demand_remaining(d, now));
}
bool hb_face_demand_monitor(hb_face_demand *d, bool enabled, uint64_t now)
{
    if (!d)
        return false;
    d->monitor_until_ms = 0;
    if (!enabled)
        return true;
    if (now > UINT64_MAX - HB_FACE_MONITOR_MS)
        return false;
    d->monitor_until_ms = now + HB_FACE_MONITOR_MS;
    return true;
}
void hb_face_demand_expire(hb_face_demand *d, uint64_t now)
{
    if (d && !hb_face_demand_remaining(d, now))
        d->monitor_until_ms = 0;
}
uint32_t hb_face_demand_period(const hb_face_demand *d)
{
    return d && d->diagnostic ? 500 : 1500;
}
