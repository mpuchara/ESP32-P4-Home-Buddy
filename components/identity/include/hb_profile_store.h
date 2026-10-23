// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_identity_core.h"
#ifdef __cplusplus
extern "C"
{
#endif
// Two 256 KiB erase-aligned banks. Caller serializes operations in identity worker.
#define HB_PROFILE_BANK 0x40000U
#define HB_PROFILE_HEADER 128U
#define HB_PROFILE_PARTITION 0x100000U
    typedef struct
    {
        void *context;
        size_t size;
        bool protected_storage;
        bool (*read)(void *, size_t, void *, size_t);
        bool (*write)(void *, size_t, const void *, size_t);
        bool (*erase)(void *, size_t, size_t);
        bool (*sha256)(const void *, size_t, uint8_t[32]);
    } hb_profile_io;
    typedef enum
    {
        HB_STORE_OK,
        HB_STORE_EMPTY,
        HB_STORE_LOCKED,
        HB_STORE_IO,
        HB_STORE_CORRUPT,
        HB_STORE_INCOMPATIBLE,
        HB_STORE_CAPACITY
    } hb_store_result;
    typedef struct
    {
        uint64_t generation;
        int bank;
    } hb_profile_revision;
    // scratch is a second database allocated in PSRAM, never on a task stack.
    hb_store_result hb_profile_restore(const hb_profile_io *, hb_id_database *, hb_id_database *,
                                       hb_profile_revision *);
    hb_store_result hb_profile_commit(const hb_profile_io *, const hb_id_database *,
                                      hb_id_database *, hb_profile_revision *);
    // Erases obsolete bank after a deletion. Active committed bank remains recoverable.
    hb_store_result hb_profile_scrub(const hb_profile_io *, const hb_profile_revision *);
    bool hb_profile_database_valid(const hb_id_database *);
#ifdef __cplusplus
}
#endif
