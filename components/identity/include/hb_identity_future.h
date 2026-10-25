// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_identity_events.h"
// Independent providers; voice/fused publishing remains physically gated.
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
    bool face_confirmed, voice_confirmed;
    uint64_t face_frame_ms, voice_window_end_ms;
    uint32_t session;
} hb_identity_fusion_input;
typedef enum
{
    HB_FUSION_UNKNOWN,
    HB_FUSION_AMBIGUOUS,
    HB_FUSION_CONFIRMED,
    HB_FUSION_CONFLICT
} hb_identity_fusion_status;
typedef struct
{
    hb_identity_fusion_status status;
    hb_id_source source;
    uint64_t person_id;
} hb_identity_fusion_result;
// Future fusion must return CONFLICT on distinct confirmed IDs; never choose
// whichever source has a higher raw similarity across different model scales.
// Voice templates require their own versioned schema/model namespace and storage
// capacity assessment; schema-1 face slots must not be repurposed silently.

#ifdef __cplusplus
extern "C"
{
#endif
    hb_identity_fusion_result hb_identity_fuse(const hb_identity_fusion_input *input,
                                               uint32_t session, uint64_t now_ms,
                                               bool voice_physically_accepted);
#ifdef __cplusplus
}
#endif
