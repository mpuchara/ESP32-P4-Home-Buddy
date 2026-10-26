// SPDX-License-Identifier: Apache-2.0
#include "hb_profile_store_v2.h"
#include <string.h>

// Schema 2 is a separately gated ESP32-P4 ABI snapshot. Models are independent;
// a future ABI/model must migrate explicitly, never reinterpret older embeddings.
_Static_assert(sizeof(hb_household_snapshot) + HB_PROFILE_V2_HEADER <= HB_PROFILE_V2_BANK,
               "Profile snapshot exceeds atomic bank");
_Static_assert(sizeof(hb_household_snapshot) % 16 == 0, "Encrypted writes need 16 byte alignment");
_Static_assert(sizeof(hb_household_snapshot) == 312416,
               "Schema 2 ABI changed; do not reinterpret saved profiles");
static uint32_t u32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t v)
{
    for (unsigned i = 0; i < 4; i++)
        p[i] = (uint8_t)(v >> (i * 8));
}
static uint64_t u64(const uint8_t *p)
{
    return (uint64_t)u32(p) | (uint64_t)u32(p + 4) << 32;
}
static void put64(uint8_t *p, uint64_t v)
{
    put32(p, (uint32_t)v);
    put32(p + 4, (uint32_t)(v >> 32));
}
static hb_store_result admission(const hb_profile_io *io)
{
    if (!io || (io->storage_mode != HB_PROFILE_STORAGE_LOCAL_UNENCRYPTED &&
                io->storage_mode != HB_PROFILE_STORAGE_HARDWARE_PROTECTED))
        return HB_STORE_LOCKED;
    if (!io->read || !io->write || !io->erase || !io->sha256)
        return HB_STORE_IO;
    return io->size == HB_PROFILE_PARTITION ? HB_STORE_OK : HB_STORE_CAPACITY;
}
bool hb_profile_v2_valid(const hb_household_snapshot *db)
{
    if (!db || db->reserved || !hb_profile_database_valid(&db->face) ||
        !hb_voice_database_valid(&db->voice))
        return false;
    for (unsigned i = 0; i < HB_VOICE_PEOPLE; ++i)
    {
        const hb_voice_person *v = &db->voice.people[i];
        if (!v->id)
            continue;
        bool found = false;
        for (unsigned j = 0; j < HB_ID_PEOPLE; ++j)
            found |= db->face.people[j].id == v->id;
        if (!found)
            return false;
    }
    return true;
}
static hb_store_result bank_read(const hb_profile_io *io, int bank, hb_household_snapshot *db,
                                 uint64_t *generation)
{
    uint8_t h[HB_PROFILE_V2_HEADER], digest[32], header_digest[32];
    size_t offset = (size_t)bank * HB_PROFILE_V2_BANK;
    if (!io->read(io->context, offset, h, sizeof(h)))
        return HB_STORE_IO;
    bool blank = true;
    for (size_t i = 0; i < sizeof(h); i++)
        blank &= h[i] == 0xff;
    if (blank)
    {
        uint8_t block[256];
        for (size_t o = sizeof(h); o < HB_PROFILE_V2_BANK; o += sizeof(block))
        {
            if (!io->read(io->context, offset + o, block, sizeof(block)))
                return HB_STORE_IO;
            for (size_t i = 0; i < sizeof(block); ++i)
                if (block[i] != 0xff)
                    return HB_STORE_CORRUPT;
        }
        return HB_STORE_EMPTY;
    }
    if (memcmp(h, "HBP2", 4) || u32(h + 4) != 2 || u32(h + 8) != sizeof(*db) ||
        u32(h + 12) != HB_ID_DIM ||
        (memcmp(h + 56, HB_ID_MODEL, sizeof(HB_ID_MODEL)) ||
         memcmp(h + 104, HB_VOICE_MODEL, sizeof(HB_VOICE_MODEL))))
        return HB_STORE_INCOMPATIBLE;
    // Protect generation and metadata as well as payload against accidental corruption.
    if (!io->sha256(h, 224, header_digest) || memcmp(h + 224, header_digest, 32))
        return HB_STORE_CORRUPT;
    if (!io->read(io->context, offset + sizeof(h), db, sizeof(*db)))
        return HB_STORE_IO;
    if (!io->sha256(db, sizeof(*db), digest) || memcmp(h + 24, digest, 32) ||
        !hb_profile_v2_valid(db))
        return HB_STORE_CORRUPT;
    *generation = u64(h + 16);
    return *generation ? HB_STORE_OK : HB_STORE_CORRUPT;
}
hb_store_result hb_profile_v2_restore(const hb_profile_io *io, hb_household_snapshot *db,
                                      hb_household_snapshot *scratch, hb_profile_revision *revision)
{
    hb_store_result a = admission(io);
    if (a != HB_STORE_OK)
        return a;
    if (!db || !scratch || !revision || db == scratch)
        return HB_STORE_IO;
    hb_profile_revision best = {0, -1};
    hb_store_result failure = HB_STORE_EMPTY;
    for (int bank = 0; bank < 2; bank++)
    {
        uint64_t generation = 0;
        hb_store_result r = bank_read(io, bank, scratch, &generation);
        if (r == HB_STORE_OK && generation > best.generation)
        {
            memcpy(db, scratch, sizeof(*db));
            best = (hb_profile_revision){generation, bank};
        }
        else if (r != HB_STORE_OK && r != HB_STORE_EMPTY)
            failure = r;
    }
    memset(scratch, 0, sizeof(*scratch));
    *revision = best;
    return best.bank >= 0 ? HB_STORE_OK : failure;
}
hb_store_result hb_profile_v2_commit(const hb_profile_io *io, const hb_household_snapshot *db,
                                     hb_household_snapshot *scratch, hb_profile_revision *revision,
                                     bool authorized)
{
    if (!authorized)
        return HB_STORE_LOCKED;
    hb_store_result r = admission(io);
    if (r != HB_STORE_OK)
        return r;
    if (!revision || revision->bank < -1 || revision->bank > 1 ||
        revision->generation == UINT64_MAX || !scratch || db == scratch || !hb_profile_v2_valid(db))
        return HB_STORE_INCOMPATIBLE;
    memcpy(scratch, db, sizeof(*db));
    for (unsigned i = 0; i < HB_ID_PEOPLE; i++)
    {
        hb_id_person *p = &scratch->face.people[i];
        p->last_greeting_ms = 0;
        p->last_sample_ms = 0;
        p->last_sample_sequence = 0;
    }
    for (unsigned i = 0; i < HB_VOICE_PEOPLE; ++i)
        scratch->voice.people[i].last_end = 0;
    uint8_t h[HB_PROFILE_V2_HEADER] = {0};
    memcpy(h, "HBP2", 4);
    put32(h + 4, 2);
    put32(h + 8, sizeof(*db));
    put32(h + 12, HB_ID_DIM);
    put64(h + 16, revision->generation + 1);
    memcpy(h + 56, HB_ID_MODEL, sizeof(HB_ID_MODEL));
    memcpy(h + 104, HB_VOICE_MODEL, sizeof(HB_VOICE_MODEL));
    int target = revision->bank <= 0 ? 1 : 0; // First commit preserves BOTH schema-1 banks.
    size_t offset = (size_t)target * HB_PROFILE_V2_BANK;
    // Fresh initialization and migration may only touch a verified blank target.
    if (revision->bank == -1)
    {
        uint8_t block[256];
        for (size_t o = 0; o < HB_PROFILE_V2_BANK; o += sizeof(block))
        {
            if (!io->read(io->context, offset + o, block, sizeof(block)))
                return HB_STORE_IO;
            for (size_t i = 0; i < sizeof(block); ++i)
                if (block[i] != 0xff)
                    return HB_STORE_INCOMPATIBLE;
        }
    }
    bool ok = io->sha256(scratch, sizeof(*scratch), h + 24) && io->sha256(h, 224, h + 224);
    // Bound each Flash operation. The device adapter yields between sectors/chunks;
    // identity worker ownership and the compute lease span the entire transaction.
    for (size_t o = 0; ok && o < HB_PROFILE_V2_BANK; o += 4096)
        ok = io->erase(io->context, offset + o, 4096);
    for (size_t o = 0; ok && o < sizeof(*scratch); o += 4096)
    {
        size_t n = sizeof(*scratch) - o;
        if (n > 4096)
            n = 4096;
        ok = io->write(io->context, offset + sizeof(h) + o, (const uint8_t *)scratch + o, n);
    }
    // Read back payload before writing the final commit header.
    uint8_t digest[32];
    if (ok)
        ok = io->read(io->context, offset + sizeof(h), scratch, sizeof(*scratch)) &&
             io->sha256(scratch, sizeof(*scratch), digest) && !memcmp(h + 24, digest, 32) &&
             io->write(io->context, offset, h, sizeof(h));
    uint64_t generation = 0;
    if (ok)
        ok = bank_read(io, target, scratch, &generation) == HB_STORE_OK &&
             generation == revision->generation + 1;
    memset(scratch, 0, sizeof(*scratch));
    if (!ok)
        return HB_STORE_IO;
    *revision = (hb_profile_revision){generation, target};
    return HB_STORE_OK;
}
hb_store_result hb_profile_v2_scrub(const hb_profile_io *io, const hb_profile_revision *revision,
                                    bool authorized)
{
    if (!authorized)
        return HB_STORE_LOCKED;
    hb_store_result r = admission(io);
    if (r != HB_STORE_OK)
        return r;
    if (!revision || revision->bank < 0 || revision->bank > 1)
        return HB_STORE_IO;
    return io->erase(io->context, (size_t)(1 - revision->bank) * HB_PROFILE_V2_BANK,
                     HB_PROFILE_V2_BANK)
               ? HB_STORE_OK
               : HB_STORE_IO;
}

