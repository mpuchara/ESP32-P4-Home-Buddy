// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
#define HB_FACE_MONITOR_MS 600000U
    typedef struct
    {
        bool diagnostic;
        uint64_t monitor_until_ms;
    } hb_face_demand;
    bool hb_face_demand_active(const hb_face_demand *d, uint64_t now);
    uint32_t hb_face_demand_remaining(const hb_face_demand *d, uint64_t now);
    bool hb_face_demand_monitor(hb_face_demand *d, bool enabled, uint64_t now);
    void hb_face_demand_expire(hb_face_demand *d, uint64_t now);
    uint32_t hb_face_demand_period(const hb_face_demand *d);
#ifdef __cplusplus
}
#endif
