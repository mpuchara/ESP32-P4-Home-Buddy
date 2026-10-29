// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_identity_core.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum
    {
        HB_ENROLL_WAIT,
        HB_ENROLL_SAMPLE,
        HB_ENROLL_VERIFY,
        HB_ENROLL_CONFIRM
    } hb_enroll_step;
    // Invoked only after explicit BEGIN. Does not mutate committed profiles.
    bool hb_enroll_fresh(hb_id_database *db, hb_id_enrollment *en, uint64_t id);
    // Pool uses the existing 15 slots; progress counts mutually supported samples.
    // A full eight-sample set is compacted in place, without marking it verified.
    bool hb_enroll_sample_coherent(hb_id_enrollment *en, const hb_id_embedding *e, unsigned faces,
                                   bool alignment, uint32_t sequence, uint64_t frame_ms,
                                   const hb_id_policy *policy, unsigned *progress, bool *ready);
    hb_enroll_step hb_enroll_next(const hb_id_enrollment *en, unsigned faces, bool valid,
                                  uint32_t sequence, uint64_t frame_ms);
#ifdef __cplusplus
}
#endif