hb_store_result hb_profile_v2_migrate(const hb_profile_io *io, hb_household_snapshot *out,
                                      hb_household_snapshot *scratch, hb_profile_revision *revision,
                                      bool authorized)
{
    if (!authorized)
        return HB_STORE_LOCKED;
    if (!out || !scratch || out == scratch || !revision)
        return HB_STORE_IO;
    hb_profile_revision existing = {0, -1};
    hb_store_result r = hb_profile_v2_restore(io, out, scratch, &existing);
    if (r == HB_STORE_OK)
    {
        *revision = existing;
        return HB_STORE_OK;
    }
    // Only a blank upper bank is eligible. Never overwrite an unknown/torn format.
    uint8_t block[256];
    if (admission(io) != HB_STORE_OK)
        return admission(io);
    for (size_t offset = HB_PROFILE_V2_BANK; offset < HB_PROFILE_PARTITION; offset += sizeof(block))
    {
        if (!io->read(io->context, offset, block, sizeof(block)))
            return HB_STORE_IO;
        for (unsigned i = 0; i < sizeof(block); ++i)
            if (block[i] != 0xff)
                return HB_STORE_INCOMPATIBLE;
    }
    memset(out, 0, sizeof(*out));
    hb_profile_revision legacy = {0, -1};
    r = hb_profile_restore(io, &out->face, &scratch->face, &legacy);
    if (r != HB_STORE_OK)
        return r;
    hb_voice_database_init(&out->voice);
    *revision = (hb_profile_revision){0, -1};
    return hb_profile_v2_commit(io, out, scratch, revision, true);
}

