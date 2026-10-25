// SPDX-License-Identifier: Apache-2.0
#include "hb_voice_core.h"
#include <math.h>
#include <string.h>
bool hb_voice_pcm_frame(const int16_t *pcm, size_t n, unsigned frame, float out[400])
{
    if (!pcm || !n || n > HB_VOICE_SAMPLES * 2 || frame >= HB_VOICE_FRAMES || !out)
        return false;
    size_t crop = n > HB_VOICE_SAMPLES ? (n - HB_VOICE_SAMPLES) / 2 : 0;
    for (unsigned j = 0; j < 400; ++j)
    {
        size_t i = crop + frame * 160 + j;
        out[j] = i < n ? pcm[i] / 32768.0f : 0;
    }
    return true;
}
static bool valid(const hb_voice_embedding *e)
{
    if (!e)
        return false;
    double norm = 0;
    for (unsigned i = 0; i < HB_VOICE_DIM; ++i)
    {
        if (!isfinite(e->v[i]))
            return false;
        norm += (double)e->v[i] * e->v[i];
    }
    return norm > 0.999 && norm < 1.001;
}
bool hb_voice_normalize(hb_voice_embedding *e)
{
    if (!e)
        return false;
    double norm = 0;
    for (unsigned i = 0; i < HB_VOICE_DIM; ++i)
    {
        if (!isfinite(e->v[i]))
            return false;
        norm += (double)e->v[i] * e->v[i];
    }
    if (!(norm > 1e-20) || !isfinite(norm))
        return false;
    norm = sqrt(norm);
    for (unsigned i = 0; i < HB_VOICE_DIM; ++i)
        e->v[i] = (float)(e->v[i] / norm);
    return valid(e);
}
bool hb_voice_dequantize(const int8_t *input, size_t size, int exponent, hb_voice_embedding *out)
{
    if (!input || !out || size != HB_VOICE_DIM || exponent < -30 || exponent > 16)
        return false;
    for (unsigned i = 0; i < HB_VOICE_DIM; ++i)
        out->v[i] = ldexpf(input[i], exponent);
    if (hb_voice_normalize(out))
        return true;
    memset(out, 0, sizeof(*out));
    return false;
}
bool hb_voice_quantize(float *features, size_t count, int exponent, int16_t *out)
{
    if (!features || !out || count != HB_VOICE_FRAMES * HB_VOICE_BINS || exponent < -30 ||
        exponent > 16)
        return false;
    for (size_t i = 0; i < count; ++i)
        if (!isfinite(features[i]))
            return false;
    for (unsigned d = 0; d < HB_VOICE_BINS; ++d)
    {
        float mean = 0;
        for (unsigned t = 0; t < HB_VOICE_FRAMES; ++t)
            mean += features[t * HB_VOICE_BINS + d];
        mean /= HB_VOICE_FRAMES;
        if (!isfinite(mean))
            return false;
        for (unsigned t = 0; t < HB_VOICE_FRAMES; ++t)
        {
            size_t i = t * HB_VOICE_BINS + d;
            // P4 ESP-DL uses round-half-even, independent of host floating rounding mode.
            float scaled = ldexpf(features[i] - mean, -exponent);
            float q = fmaxf(-32768, fminf(32767, scaled));
            if (q > -32768 && q < 32767)
            {
                float rounded = q < 0 ? q - 0.5f : q + 0.5f;
                int value = (int)rounded;
                if (fabsf(rounded - value) < 1e-6f && (value & 1))
                    value += q < 0 ? 1 : -1;
                q = (float)value;
            }
            out[i] = (int16_t)fmaxf(-32768, fminf(32767, q));
        }
    }
    return true;
}
void hb_voice_gate_reset(hb_voice_gate *g)
{
    if (g)
        memset(g, 0, sizeof(*g));
}
hb_voice_gate_state hb_voice_gate_feed(hb_voice_gate *g, const int16_t *pcm, size_t n,
                                       uint32_t rate, uint64_t end)
{
    if (!g)
        return HB_VAD_REJECTED;
    if (g->state == HB_VAD_READY || g->state == HB_VAD_REJECTED)
        return g->state;
    if (!pcm || !n || n > 320 || rate != 16000 || !end ||
        (g->end_ms && (end <= g->end_ms || end - g->end_ms > 40)) ||
        g->samples + n > HB_VOICE_SAMPLES)
    {
        g->state = HB_VAD_REJECTED;
        return g->state;
    }
    int64_t sum = 0, squares = 0;
    unsigned clip = 0, changes = 0;
    for (size_t i = 0; i < n; ++i)
    {
        int32_t v = pcm[i];
        sum += v;
        squares += (int64_t)v * v;
        clip += v >= 32760 || v <= -32760;
        if (i)
            changes += pcm[i] != pcm[i - 1];
    }
    double variance = (double)(squares * (int64_t)n - sum * sum) / (double)(n * n);
    if (clip > n / 20 || sum / (int64_t)n > 8000 || sum / (int64_t)n < -8000)
    {
        g->state = HB_VAD_REJECTED;
        return g->state;
    }
    bool repeats = false;
    for (size_t period = 1; period <= 8 && n >= period * 4; ++period)
    {
        bool same = true;
        for (size_t i = period; i < n; ++i)
            same &= pcm[i] == pcm[i % period];
        repeats |= same;
    }
    bool activity = !repeats && variance >= 200.0 * 200.0 && changes > n / 4;
    g->samples += (uint32_t)n;
    g->end_ms = end;
    if (activity)
    {
        g->active_samples += (uint32_t)n;
        g->pause_samples = 0;
    }
    else
        g->pause_samples += (uint32_t)n;
    if (g->active_samples >= 4800)
        g->state = HB_VAD_ACTIVITY;
    if (g->samples == HB_VOICE_SAMPLES || g->pause_samples >= 8000)
        g->state = g->samples == HB_VOICE_SAMPLES && g->active_samples >= 32000 ? HB_VAD_READY
                                                                                : HB_VAD_REJECTED;
    return g->state;
}
float hb_voice_similarity(const hb_voice_embedding *a, const hb_voice_embedding *b)
{
    float dot = 0;
    for (unsigned i = 0; i < HB_VOICE_DIM; ++i)
        dot += a->v[i] * b->v[i];
    return fmaxf(-1, fminf(1, dot));
}
bool hb_voice_policy_valid(const hb_voice_policy *p)
{
    return p && p->calibrated && isfinite(p->threshold) && p->threshold > 0 && p->threshold <= 1 &&
           isfinite(p->margin) && p->margin > 0 && p->margin <= 1 && isfinite(p->agreement) &&
           p->agreement > 0 && p->agreement <= 1 && isfinite(p->duplicate) &&
           p->duplicate > p->agreement && p->duplicate <= 1 && p->confirmations >= 2 &&
           p->confirmations <= 10;
}
void hb_voice_database_init(hb_voice_database *db)
{
    if (!db)
        return;
    memset(db, 0, sizeof(*db));
    db->schema = 1;
    memcpy(db->model, HB_VOICE_MODEL, sizeof(HB_VOICE_MODEL));
}
bool hb_voice_database_valid(const hb_voice_database *db)
{
    if (!db || db->schema != 1 || memcmp(db->model, HB_VOICE_MODEL, sizeof(HB_VOICE_MODEL)))
        return false;
    for (unsigned i = 0; i < HB_VOICE_PEOPLE; ++i)
    {
        const hb_voice_person *p = &db->people[i];
        if (p->count > HB_VOICE_TEMPLATES || (!p->id && p->count))
            return false;
        if (p->id)
            for (unsigned j = 0; j < i; ++j)
                if (db->people[j].id == p->id)
                    return false;
        for (unsigned j = 0; j < p->count; ++j)
            if (!valid(&p->samples[j]))
                return false;
    }
    return true;
}
bool hb_voice_enroll(hb_voice_database *db, uint64_t id, const hb_voice_embedding *e, uint64_t end,
                     const hb_voice_policy *p)
{
    if (!hb_voice_database_valid(db))
        return false;
    hb_voice_person *person = NULL, *empty = NULL;
    for (unsigned i = 0; i < HB_VOICE_PEOPLE; ++i)
    {
        if (db->people[i].id == id)
            person = &db->people[i];
        if (!db->people[i].id && !empty)
            empty = &db->people[i];
    }
    if (!person)
        person = empty;
    return hb_voice_person_add(person, id, e, end, p);
}
bool hb_voice_person_add(hb_voice_person *person, uint64_t id, const hb_voice_embedding *e,
                         uint64_t end, const hb_voice_policy *p)
{
    if (!person || !id || !valid(e) || !end || !hb_voice_policy_valid(p) ||
        (person->id && person->id != id) || person->count >= HB_VOICE_TEMPLATES ||
        (person->last_end && (end <= person->last_end || end - person->last_end < 3015)))
        return false;
    for (unsigned i = 0; i < person->count; ++i)
    {
        float score = hb_voice_similarity(e, &person->samples[i]);
        if (score < p->agreement || score >= p->duplicate)
            return false;
    }
    person->id = id;
    person->samples[person->count++] = *e;
    person->last_end = end;
    return true;
}
void hb_voice_delete(hb_voice_database *db, uint64_t id)
{
    if (db && id)
        for (unsigned i = 0; i < HB_VOICE_PEOPLE; ++i)
            if (db->people[i].id == id)
                memset(&db->people[i], 0, sizeof(db->people[i]));
}
hb_voice_match hb_voice_identify(const hb_voice_database *db, const hb_voice_embedding *e,
                                 const hb_voice_policy *p)
{
    hb_voice_match r = {HB_VOICE_UNAVAILABLE, 0, -1, -1};
    if (!hb_voice_database_valid(db) || !valid(e) || !hb_voice_policy_valid(p))
        return r;
    uint64_t best_id = 0;
    bool winner_agrees = false;
    for (unsigned i = 0; i < HB_VOICE_PEOPLE; ++i)
    {
        const hb_voice_person *person = &db->people[i];
        if (!person->id || person->count < 3 || person->count > HB_VOICE_TEMPLATES)
            continue;
        float best = -1;
        unsigned agree = 0;
        for (unsigned j = 0; j < person->count; ++j)
        {
            if (!valid(&person->samples[j]))
                return r;
            float score = hb_voice_similarity(e, &person->samples[j]);
            if (score > best)
                best = score;
            agree += score >= p->agreement;
        }
        if (best > r.best)
        {
            r.second = r.best;
            r.best = best;
            best_id = person->id;
            winner_agrees = agree >= 2;
        }
        else if (best > r.second)
            r.second = best;
    }
    r.state = HB_VOICE_UNKNOWN;
    if (best_id && r.best >= p->threshold)
    {
        r.state = r.best - r.second < p->margin ? HB_VOICE_AMBIGUOUS
                  : winner_agrees               ? HB_VOICE_KNOWN
                                                : HB_VOICE_UNKNOWN;
        if (r.state == HB_VOICE_KNOWN)
            r.person_id = best_id;
    }
    return r;
}
bool hb_voice_confirm(hb_voice_tracker *t, hb_voice_match m, uint32_t epoch, uint64_t end,
                      uint64_t now, const hb_voice_policy *p)
{
    if (!t)
        return false;
    if (!hb_voice_policy_valid(p) || !epoch || !end || end > now || now - end > 5000 ||
        m.state != HB_VOICE_KNOWN || !m.person_id ||
        (t->end_ms && (end <= t->end_ms || end - t->end_ms < 3015)))
    {
        memset(t, 0, sizeof(*t));
        return false;
    }
    if (t->epoch != epoch || t->person_id != m.person_id || (t->end_ms && end - t->end_ms > 10000))
        t->hits = 0;
    t->epoch = epoch;
    t->person_id = m.person_id;
    t->end_ms = end;
    if (t->hits < p->confirmations)
        ++t->hits;
    return t->hits >= p->confirmations;
}
