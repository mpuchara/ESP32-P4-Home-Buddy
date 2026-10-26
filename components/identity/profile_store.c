// SPDX-License-Identifier: Apache-2.0
#include "hb_profile_store.h"
#include <string.h>

const char *hb_profile_result_text(hb_store_result result)
{
    switch (result)
    {
    case HB_STORE_OK:
        return "Zapis prawidłowy";
    case HB_STORE_EMPTY:
        return "Pusta baza lokalna, bez szyfrowania";
    case HB_STORE_LOCKED:
        return "Polityka pamięci blokuje dostęp";
    case HB_STORE_IO:
        return "Błąd odczytu lub zapisu Flash";
    case HB_STORE_CORRUPT:
        return "Nieprawidłowy lub przerwany zapis bazy";
    case HB_STORE_INCOMPATIBLE:
        return "Niezgodny format lub model bazy";
    case HB_STORE_CAPACITY:
        return "Niezgodny rozmiar partycji profili";
    default:
        return "Nieznany błąd pamięci";
    }
}

// Schema 1 is an ESP32-P4 little-endian ABI snapshot. ABI and model are pinned;
// a future ABI/model must migrate explicitly, never reinterpret older embeddings.
_Static_assert(sizeof(hb_id_database) + HB_PROFILE_HEADER <= HB_PROFILE_BANK,
               "Profile snapshot exceeds atomic bank");
_Static_assert(sizeof(hb_id_database) % 16 == 0, "Encrypted writes need 16 byte alignment");
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
bool hb_profile_database_valid(const hb_id_database *db)
{
    if (!db || memcmp(db->model, HB_ID_MODEL, sizeof(db->model)) || !db->next_id)
        return false;
    for (unsigned i = 0; i < HB_ID_PEOPLE; i++)
    {
        const hb_id_person *p = &db->people[i];
        if (!p->id)
        {
            if (p->count)
                return false;
            continue;
        }
        unsigned char enabled;
        memcpy(&enabled, &p->enabled, sizeof(enabled));
        if (enabled > 1)
            return false;
        if (p->id >= db->next_id || !p->name[0] || !memchr(p->name, 0, HB_ID_NAME) ||
            p->count > HB_ID_TEMPLATES)
            return false;
        for (unsigned j = 0; j < i; j++)
            if (db->people[j].id == p->id)
                return false;
        for (unsigned j = 0; j < p->count; j++)
            if (!hb_id_embedding_valid(&p->templates[j]))
                return false;
    }
    return true;
}
static hb_store_result bank_read(const hb_profile_io *io, int bank, hb_id_database *db,
                                 uint64_t *generation)
{
    uint8_t h[HB_PROFILE_HEADER], digest[32], header_digest[32];
    size_t offset = (size_t)bank * HB_PROFILE_BANK;
    if (!io->read(io->context, offset, h, sizeof(h)))
        return HB_STORE_IO;
    bool blank = true;
    for (size_t i = 0; i < sizeof(h); i++)
        blank &= h[i] == 0xff;
    if (blank)
        return HB_STORE_EMPTY; // Interrupted write before commit marker.
    if (memcmp(h, "HBP1", 4) || u32(h + 4) != 1 || u32(h + 8) != sizeof(*db) ||
        u32(h + 12) != HB_ID_DIM || memcmp(h + 56, HB_ID_MODEL, sizeof(HB_ID_MODEL)))
        return HB_STORE_INCOMPATIBLE;
    // Protect generation and metadata as well as payload against accidental corruption.
    if (!io->sha256(h, 96, header_digest) || memcmp(h + 96, header_digest, 32))
        return HB_STORE_CORRUPT;
    if (!io->read(io->context, offset + sizeof(h), db, sizeof(*db)))
        return HB_STORE_IO;
    if (!io->sha256(db, sizeof(*db), digest) || memcmp(h + 24, digest, 32) ||
        !hb_profile_database_valid(db))
        return HB_STORE_CORRUPT;
    *generation = u64(h + 16);
    return *generation ? HB_STORE_OK : HB_STORE_CORRUPT;
}
hb_store_result hb_profile_restore(const hb_profile_io *io, hb_id_database *db,
                                   hb_id_database *scratch, hb_profile_revision *revision)
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
hb_store_result hb_profile_commit(const hb_profile_io *io, const hb_id_database *db,
                                  hb_id_database *scratch, hb_profile_revision *revision)
{
    hb_store_result r = admission(io);
    if (r != HB_STORE_OK)
        return r;
    if (!revision || revision->generation == UINT64_MAX || !scratch || db == scratch ||
        !hb_profile_database_valid(db))
        return HB_STORE_INCOMPATIBLE;
    memcpy(scratch, db, sizeof(*db));
    for (unsigned i = 0; i < HB_ID_PEOPLE; i++)
    {
        hb_id_person *p = &scratch->people[i];
        p->last_greeting_ms = 0;
        p->last_sample_ms = 0;
        p->last_sample_sequence = 0;
    }
    uint8_t h[HB_PROFILE_HEADER] = {0};
    memcpy(h, "HBP1", 4);
    put32(h + 4, 1);
    put32(h + 8, sizeof(*db));
    put32(h + 12, HB_ID_DIM);
    put64(h + 16, revision->generation + 1);
    memcpy(h + 56, HB_ID_MODEL, sizeof(HB_ID_MODEL));
    int target = revision->bank == 0 ? 1 : 0;
    size_t offset = (size_t)target * HB_PROFILE_BANK;
    bool ok = io->sha256(scratch, sizeof(*scratch), h + 24) && io->sha256(h, 96, h + 96) &&
              io->erase(io->context, offset, HB_PROFILE_BANK) &&
              io->write(io->context, offset + sizeof(h), scratch, sizeof(*scratch));
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
hb_store_result hb_profile_scrub(const hb_profile_io *io, const hb_profile_revision *revision)
{
    hb_store_result r = admission(io);
    if (r != HB_STORE_OK)
        return r;
    if (!revision || revision->bank < 0 || revision->bank > 1)
        return HB_STORE_IO;
    return io->erase(io->context, (size_t)(1 - revision->bank) * HB_PROFILE_BANK, HB_PROFILE_BANK)
               ? HB_STORE_OK
               : HB_STORE_IO;
}
