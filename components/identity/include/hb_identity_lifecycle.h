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

typedef enum
{
    HB_ID_MODELS_RUN,
    HB_ID_MODELS_PARK,
    HB_ID_MODELS_RELEASE
} hb_id_model_action;
// Disabling a member changes matching eligibility, not the owner's AUTO preference.
// Retain at most the existing model set across that administrative pause. Explicit
// Stop/cancel/deletion clears profile_pause and must still release resources.
static inline hb_id_model_action hb_id_model_next(hb_id_mode mode, bool usable, bool auto_enabled,
                                                  bool profile_pause, bool loaded)
{
    if (mode != HB_ID_STOP && (mode == HB_ID_ENROLL || usable))
        return HB_ID_MODELS_RUN;
    if (mode == HB_ID_STOP && auto_enabled && profile_pause && loaded)
        return HB_ID_MODELS_PARK;
    return HB_ID_MODELS_RELEASE;
}

static inline hb_id_state hb_id_detection_failure(bool detection_valid, unsigned faces)
{
    return detection_valid && faces > 1 ? HB_ID_AMBIGUOUS : HB_ID_UNAVAILABLE;
}
