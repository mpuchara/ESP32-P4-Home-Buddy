// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_profile_store.h"
#include "hb_voice_core.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define HB_PROFILE_V2_BANK 0x80000U
#define HB_PROFILE_V2_HEADER 256U
    typedef struct
    {
        hb_id_database face;
        hb_voice_database voice;
        uint64_t reserved; // Explicit 16-byte write alignment, must remain zero.
    } hb_household_snapshot;
    bool hb_profile_v2_valid(const hb_household_snapshot *);
    // Runtime boot: restore schema 2, migrate validated legacy only into blank upper bank.
    // On fully blank storage, commit an empty fallback before allowing enrollment.
    hb_store_result hb_profile_v2_boot(const hb_profile_io *, hb_household_snapshot *,
                                       hb_household_snapshot *, hb_profile_revision *,
                                       uint32_t seed);
    hb_store_result hb_profile_v2_restore(const hb_profile_io *, hb_household_snapshot *,
                                          hb_household_snapshot *, hb_profile_revision *);
    hb_store_result hb_profile_v2_commit(const hb_profile_io *, const hb_household_snapshot *,
                                         hb_household_snapshot *, hb_profile_revision *,
                                         bool authorized);
    hb_store_result hb_profile_v2_scrub(const hb_profile_io *, const hb_profile_revision *,
                                        bool authorized);
    // Explicit migration starts at bank 1 (0x80000), leaving BOTH legacy banks intact.
    // Dirty upper bank requires offline recovery; it is never automatically erased.
    hb_store_result hb_profile_v2_migrate(const hb_profile_io *, hb_household_snapshot *,
                                          hb_household_snapshot *, hb_profile_revision *,
                                          bool authorized);
#ifdef __cplusplus
}
#endif
