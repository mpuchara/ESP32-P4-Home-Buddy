// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
#define HB_PRESENCE_DATA_TIMEOUT_MS 3000
#define HB_PRESENCE_EXIT_MS 5000
#define HB_PRESENCE_ENTER_HITS 2
#define HB_PRESENCE_EXIT_MISSES 4
    typedef enum
    {
        HB_PRESENCE_OFF,
        HB_PRESENCE_UNKNOWN,
        HB_PRESENCE_PRESENT,
        HB_PRESENCE_ABSENT
    } hb_presence_value;
    typedef struct
    {
        hb_presence_value value;
        uint32_t sequence;
        uint64_t frame_ms, completed_ms, streak_start_ms;
        uint8_t hits, misses;
    } hb_presence_tracker;
    typedef struct
    {
        hb_presence_value value;
        uint8_t hits, misses;
        bool fresh;
    } hb_presence_status;
    void hb_presence_reset(hb_presence_tracker *tracker);
    // Consume only distinct, chronological, fresh completed analyses. No boxes/identity retained.
    bool hb_presence_observe(hb_presence_tracker *tracker, uint32_t sequence, uint64_t frame_ms,
                             uint64_t completed_ms, bool detected, uint64_t now_ms);
    hb_presence_status hb_presence_read(const hb_presence_tracker *tracker, bool enabled,
                                        bool healthy, uint64_t now_ms);
#ifdef __cplusplus
}
#endif
