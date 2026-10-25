// SPDX-License-Identifier: Apache-2.0
#include "hb_household_voice.h"
#include <math.h>
#include <string.h>
bool hb_voice_accepted(const hb_voice_acceptance *a, const hb_voice_policy *p)
{
    return a && a->microphone && a->numerical && a->model && a->resources && a->speech_selection &&
           a->consent && hb_voice_policy_valid(p);
}
void hb_household_voice_cancel(hb_household_voice_enrollment *en)
{
    if (en)
    {
        memset(en, 0, sizeof(*en));
        en->state = HB_VEN_CANCELLED;
    }
}
static hb_voice_enrollment_result session(hb_household_voice_enrollment *en, hb_id_database *db,
                                          uint32_t epoch, uint64_t now)
{
    if (!en || !epoch || en->epoch != epoch)
        return HB_VEN_SESSION;
    hb_id_person *p = hb_id_find(db, en->staging.id);
    if (!p || !p->enabled)
        return HB_VEN_PROFILE;
    if (!now || now > en->deadline_ms || now < en->deadline_ms - 600000)
        return HB_VEN_TIMEOUT;
    return HB_VEN_OK;
}
hb_voice_enrollment_result hb_household_voice_begin(hb_household_voice_enrollment *en,
                                                    hb_id_database *db, uint64_t id, uint32_t epoch,
                                                    uint64_t now, const hb_voice_acceptance *a,
                                                    const hb_voice_policy *p)
{
    if (!en)
        return HB_VEN_SESSION;
    hb_household_voice_cancel(en);
    if (!hb_voice_accepted(a, p))
        return HB_VEN_GATED;
    hb_id_person *person = hb_id_find(db, id);
    if (!person || !person->enabled)
        return HB_VEN_PROFILE;
    if (!epoch || !now || now > UINT64_MAX - 600000)
        return HB_VEN_SESSION;
    en->staging.id = id;
    en->epoch = epoch;
    en->deadline_ms = now + 600000;
    en->policy = *p;
    en->state = HB_VEN_COLLECT;
    return HB_VEN_OK;
}
hb_voice_enrollment_result hb_household_voice_sample(hb_household_voice_enrollment *en,
                                                     hb_id_database *db,
                                                     const hb_voice_database *voice,
                                                     const hb_voice_embedding *e, uint32_t epoch,
                                                     uint64_t end, uint64_t now, bool verify)
{
    hb_voice_enrollment_result r = session(en, db, epoch, now);
    if (r != HB_VEN_OK)
    {
        hb_household_voice_cancel(en);
        return r;
    }
    if (!hb_voice_database_valid(voice) || !e || !end || end > now || now - end > 5000 ||
        end < en->deadline_ms - 600000 + 3015)
        return HB_VEN_SAMPLE;
    if (verify)
    {
        if (en->state != HB_VEN_VERIFY || end <= en->staging.last_end ||
            end - en->staging.last_end < 3015)
            return HB_VEN_SESSION;
        // Check against every staged template and all other household profiles.
        double norm = 0;
        for (unsigned d = 0; d < HB_VOICE_DIM; ++d)
            norm += (double)e->v[d] * e->v[d];
        if (!isfinite(norm) || norm < .999 || norm > 1.001)
            return HB_VEN_SAMPLE;
        float own_min = 1;
        for (unsigned j = 0; j < en->staging.count; ++j)
        {
            float dot = hb_voice_similarity(e, &en->staging.samples[j]);
            if (!isfinite(dot) || dot < en->policy.threshold || dot >= en->policy.duplicate)
                return HB_VEN_MISMATCH;
            if (dot < own_min)
                own_min = dot;
        }
        for (unsigned i = 0; i < HB_VOICE_PEOPLE; ++i)
        {
            const hb_voice_person *other = &voice->people[i];
            if (!other->id || other->id == en->staging.id)
                continue;
            for (unsigned j = 0; j < other->count; ++j)
                if (own_min - hb_voice_similarity(e, &other->samples[j]) < en->policy.margin)
                    return HB_VEN_MISMATCH;
        }
        en->verified_end_ms = end;
        en->state = HB_VEN_READY;
        return HB_VEN_OK;
    }
    if (en->state != HB_VEN_COLLECT)
        return HB_VEN_SESSION;
    if (!hb_voice_person_add(&en->staging, en->staging.id, e, end, &en->policy))
        return HB_VEN_DUPLICATE_OR_INCONSISTENT;
    if (en->staging.count == HB_VOICE_TEMPLATES)
        en->state = HB_VEN_VERIFY;
    return HB_VEN_OK;
}
hb_voice_enrollment_result hb_household_voice_commit(hb_household_voice_enrollment *en,
                                                     hb_id_database *db, hb_voice_database *voice,
                                                     uint32_t epoch, uint64_t now)
{
    hb_voice_enrollment_result r = session(en, db, epoch, now);
    if (r != HB_VEN_OK)
    {
        hb_household_voice_cancel(en);
        return r;
    }
    if (en->state != HB_VEN_READY || !en->verified_end_ms || !hb_voice_database_valid(voice))
        return HB_VEN_SESSION;
    hb_voice_person *slot = NULL;
    for (unsigned i = 0; i < HB_VOICE_PEOPLE; ++i)
        if (voice->people[i].id == en->staging.id)
        {
            slot = &voice->people[i];
            break;
        }
    if (!slot)
        for (unsigned i = 0; i < HB_VOICE_PEOPLE; ++i)
            if (!voice->people[i].id)
            {
                slot = &voice->people[i];
                break;
            }
    if (!slot)
        return HB_VEN_PROFILE;
    *slot = en->staging;
    hb_household_voice_cancel(en);
    en->state = HB_VEN_SAVED;
    return HB_VEN_OK;
}
void hb_household_voice_reconcile(hb_id_database *db, hb_voice_database *voice)
{
    if (!voice)
        return;
    for (unsigned i = 0; i < HB_VOICE_PEOPLE; ++i)
    {
        hb_id_person *p = hb_id_find(db, voice->people[i].id);
        if (!p || !p->enabled)
            memset(&voice->people[i], 0, sizeof(voice->people[i]));
    }
}
unsigned hb_household_voice_count(const hb_voice_database *db, uint64_t id)
{
    if (hb_voice_database_valid(db) && id)
        for (unsigned i = 0; i < HB_VOICE_PEOPLE; ++i)
            if (db->people[i].id == id)
                return db->people[i].count;
    return 0;
}
const char *hb_household_voice_error(hb_voice_enrollment_result r)
{
    switch (r)
    {
    case HB_VEN_OK:
        return "Próbka przyjęta tylko w RAM";
    case HB_VEN_GATED:
        return "Głos niedostępny: oczekuje na test mikrofonu, modelu i kalibrację";
    case HB_VEN_PROFILE:
        return "Brak aktywnego domownika";
    case HB_VEN_SESSION:
        return "Sesja nieaktualna; rozpocznij ponownie";
    case HB_VEN_SAMPLE:
        return "Nieprawidłowa lub nieaktualna próbka";
    case HB_VEN_DUPLICATE_OR_INCONSISTENT:
        return "Próbka zbyt podobna lub niespójna; powiedz inne zdanie";
    case HB_VEN_MISMATCH:
        return "Weryfikacja nieudana; użyj nowej wypowiedzi jednej osoby";
    case HB_VEN_TIMEOUT:
        return "Upłynął czas rejestracji";
    }
    return "Błąd rejestracji głosu";
}
hb_voice_match hb_household_voice_identify(hb_id_database *db, const hb_voice_database *voice,
                                           const hb_voice_embedding *embedding,
                                           const hb_voice_acceptance *accepted,
                                           const hb_voice_policy *policy, hb_voice_tracker *tracker,
                                           uint32_t epoch, uint64_t end, uint64_t now,
                                           bool *confirmed)
{
    hb_voice_match match = {HB_VOICE_UNAVAILABLE, 0, -1, -1};
    if (confirmed)
        *confirmed = false;
    if (!confirmed || !tracker || !hb_voice_accepted(accepted, policy) ||
        !hb_voice_database_valid(voice) || !epoch || !end || end > now || now - end > 5000)
        goto unavailable;
    for (unsigned i = 0; i < HB_VOICE_PEOPLE; ++i)
    {
        if (!voice->people[i].id)
            continue;
        hb_id_person *person = hb_id_find(db, voice->people[i].id);
        if (!person || !person->enabled)
            goto unavailable;
    }
    match = hb_voice_identify(voice, embedding, policy);
    *confirmed = hb_voice_confirm(tracker, match, epoch, end, now, policy);
    return match;
unavailable:
    if (tracker)
        memset(tracker, 0, sizeof(*tracker));
    return match;
}
