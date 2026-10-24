// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_identity.h"
static inline bool hb_id_background(hb_id_mode mode)
{
    return mode == HB_ID_BACKGROUND || mode == HB_ID_AUTO;
}
static inline bool hb_id_expired(hb_id_mode mode, uint64_t deadline, uint64_t now)
{
    return mode != HB_ID_STOP && mode != HB_ID_AUTO && now >= deadline;
}
static inline bool hb_id_auto_ready(bool enabled, unsigned usable_profiles)
{
    return enabled && usable_profiles > 0;
}
