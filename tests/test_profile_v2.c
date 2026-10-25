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
    if (off + n > sizeof(flash))
        return false;
    if (calls++ == fail)
    {
        memset(flash + off, 255, tear < n ? tear : n);
        return false;
    }
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
static void boot_lifecycle(void)
{
    hb_profile_io io = {NULL,    sizeof(flash), HB_PROFILE_STORAGE_LOCAL_UNENCRYPTED,
                        read_io, write_io,      erase_io,
                        hash_io};
    hb_profile_revision rev = {0, -1};
    memset(flash, 255, sizeof(flash));
    assert(hb_profile_v2_boot(&io, &db, &scratch, &rev, 123) == HB_STORE_EMPTY);
    assert(hb_profile_v2_valid(&db) && rev.bank == -1);
    // A torn first payload, despite erased header, is never a fresh empty database.
    flash[HB_PROFILE_V2_BANK + HB_PROFILE_V2_HEADER + 42] = 0;
    memcpy(backup, flash, sizeof(flash));
    assert(hb_profile_v2_boot(&io, &db, &scratch, &rev, 123) == HB_STORE_CORRUPT);
    assert(!memcmp(backup, flash, sizeof(flash)));
    memset(flash, 255, sizeof(flash));
    uint64_t id = hb_id_create(&db.face, "Boot synthetic");
    assert(id);
    db.face.people[0].count = 1;
    db.face.people[0].templates[0].v[0] = 1;
    db.voice.people[0].id = id;
    db.voice.people[0].count = 1;
    db.voice.people[0].samples[0].v[0] = 1;
    assert(hb_profile_v2_commit(&io, &db, &scratch, &rev, true) == HB_STORE_OK);
    assert(rev.bank == 1 && rev.generation == 1);
    memset(&restored, 0, sizeof(restored));
    assert(hb_profile_v2_boot(&io, &restored, &scratch, &rev, 999) == HB_STORE_OK);
    assert(restored.face.people[0].id == id && restored.voice.people[0].id == id);
    assert(!strcmp(restored.face.people[0].name, "Boot synthetic"));
    assert(restored.face.people[0].templates[0].v[0] == 1);
    assert(restored.voice.people[0].samples[0].v[0] == 1);
    // Rename and disabled state survive serialization without changing either modality.
    assert(hb_id_rename(&db.face, id, "Renamed synthetic"));
    db.face.people[0].enabled = false;
    assert(hb_profile_v2_commit(&io, &db, &scratch, &rev, true) == HB_STORE_OK);
    assert(rev.bank == 0 && rev.generation == 2);
    assert(hb_profile_v2_boot(&io, &restored, &scratch, &rev, 999) == HB_STORE_OK);
    assert(!restored.face.people[0].enabled && restored.face.people[0].id == id);
    assert(!strcmp(restored.face.people[0].name, "Renamed synthetic"));
    assert(restored.voice.people[0].samples[0].v[0] == 1);
    // Unknown namespace/schema newest generation falls back to the older valid bank.
    flash[4] = 99;
    assert(hb_profile_v2_boot(&io, &restored, &scratch, &rev, 999) == HB_STORE_OK);
    assert(rev.bank == 1 && rev.generation == 1);
    flash[HB_PROFILE_V2_BANK + 104] ^= 1;
    memcpy(backup, flash, sizeof(flash));
    assert(hb_profile_v2_boot(&io, &restored, &scratch, &rev, 999) == HB_STORE_INCOMPATIBLE);
    assert(!memcmp(backup, flash, sizeof(flash)));
    memset(flash, 255, sizeof(flash));
    flash[12345] = 42;
    assert(hb_profile_v2_boot(&io, &restored, &scratch, &rev, 999) == HB_STORE_CORRUPT);
    io.storage_mode = (hb_profile_storage_mode)99;
    assert(hb_profile_v2_boot(&io, &restored, &scratch, &rev, 999) == HB_STORE_LOCKED);
    puts("local unencrypted boot/lifecycle/namespace/unknown/torn initialization passed");
}
int main(void)
{
    boot_lifecycle();
    hb_profile_io io = {NULL,    sizeof(flash), HB_PROFILE_STORAGE_HARDWARE_PROTECTED,
                        read_io, write_io,      erase_io,
                        hash_io};
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
    const size_t tears[] = {0, 1, 15, 16, 127, 128, 255, 256, 257, 312415, 312416};
    for (int step = 0; step < 6; ++step)
        for (unsigned cut = 0; cut < sizeof(tears) / sizeof(tears[0]); ++cut)
        {
            memcpy(flash, backup, sizeof(flash));
            rev = good;
            calls = 0;
            fail = step;
            tear = tears[cut];
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
    io.storage_mode = HB_PROFILE_STORAGE_RAM_ONLY;
    assert(hb_profile_v2_restore(&io, &restored, &scratch, &rev) == HB_STORE_LOCKED);
    printf("schema2 face=%zu voice=%zu payload=%zu header=%u bank_headroom=%zu; "
           "migration/fault/capacity/scrub passed\n",
           sizeof(db.face), sizeof(db.voice), sizeof(db), HB_PROFILE_V2_HEADER,
           HB_PROFILE_V2_BANK - HB_PROFILE_V2_HEADER - sizeof(db));
}
