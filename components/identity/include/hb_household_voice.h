// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_identity_core.h"
#include "hb_voice_core.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct
    {
        bool microphone, numerical, model, resources, speech_selection, consent;
    } hb_voice_acceptance;
    typedef enum
    {
        HB_VEN_IDLE,
        HB_VEN_COLLECT,
        HB_VEN_VERIFY,
        HB_VEN_READY,
        HB_VEN_SAVED,
        HB_VEN_CANCELLED,
        HB_VEN_FAILED
    } hb_voice_enrollment_state;
    typedef enum
    {
        HB_VEN_OK,
        HB_VEN_GATED,
        HB_VEN_PROFILE,
        HB_VEN_SESSION,
        HB_VEN_SAMPLE,
        HB_VEN_DUPLICATE_OR_INCONSISTENT,
        HB_VEN_MISMATCH,
        HB_VEN_TIMEOUT
    } hb_voice_enrollment_result;
    typedef struct
    {
        hb_voice_enrollment_state state;
        hb_voice_person staging;
        hb_voice_policy policy;
        uint32_t epoch;
        uint64_t deadline_ms, verified_end_ms;
    } hb_household_voice_enrollment;
    bool hb_voice_accepted(const hb_voice_acceptance *, const hb_voice_policy *);
    hb_voice_enrollment_result hb_household_voice_begin(hb_household_voice_enrollment *,
                                                        hb_id_database *, uint64_t id,
                                                        uint32_t epoch, uint64_t now,
                                                        const hb_voice_acceptance *,
                                                        const hb_voice_policy *);
    hb_voice_enrollment_result hb_household_voice_sample(hb_household_voice_enrollment *,
                                                         hb_id_database *,
                                                         const hb_voice_database *,
                                                         const hb_voice_embedding *, uint32_t epoch,
                                                         uint64_t end, uint64_t now, bool verify);
    hb_voice_enrollment_result hb_household_voice_commit(hb_household_voice_enrollment *,
                                                         hb_id_database *, hb_voice_database *,
                                                         uint32_t epoch, uint64_t now);
    void hb_household_voice_cancel(hb_household_voice_enrollment *);
    // Called by the single household owner after restore/deletion. Names stay face-owner metadata.
    void hb_household_voice_reconcile(hb_id_database *, hb_voice_database *);
    unsigned hb_household_voice_count(const hb_voice_database *, uint64_t id);
    hb_voice_match hb_household_voice_identify(hb_id_database *, const hb_voice_database *,
                                               const hb_voice_embedding *,
                                               const hb_voice_acceptance *, const hb_voice_policy *,
                                               hb_voice_tracker *, uint32_t epoch, uint64_t end,
                                               uint64_t now, bool *confirmed);
    const char *hb_household_voice_error(hb_voice_enrollment_result);
#ifdef __cplusplus
}
#endif
