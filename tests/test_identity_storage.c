// SPDX-License-Identifier: Apache-2.0
#include "hb_identity_events.h"
#include "hb_identity_model.h"
#include "hb_profile_store.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct
{
    uint8_t flash[HB_PROFILE_PARTITION];
    int operation, fail_at;
    bool torn;
} device;
static bool fault(device *d)
{
    return ++d->operation == d->fail_at;
}
static bool rd(void *c, size_t o, void *b, size_t n)
{
    device *d = c;
    if (o + n > sizeof(d->flash) || fault(d))
        return false;
    memcpy(b, d->flash + o, n);
    return true;
}
static bool wr(void *c, size_t o, const void *b, size_t n)
{
    device *d = c;
    if (o + n > sizeof(d->flash))
        return false;
    bool fail = fault(d);
    if (fail && !d->torn)
        return false;
    for (size_t i = 0; i < (fail ? n / 2 : n); i++)
        d->flash[o + i] &= ((const uint8_t *)b)[i];
    return !fail;
}
static bool erase(void *c, size_t o, size_t n)
{
    device *d = c;
    if (o + n > sizeof(d->flash) || o % 4096 || n % 4096)
        return false;
    bool fail = fault(d);
    if (fail && !d->torn)
        return false;
    memset(d->flash + o, 255, fail ? n / 2 : n);
    return !fail;
}
// Deterministic injected checksum tests the transaction protocol, not cryptography.
// Device adapter uses mbedTLS SHA256; model tool tests check actual SHA256 bytes.
static bool checksum(const void *b, size_t n, uint8_t out[32])
{
    uint32_t h = 2166136261U;
    const uint8_t *p = b;
    for (size_t i = 0; i < n; i++)
        h = (h ^ p[i]) * 16777619U;
    for (unsigned i = 0; i < 32; i++)
        out[i] = (uint8_t)(h >> ((i % 4) * 8));
    return true;
}
static void profiles(void)
{
    device *d = calloc(1, sizeof(*d));
    memset(d->flash, 255, sizeof(d->flash));
    hb_id_database *db = calloc(1, sizeof(*db)), *scratch = calloc(1, sizeof(*db)),
                   *loaded = calloc(1, sizeof(*db));
    hb_profile_io io = {d,     sizeof(d->flash), HB_PROFILE_STORAGE_HARDWARE_PROTECTED, rd, wr,
                        erase, checksum};
    hb_profile_revision rev = {0, -1};
    hb_id_database_init(db, 123);
    uint64_t id = hb_id_create(db, "Maciej");
    hb_id_person *p = hb_id_find(db, id);
    p->count = 1;
    p->templates[0].v[0] = 1;
    assert(hb_profile_restore(&io, loaded, scratch, &rev) == HB_STORE_EMPTY);
    assert(hb_profile_commit(&io, db, scratch, &rev) == HB_STORE_OK && rev.generation == 1);
    assert(hb_profile_restore(&io, loaded, scratch, &rev) == HB_STORE_OK && hb_id_find(loaded, id));
    device *original = malloc(sizeof(*d));
    *original = *d;
    assert(hb_id_rename(db, id, "Nowa nazwa"));
    // Every read/erase/write boundary, including partially written commit headers.
    for (int torn = 0; torn < 2; torn++)
        for (int failure = 1; failure <= 8; failure++)
        {
            *d = *original;
            d->operation = 0;
            d->fail_at = failure;
            d->torn = torn != 0;
            rev = (hb_profile_revision){1, 0};
            hb_store_result result = hb_profile_commit(&io, db, scratch, &rev);
            d->fail_at = 0;
            d->operation = 0;
            assert(hb_profile_restore(&io, loaded, scratch, &rev) == HB_STORE_OK);
            const char *name = hb_id_find(loaded, id)->name;
            assert(!strcmp(name, "Maciej") || !strcmp(name, "Nowa nazwa"));
            if (result == HB_STORE_OK)
                assert(!strcmp(name, "Nowa nazwa"));
        }
    *d = *original;
    rev = (hb_profile_revision){1, 0};
    io.storage_mode = HB_PROFILE_STORAGE_RAM_ONLY;
    assert(hb_profile_commit(&io, db, scratch, &rev) == HB_STORE_LOCKED);
    io.storage_mode = HB_PROFILE_STORAGE_HARDWARE_PROTECTED;
    io.size--;
    assert(hb_profile_commit(&io, db, scratch, &rev) == HB_STORE_CAPACITY);
    io.size++;
    db->model[0] = 'x';
    assert(hb_profile_commit(&io, db, scratch, &rev) == HB_STORE_INCOMPATIBLE);
    db->model[0] = HB_ID_MODEL[0];
    assert(hb_profile_commit(&io, db, scratch, &rev) == HB_STORE_OK);
    d->flash[(size_t)rev.bank * HB_PROFILE_BANK + HB_PROFILE_HEADER + 10] ^= 1;
    assert(hb_profile_restore(&io, loaded, scratch, &rev) == HB_STORE_OK);
    assert(!strcmp(hb_id_find(loaded, id)->name, "Maciej")); // previous valid generation
    assert(hb_id_delete(loaded, id));
    assert(hb_profile_commit(&io, loaded, scratch, &rev) == HB_STORE_OK);
    assert(hb_profile_scrub(&io, &rev) == HB_STORE_OK);
    assert(hb_profile_restore(&io, loaded, scratch, &rev) == HB_STORE_OK &&
           !hb_id_find(loaded, id));
    assert(loaded->next_id == id + 1);
    for (unsigned i = 0; i < HB_ID_PEOPLE; i++)
        assert(hb_id_create(loaded, "Person"));
    assert(!hb_id_create(loaded, "Overflow"));
    for (unsigned i = 0; i < HB_ID_PEOPLE; i++)
    {
        loaded->people[i].count = HB_ID_TEMPLATES;
        for (unsigned j = 0; j < HB_ID_TEMPLATES; j++)
            loaded->people[i].templates[j].v[0] = 1;
    }
    assert(hb_profile_commit(&io, loaded, scratch, &rev) == HB_STORE_OK);
    assert(hb_profile_restore(&io, loaded, scratch, &rev) == HB_STORE_OK);
    assert(sizeof(*db) + HB_PROFILE_HEADER <= HB_PROFILE_BANK);
    printf("Profile snapshot: %zu bytes, bank: %u bytes\n", sizeof(*db), HB_PROFILE_BANK);
    free(original);
    free(d);
    free(db);
    free(scratch);
    free(loaded);
}
static void events(void)
{
    hb_event_bus bus;
    hb_event_bus_init(&bus);
    hb_id_event e = {.frame_ms = 100,
                     .timestamp_ms = 200,
                     .session = 1,
                     .sequence = 1,
                     .state = HB_ID_KNOWN,
                     .confirmed = true,
                     .person_id = 42,
                     .kind = HB_ID_EVENT_RECOGNIZED};
    assert(hb_event_push(&bus, e));
    assert(!hb_event_push(&bus, e));
    e.kind = (hb_id_event_kind)-1;
    assert(!hb_event_push(&bus, e));
    e.kind = HB_ID_EVENT_RECOGNIZED;
    hb_id_event out;
    assert(hb_event_pop(&bus, HB_EVENTS_GREETING, 1, 300, &out) && out.person_id == 42);
    assert(hb_event_pop(&bus, HB_EVENTS_HA, 1, 300, &out) && out.event_id == 1);
    assert(!hb_event_pop(&bus, HB_EVENTS_HA, 1, 300, &out));
    e.person_id = 43;
    assert(hb_event_push(&bus, e));
    e.person_id = 42;
    assert(!hb_event_push(&bus, e)); // same encounter, even after another confirmed person
    assert(hb_event_pop(&bus, HB_EVENTS_HA, 1, 300, &out) && out.person_id == 43);
    assert(!hb_event_pop(&bus, HB_EVENTS_LEGACY, 2, 300, &out)); // cancellation
    for (unsigned i = 2; i < 40; i++)
    {
        e.session = i;
        assert(hb_event_push(&bus, e));
    }
    assert(bus.overflow[HB_EVENTS_HA] > 0);
    assert(hb_event_pop(&bus, HB_EVENTS_HA, 39, 300, &out) && out.session == 39);
    e.session = 40;
    assert(hb_event_push(&bus, e));
    assert(!hb_event_pop(&bus, HB_EVENTS_HA, 40, 5200, &out));
    e.session = 41;
    e.confirmed = false;
    assert(!hb_event_push(&bus, e));
    e.kind = HB_ID_EVENT_UNKNOWN;
    e.state = HB_ID_UNKNOWN;
    e.person_id = 0;
    assert(hb_event_push(&bus, e));
    assert(!hb_event_push(&bus, e));
    e.source = HB_ID_SOURCE_VOICE;
    e.session = 42;
    assert(!hb_event_push(&bus, e));
    e.source = HB_ID_SOURCE_FACE;
    e.timestamp_ms = 99;
    assert(!hb_event_push(&bus, e));
}
static void model(void)
{
    uint8_t h[HB_MBF_OFFSET] = {0};
    memcpy(h, "PDL3v1.0", 8);
    uint32_t size = HB_MBF_PACKAGE_BYTES;
    for (unsigned i = 0; i < 4; i++)
        h[20 + i] = (uint8_t)(size >> (8 * i));
    h[56] = 1;
    h[60] = 112;
    h[64] = 72;
    h[68] = 31;
    memcpy(h + 72, "human_face_feat_mbf_s8_v1.espdl", 31);
    assert(hb_identity_model_header_valid(h, sizeof(h), 0x400000));
    assert(!hb_identity_model_header_valid(h, sizeof(h), 1966080));
    assert(!hb_identity_model_header_valid(h, 100, 0x400000));
    unsigned offsets[] = {0, 4, 19, 20, 56, 60, 64, 68, 72, 111};
    for (unsigned i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++)
    {
        h[offsets[i]] ^= 1;
        assert(!hb_identity_model_header_valid(h, sizeof(h), 0x400000));
        h[offsets[i]] ^= 1;
    }
}
int main(void)
{
    profiles();
    events();
    model();
    puts("Flash transactions, corruption, model format and bounded events passed");
    return 0;
}
