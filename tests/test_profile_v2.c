// SPDX-License-Identifier: Apache-2.0
#include "hb_profile_store_v2.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t flash[HB_PROFILE_PARTITION], backup[HB_PROFILE_PARTITION];
static hb_household_snapshot db, scratch, restored;
static int fail = -1, calls;
static size_t tear;
static bool migration_tear;
static bool read_io(void *ctx, size_t off, void *out, size_t n)
{
    (void)ctx;
    if (off + n > sizeof(flash) || calls++ == fail)
        return false;
    memcpy(out, flash + off, n);
    return true;
}
static bool write_io(void *ctx, size_t off, const void *in, size_t n)
{
    (void)ctx;
    if (off + n > sizeof(flash))
        return false;
    if (migration_tear && off == HB_PROFILE_V2_BANK + HB_PROFILE_V2_HEADER)
    {
        memcpy(flash + off, in, 32);
        return false;
    }
    if (calls++ == fail)
    {
        if (tear < n)
            memcpy(flash + off, in, tear);
        return false;
    }
    memcpy(flash + off, in, n);
    return true;
}
static bool erase_io(void *ctx, size_t off, size_t n)
{
    (void)ctx;
    if (off + n > sizeof(flash) || calls++ == fail)
        return false;
    memset(flash + off, 255, n);
    return true;
}
// Fault-protocol checksum only. Firmware supplies SHA256; not a cryptographic test.
static bool hash_io(const void *in, size_t n, uint8_t out[32])
{
    const uint8_t *p = in;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; ++i)
        h = (h ^ p[i]) * 16777619u;
    for (unsigned i = 0; i < 32; ++i)
    {
        h = h * 1664525u + 1013904223u;
        out[i] = (uint8_t)(h >> 24);
    }
    return true;
}
int main(void)
{
    hb_profile_io io = {NULL, sizeof(flash), true, read_io, write_io, erase_io, hash_io};
    memset(flash, 255, sizeof(flash));
    hb_id_database_init(&db.face, 42);
    hb_voice_database_init(&db.voice);
    uint64_t id = hb_id_create(&db.face, "Synthetic");
    assert(id);
    hb_profile_revision rev = {0, -1};
    assert(hb_profile_v2_commit(&io, &db, &scratch, &rev, false) == HB_STORE_LOCKED);
    hb_profile_revision legacy = {0, -1};
    assert(hb_profile_commit(&io, &db.face, &scratch.face, &legacy) == HB_STORE_OK);
    assert(hb_id_rename(&db.face, id, "Latest"));
    assert(hb_profile_commit(&io, &db.face, &scratch.face, &legacy) == HB_STORE_OK);
    memcpy(backup, flash, sizeof(flash));
    assert(hb_profile_v2_migrate(&io, &db, &scratch, &rev, false) == HB_STORE_LOCKED);
    migration_tear = true;
    assert(hb_profile_v2_migrate(&io, &db, &scratch, &rev, true) == HB_STORE_IO);
    migration_tear = false;
    assert(!memcmp(flash, backup, HB_PROFILE_V2_BANK));
    assert(hb_profile_restore(&io, &restored.face, &scratch.face, &legacy) == HB_STORE_OK);
    assert(!strcmp(restored.face.people[0].name, "Latest"));
    assert(hb_profile_v2_migrate(&io, &db, &scratch, &rev, true) == HB_STORE_INCOMPATIBLE);
    memcpy(flash, backup, sizeof(flash)); // Explicit synthetic offline recovery.
    assert(hb_profile_v2_migrate(&io, &db, &scratch, &rev, true) == HB_STORE_OK);
    assert(rev.bank == 1 && !memcmp(flash, backup, HB_PROFILE_V2_BANK));
    assert(!strcmp(db.face.people[0].name, "Latest") && db.face.people[0].id == id);
    assert(hb_profile_v2_restore(&io, &restored, &scratch, &rev) == HB_STORE_OK);
    assert(restored.voice.schema == 1 && !restored.voice.people[0].id);
    // Fill every person and both template capacities.
    for (unsigned i = 0; i < HB_ID_PEOPLE; ++i)
    {
        if (i)
            assert(hb_id_create(&db.face, "Synthetic"));
        db.face.people[i].count = HB_ID_TEMPLATES;
        db.voice.people[i].id = db.face.people[i].id;
        db.voice.people[i].count = HB_VOICE_TEMPLATES;
        for (unsigned j = 0; j < HB_ID_TEMPLATES; ++j)
            db.face.people[i].templates[j].v[0] = 1;
        for (unsigned j = 0; j < HB_VOICE_TEMPLATES; ++j)
            db.voice.people[i].samples[j].v[0] = 1;
    }
    assert(hb_profile_v2_commit(&io, &db, &scratch, &rev, true) == HB_STORE_OK);
    assert(hb_profile_v2_restore(&io, &restored, &scratch, &rev) == HB_STORE_OK);
    assert(restored.voice.people[7].count == 4 && restored.face.people[7].count == 15);
    memcpy(backup, flash, sizeof(flash));
    hb_profile_revision good = rev;
    assert(hb_id_rename(&db.face, id, "Changed"));
    for (int step = 0; step < 6; ++step)
        for (size_t part = 0; part <= 128; part += 16)
        {
            memcpy(flash, backup, sizeof(flash));
            rev = good;
            calls = 0;
            fail = step;
            tear = part;
            assert(hb_profile_v2_commit(&io, &db, &scratch, &rev, true) == HB_STORE_IO);
            fail = -1;
            assert(hb_profile_v2_restore(&io, &restored, &scratch, &rev) == HB_STORE_OK);
            assert(restored.face.people[0].id == id);
        }
    memcpy(flash, backup, sizeof(flash));
    rev = good;
    flash[good.bank * HB_PROFILE_V2_BANK + HB_PROFILE_V2_HEADER + 10] ^= 1;
    assert(hb_profile_v2_restore(&io, &restored, &scratch, &rev) == HB_STORE_OK &&
           rev.bank != good.bank);
    memcpy(flash, backup, sizeof(flash));
    rev = good;
    assert(hb_id_delete(&db.face, id));
    memset(&db.voice.people[0], 0, sizeof(db.voice.people[0]));
    assert(hb_profile_v2_commit(&io, &db, &scratch, &rev, true) == HB_STORE_OK);
    assert(hb_profile_v2_scrub(&io, &rev, false) == HB_STORE_LOCKED);
    assert(hb_profile_v2_scrub(&io, &rev, true) == HB_STORE_OK);
    assert(hb_profile_v2_restore(&io, &restored, &scratch, &rev) == HB_STORE_OK);
    assert(!hb_id_find(&restored.face, id) && !restored.voice.people[0].count);
    db.voice.model[0] ^= 1;
    assert(!hb_profile_v2_valid(&db));
    db.voice.model[0] ^= 1;
    db.voice.people[0].id = id;
    assert(!hb_profile_v2_valid(&db));
    io.size--;
    assert(hb_profile_v2_restore(&io, &restored, &scratch, &rev) == HB_STORE_CAPACITY);
    io.size++;
    io.protected_storage = false;
    assert(hb_profile_v2_restore(&io, &restored, &scratch, &rev) == HB_STORE_LOCKED);
    printf("schema2 face=%zu voice=%zu payload=%zu header=%u bank_headroom=%zu; "
           "migration/fault/capacity/scrub passed\n",
           sizeof(db.face), sizeof(db.voice), sizeof(db), HB_PROFILE_V2_HEADER,
           HB_PROFILE_V2_BANK - HB_PROFILE_V2_HEADER - sizeof(db));
}