// Does not create an empty database over unrecognized or partially written data.
hb_store_result hb_profile_v2_boot(const hb_profile_io *io, hb_household_snapshot *db,
                                   hb_household_snapshot *scratch, hb_profile_revision *rev,
                                   uint32_t seed)
{
    hb_store_result r = hb_profile_v2_restore(io, db, scratch, rev);
    if (r == HB_STORE_OK)
        return r;
    if (r == HB_STORE_EMPTY)
    {
        memset(db, 0, sizeof(*db));
        hb_id_database_init(&db->face, seed);
        hb_voice_database_init(&db->voice);
        // Both entire banks were verified erased by restore. Establish an empty
        // fallback BEFORE enrollment can capture user templates. An interrupted
        // bootstrap still fails closed; never erase its unknown partial bytes.
        return hb_profile_v2_commit(io, db, scratch, rev, true);
    }
    if (r == HB_STORE_LOCKED || r == HB_STORE_CAPACITY || r == HB_STORE_IO)
        return r;
    // A schema-2 marker anywhere prevents reinterpretation as legacy.
    uint8_t magic[4];
    bool legacy = false;
    for (unsigned bank = 0; bank < 4; ++bank)
    {
        if (!io->read(io->context, bank * HB_PROFILE_BANK, magic, sizeof(magic)))
            return HB_STORE_IO;
        if (!memcmp(magic, "HBP2", 4))
            return r;
        legacy |= bank < 2 && !memcmp(magic, "HBP1", 4);
    }
    return legacy ? hb_profile_v2_migrate(io, db, scratch, rev, true) : r;
}
