// SPDX-License-Identifier: Apache-2.0
#include "hb_identity_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static hb_id_policy policy = {0.70f, 0.12f, 0.60f, 3};
static hb_id_embedding vector(unsigned axis, float primary)
{
    hb_id_embedding e = {{0}};
    e.v[axis] = primary;
    e.v[axis + 1] = sqrtf(1 - primary * primary);
    return e;
}
static void fill(hb_id_database *db, uint64_t id, hb_id_embedding e, unsigned count)
{
    hb_id_person *p = hb_id_find(db, id);
    assert(p);
    p->count = count;
    for (unsigned i = 0; i < count; ++i)
        p->templates[i] = e;
}
static void geometry(void)
{
    hb_id_landmarks l = {{{38.2946f, 51.6963f},
                          {41.5493f, 92.3655f},
                          {56.0252f, 71.7366f},
                          {73.5318f, 51.5014f},
                          {70.7299f, 92.2041f}}};
    assert(hb_id_landmarks_valid(&l, 112, 112));
    for (unsigned i = 0; i < 5; ++i)
    {
        l.p[i].x = l.p[i].x * 2 + 15;
        l.p[i].y = l.p[i].y * 2 + 20;
    }
    assert(hb_id_landmarks_valid(&l, 300, 300));
    hb_id_landmarks bad = l;
    bad.p[0] = l.p[3];
    bad.p[3] = l.p[0];
    assert(!hb_id_landmarks_valid(&bad, 300, 300));
    bad = l;
    bad.p[2].x = NAN;
    assert(!hb_id_landmarks_valid(&bad, 300, 300));
    bad = l;
    bad.p[2].y = 299;
    assert(!hb_id_landmarks_valid(&bad, 300, 300));
    memset(&bad, 0, sizeof(bad));
    assert(!hb_id_alignment_valid(&bad));
    bad = l;
    bad.p[2].y = bad.p[0].y;
    assert(!hb_id_landmarks_valid(&bad, 300, 300));
    unsigned sx, sy;
    assert(hb_id_to_sensor(0, 0, 1288, 728, &sx, &sy) && sx == 1287 && sy == 0);
    assert(hb_id_to_sensor(727, 1287, 1288, 728, &sx, &sy) && sx == 0 && sy == 727);
    assert(!hb_id_to_sensor(728, 0, 1288, 728, &sx, &sy));
    hb_id_roi roi;
    assert(hb_id_roi_map(70, 100, 160, 240, 234, 416, 1288, 728, &roi));
    assert(roi.x >= 0 && roi.y >= 0 && roi.x + roi.w <= 728 && roi.y + roi.h <= 1288);
    assert(roi.w > 90 && roi.h > 150);
    assert(!hb_id_roi_map(-1, 100, 160, 250, 234, 416, 1288, 728, &roi));
    assert(!hb_id_roi_map(100, 100, 100, 250, 234, 416, 1288, 728, &roi));
    assert(!hb_id_roi_map(0, 0, 233, 415, 234, 416, 1288, 728, &roi)); // bounded ROI refusal
    uint8_t raw[32 * 32], rgb[32 * 32 * 3];
    for (unsigned y = 0; y < 32; ++y)
        for (unsigned x = 0; x < 32; ++x)
            raw[y * 32 + x] = (x + y) % 240 + 16;
    roi = (hb_id_roi){0, 0, 32, 32};
    assert(hb_id_raw_roi(raw, sizeof(raw), 32, 32, roi, rgb, sizeof(rgb)));
    assert(!hb_id_raw_roi(raw, sizeof(raw) - 1, 32, 32, roi, rgb, sizeof(rgb)));
    assert(!hb_id_raw_roi(raw, sizeof(raw), 32, 32, roi, rgb, sizeof(rgb) - 1));
    roi.x = 1;
    assert(!hb_id_raw_roi(raw, sizeof(raw), 32, 32, roi, rgb, sizeof(rgb)));
}
static void matching(void)
{
    hb_id_database *db = calloc(1, sizeof(*db));
    assert(db);
    hb_id_database_init(db, 42);
    uint64_t a = hb_id_create(db, "Anna"), b = hb_id_create(db, "Bartek");
    assert(a && b && a != b);
    hb_id_embedding e = vector(0, 1), other = vector(2, 1);
    assert(hb_id_embedding_valid(&e));
    hb_id_embedding invalid = {{0}};
    assert(!hb_id_embedding_valid(&invalid));
    invalid = e;
    invalid.v[1] = INFINITY;
    assert(!hb_id_embedding_valid(&invalid));
    invalid = e;
    invalid.v[0] = 2;
    assert(!hb_id_embedding_valid(&invalid));
    fill(db, a, e, 8);
    fill(db, b, other, 15);
    hb_id_match m = hb_id_match_embedding(db, &e, &policy);
    assert(m.state == HB_ID_KNOWN && m.person_id == a && m.best == 1 && m.second == 0);
    hb_id_embedding stranger = vector(4, 1);
    assert(hb_id_match_embedding(db, &stranger, &policy).state == HB_ID_UNKNOWN);
    fill(db, b, vector(0, 0.96f), 15);
    assert(hb_id_match_embedding(db, &e, &policy).state == HB_ID_AMBIGUOUS);
    fill(db, b, other, 8);
    hb_id_policy boundary = policy;
    boundary.threshold = 1;
    assert(hb_id_match_embedding(db, &e, &boundary).state == HB_ID_KNOWN);
    boundary.threshold = NAN;
    assert(hb_id_match_embedding(db, &e, &boundary).state == HB_ID_UNAVAILABLE);
    assert(hb_id_rename(db, a, "Nowa nazwa") && hb_id_find(db, a)->id == a);
    assert(!hb_id_rename(db, a, "\n"));
    assert(hb_id_delete(db, a));
    uint64_t replacement = hb_id_create(db, "Nowa osoba");
    assert(replacement != a && hb_id_find(db, replacement)->count == 0);
    assert(hb_id_match_embedding(db, &e, &policy).state == HB_ID_UNKNOWN);
    hb_id_find(db, b)->count = 16;
    assert(hb_id_match_embedding(db, &e, &policy).state == HB_ID_UNAVAILABLE);
    hb_id_find(db, b)->count = 8;
    db->model[0] = '!';
    assert(hb_id_match_embedding(db, &e, &policy).state == HB_ID_UNAVAILABLE);
    assert(!hb_id_create(db, "Wrong model"));
    free(db);
}
static void temporal(void)
{
    hb_id_tracker t;
    hb_id_match known = {HB_ID_KNOWN, 1, 0.9f, 0.1f};
    hb_id_tracker_reset(&t, 3);
    assert(!hb_id_confirm(&t, known, 3, 1, 1000, 1100, 1200, &policy));
    assert(!hb_id_confirm(&t, known, 3, 2, 2000, 2100, 2200, &policy));
    assert(hb_id_confirm(&t, known, 3, 3, 3000, 3100, 3200, &policy));
    assert(!hb_id_confirm(&t, known, 3, 3, 3000, 3100, 3200, &policy)); // duplicates reset
    assert(!t.hits);
    assert(!hb_id_confirm(&t, known, 3, 4, 4000, 4100, 4200, &policy));
    known.person_id = 2;
    assert(!hb_id_confirm(&t, known, 3, 5, 5000, 5100, 5200, &policy));
    assert(t.hits == 1 && t.person_id == 2);
    assert(!hb_id_confirm(&t, known, 2, 6, 6000, 6100, 6200, &policy)); // cancelled session
    hb_id_match unknown = {HB_ID_UNKNOWN, 0, 0, 0};
    assert(!hb_id_confirm(&t, unknown, 3, 6, 6000, 6100, 6200, &policy) && !t.hits);
    assert(!hb_id_confirm(&t, known, 3, 7, 7000, 7100, 13000, &policy)); // stale
    assert(!hb_id_confirm(&t, known, 3, 8, 9000, 8000, 10000, &policy)); // invalid timestamps
    assert(!hb_id_confirm(&t, known, 3, 5, 5000, 5100, 5200, &policy));  // out of order
}
static void enrollment(void)
{
    hb_id_database *db = calloc(1, sizeof(*db));
    hb_id_enrollment *en = calloc(1, sizeof(*en));
    assert(db && en);
    hb_id_database_init(db, 7);
    en->target_id = hb_id_create(db, "Test");
    hb_id_embedding e = vector(0, 1);
    assert(!hb_id_enroll_sample(en, &e, 0, true, 1, 1000));
    assert(!hb_id_enroll_sample(en, &e, 2, true, 1, 1000));
    assert(!hb_id_enroll_sample(en, &e, 1, false, 1, 1000));
    assert(hb_id_enroll_sample(en, &e, 1, true, 1, 1000));
    assert(!hb_id_enroll_sample(en, &e, 1, true, 2, 2000)); // redundant
    assert(!hb_id_enroll_commit(db, en));
    for (unsigned i = 1; i < 8; ++i)
    {
        e = vector(0, 0.95f);
        e.v[1] = 0;
        e.v[i + 1] = sqrtf(1 - 0.95f * 0.95f);
        assert(hb_id_enroll_sample(en, &e, 1, true, i + 1, (i + 1) * 1000));
    }
    e = vector(0, 1);
    assert(!hb_id_enroll_verify(en, &e, 8, 8000, &policy)); // enrollment frame
    assert(hb_id_enroll_verify(en, &e, 9, 9000, &policy));
    uint64_t id = en->target_id;
    assert(hb_id_enroll_commit(db, en));
    assert(hb_id_find(db, id)->count == 8 && !en->count);
    assert(hb_id_enroll_begin(db, en, id));
    assert(en->count == 8 && en->last_sequence == 8);
    assert(!hb_id_enroll_sample(en, &e, 1, true, 10, 10000)); // existing template stays redundant
    e = vector(0, 0.95f);
    e.v[1] = 0;
    e.v[9] = sqrtf(1 - 0.95f * 0.95f);
    assert(hb_id_enroll_sample(en, &e, 1, true, 10, 10000));
    assert(en->count == 9 && hb_id_find(db, id)->count == 8);
    memset(en, 0, sizeof(*en)); // cancel append leaves committed templates intact
    assert(hb_id_enroll_begin(db, en, id) && en->count == 8);
    assert(hb_id_enroll_sample(en, &e, 1, true, 11, 11000));
    assert(hb_id_enroll_verify(en, &e, 12, 12000, &policy));
    assert(hb_id_enroll_commit(db, en) && hb_id_find(db, id)->count == 9);
    assert(hb_id_delete(db, id));
    assert(!hb_id_enroll_commit(db, en));
    free(en);
    free(db);
}
int main(void)
{
    hb_id_evaluation stats = {0};
    hb_id_match trial = {HB_ID_KNOWN, 1, 0.9f, 0};
    assert(hb_id_evaluate(&stats, 1, trial, true, 1, 1000));
    assert(!hb_id_evaluate(&stats, 1, trial, true, 1, 1000));
    assert(hb_id_evaluate(&stats, 2, trial, true, 2, 2000));
    assert(hb_id_evaluate(&stats, 0, trial, true, 3, 3000));
    trial.state = HB_ID_AMBIGUOUS;
    assert(hb_id_evaluate(&stats, 0, trial, false, 4, 4000));
    assert(hb_id_evaluate(&stats, 1, trial, false, 5, 5000));
    assert(stats.true_accept == 1 && stats.confusion == 1 && stats.unknown_accept == 1);
    assert(stats.unknown_reject == 1 && stats.false_reject == 1 && stats.ambiguous == 2);
    geometry();
    matching();
    temporal();
    enrollment();
    puts("Identity: geometry, native ROI, embeddings, profiles, matching, temporal and enrollment "
         "passed");
    return 0;
}
