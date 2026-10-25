// SPDX-License-Identifier: Apache-2.0
#include "hb_household_voice.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static hb_id_database face;
static hb_voice_database voice, prior;
static hb_household_voice_enrollment en;
static hb_voice_policy policy = {true, .85f, .1f, .7f, .9999f, 2};
static hb_voice_acceptance accepted = {true, true, true, true, true, true};
static hb_voice_embedding sample(float angle)
{
    hb_voice_embedding e = {{0}};
    e.v[0] = cosf(angle);
    e.v[1] = sinf(angle);
    return e;
}
static uint64_t begin(void)
{
    hb_id_database_init(&face, 1);
    hb_voice_database_init(&voice);
    uint64_t id = hb_id_create(&face, "Synthetic");
    assert(id);
    assert(hb_household_voice_begin(&en, &face, id, 7, 1, &accepted, &policy) == HB_VEN_OK);
    return id;
}
static void collect(void)
{
    float angles[] = {0, .1f, -.1f, .2f};
    for (unsigned i = 0; i < 4; ++i)
    {
        hb_voice_embedding e = sample(angles[i]);
        assert(hb_household_voice_sample(&en, &face, &voice, &e, 7, 4000 + i * 4000,
                                         4001 + i * 4000, false) == HB_VEN_OK);
    }
    assert(en.state == HB_VEN_VERIFY);
}
int main(void)
{
    uint64_t id = begin();
    hb_voice_acceptance no = accepted;
    no.consent = false;
    assert(hb_household_voice_begin(&en, &face, id, 7, 1, &no, &policy) == HB_VEN_GATED);
    no = accepted;
    no.speech_selection = false;
    assert(hb_household_voice_begin(&en, &face, id, 7, 1, &no, &policy) == HB_VEN_GATED);
    policy.calibrated = false;
    assert(!hb_voice_accepted(&accepted, &policy));
    policy.calibrated = true;
    id = begin();
    collect();
    hb_voice_embedding e = sample(.05f);
    assert(hb_household_voice_commit(&en, &face, &voice, 7, 17000) == HB_VEN_SESSION);
    assert(hb_household_voice_sample(&en, &face, &voice, &e, 7, 16001, 16002, true) ==
           HB_VEN_SESSION);
    assert(hb_household_voice_sample(&en, &face, &voice, &e, 7, 20000, 20001, true) == HB_VEN_OK);
    assert(hb_household_voice_commit(&en, &face, &voice, 7, 20002) == HB_VEN_OK);
    assert(hb_household_voice_count(&voice, id) == 4);
    hb_voice_tracker tracker = {0};
    bool confirmed = false;
    assert(hb_household_voice_identify(&face, &voice, &e, &accepted, &policy, &tracker, 7, 24000,
                                       24001, &confirmed)
               .person_id == id);
    assert(!confirmed);
    assert(hb_household_voice_identify(&face, &voice, &e, &accepted, &policy, &tracker, 7, 28000,
                                       28001, &confirmed)
               .person_id == id);
    assert(confirmed);
    assert(hb_household_voice_identify(&face, &voice, &e, &accepted, &policy, &tracker, 7, 28000,
                                       34001, &confirmed)
               .state == HB_VOICE_UNAVAILABLE);
    assert(!confirmed && !tracker.hits);
    face.people[0].enabled = false;
    assert(hb_household_voice_identify(&face, &voice, &e, &accepted, &policy, &tracker, 7, 36000,
                                       36001, &confirmed)
               .state == HB_VOICE_UNAVAILABLE);
    face.people[0].enabled = true;
    assert(hb_id_rename(&face, id, "Renamed"));
    assert(hb_household_voice_count(&voice, id) == 4);
    prior = voice;
    assert(hb_household_voice_begin(&en, &face, id, 8, 21000, &accepted, &policy) == HB_VEN_OK);
    hb_household_voice_cancel(&en);
    assert(!memcmp(&voice, &prior, sizeof(voice)));
    // Face re-enrollment does not touch voice templates.
    hb_id_enrollment facial = {0};
    assert(hb_id_enroll_begin(&face, &facial, id));
    assert(!memcmp(&voice, &prior, sizeof(voice)));
    assert(hb_id_delete(&face, id));
    hb_household_voice_reconcile(&face, &voice);
    assert(!hb_household_voice_count(&voice, id));
    id = begin();
    e = sample(0);
    assert(hb_household_voice_sample(&en, &face, &voice, &e, 7, 4000, 4001, false) == HB_VEN_OK);
    assert(hb_household_voice_sample(&en, &face, &voice, &e, 7, 8000, 8001, false) ==
           HB_VEN_DUPLICATE_OR_INCONSISTENT);
    e.v[0] = NAN;
    assert(hb_household_voice_sample(&en, &face, &voice, &e, 7, 8000, 8001, false) != HB_VEN_OK);
    e = sample(.1f);
    assert(hb_household_voice_sample(&en, &face, &voice, &e, 8, 8000, 8001, false) ==
           HB_VEN_SESSION);
    assert(en.state == HB_VEN_CANCELLED && en.staging.count == 0);
    begin();
    collect();
    e = sample(1.5f);
    assert(hb_household_voice_sample(&en, &face, &voice, &e, 7, 20000, 20001, true) ==
           HB_VEN_MISMATCH);
    e = sample(.05f);
    uint64_t other = hb_id_create(&face, "Other");
    voice.people[0].id = other;
    voice.people[0].count = 3;
    for (unsigned i = 0; i < 3; ++i)
        voice.people[0].samples[i] = e;
    assert(hb_household_voice_sample(&en, &face, &voice, &e, 7, 20000, 20001, true) ==
           HB_VEN_MISMATCH);
    assert(hb_household_voice_sample(&en, &face, &voice, &e, 7, 20000, 600002, true) ==
           HB_VEN_TIMEOUT);
    puts("Household voice: gates, four staged samples, fresh verification, conflict, rename, "
         "independent re-enrollment, delete, cancellation, timeout passed");
}
