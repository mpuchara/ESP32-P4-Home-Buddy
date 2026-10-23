// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_identity_events.h"
// Architectural contracts only. No voice capture, VAD, embedding or fusion runs yet.
// A future provider must copy/release capture leases before expensive inference,
// honor the shared compute mutex, cancellation epoch and bounded memory admission.
typedef struct
{
    uint32_t sample_rate, max_samples, capture_epoch;
    unsigned channels;
    const char *model_namespace;
    float acceptance_threshold, different_person_margin;
} hb_voice_provider_contract;
typedef struct
{
    hb_id_match face, voice;
    uint64_t face_frame_ms, voice_window_end_ms;
    uint32_t session;
} hb_identity_fusion_input;
// Future fusion must return AMBIGUOUS on distinct confirmed IDs; never choose
// whichever source has a higher raw similarity across different model scales.
// Voice templates require their own versioned schema/model namespace and storage
// capacity assessment; schema-1 face slots must not be repurposed silently.
