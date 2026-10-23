// SPDX-License-Identifier: Apache-2.0
#include "hb_identity_core.h"
#include <math.h>
#include <string.h>
bool hb_id_evaluate(hb_id_evaluation *s, uint64_t expected, hb_id_match match, bool confirmed,
                    uint32_t sequence, uint64_t frame_ms)
{
    if (!s || !sequence || sequence <= s->last_sequence || !frame_ms ||
        frame_ms <= s->last_frame_ms)
        return false;
    s->last_sequence = sequence;
    s->last_frame_ms = frame_ms;
    if (match.state == HB_ID_UNAVAILABLE)
    {
        ++s->unavailable;
        return true;
    }
    bool accepted = match.state == HB_ID_KNOWN && match.person_id && confirmed;
    if (match.state == HB_ID_AMBIGUOUS)
        ++s->ambiguous;
    if (expected)
    {
        ++s->known_trials;
        if (accepted && match.person_id == expected)
            ++s->true_accept;
        else if (accepted)
            ++s->confusion;
        else
            ++s->false_reject;
    }
    else
    {
        ++s->unknown_trials;
        if (accepted)
            ++s->unknown_accept;
        else
            ++s->unknown_reject;
    }
    return true;
}

static bool name_valid(const char *s)
{
    if (!s || !s[0] || strlen(s) >= HB_ID_NAME)
        return false;
    bool visible = false;
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p)
    {
        if (*p < 32 || *p == 127)
            return false;
        visible |= *p != ' ';
    }
    return visible;
}
bool hb_id_embedding_valid(const hb_id_embedding *e)
{
    if (!e)
        return false;
    double norm = 0;
    for (unsigned i = 0; i < HB_ID_DIM; ++i)
    {
        if (!isfinite(e->v[i]))
            return false;
        norm += (double)e->v[i] * e->v[i];
    }
    // Upstream FeatPostprocessor performs L2 normalization. Reject broken output.
    return norm >= 0.98 && norm <= 1.02;
}
float hb_id_similarity(const hb_id_embedding *a, const hb_id_embedding *b)
{
    if (!hb_id_embedding_valid(a) || !hb_id_embedding_valid(b))
        return NAN;
    double sum = 0;
    for (unsigned i = 0; i < HB_ID_DIM; ++i)
        sum += (double)a->v[i] * b->v[i];
    return (float)fmax(-1, fmin(1, sum));
}
bool hb_id_landmarks_valid(const hb_id_landmarks *l, unsigned w, unsigned h)
{
    if (!l || w < 32 || h < 32 || w > 4096 || h > 4096)
        return false;
    for (unsigned i = 0; i < 5; ++i)
        if (!isfinite(l->p[i].x) || !isfinite(l->p[i].y) || l->p[i].x < 1 || l->p[i].y < 1 ||
            l->p[i].x >= w - 1 || l->p[i].y >= h - 1)
            return false;
    hb_id_point le = l->p[0], lm = l->p[1], n = l->p[2], re = l->p[3], rm = l->p[4];
    float dx = re.x - le.x, dy = re.y - le.y;
    float eye2 = dx * dx + dy * dy;
    if (dx < 4 || eye2 < 64 || lm.x >= rm.x)
        return false;
    // Signed areas reject mirrored, collinear and inverted arrangements; allow modest roll.
    float nose_cross = dx * (n.y - le.y) - dy * (n.x - le.x);
    float mouth_cross = dx * ((lm.y + rm.y) / 2 - le.y) - dy * ((lm.x + rm.x) / 2 - le.x);
    return nose_cross > eye2 * 0.05f && mouth_cross > nose_cross && mouth_cross < eye2 * 3 &&
           n.x > le.x - dx * 0.25f && n.x < re.x + dx * 0.25f && fabsf(dy) < dx &&
           hb_id_alignment_valid(l);
}
bool hb_id_alignment_valid(const hb_id_landmarks *l)
{
    if (!l)
        return false;
    static const hb_id_point reference[5] = {{38.2946f, 51.6963f},
                                             {41.5493f, 92.3655f},
                                             {56.0252f, 71.7366f},
                                             {73.5318f, 51.5014f},
                                             {70.7299f, 92.2041f}};
    float cx = 0, cy = 0, dx = 0, dy = 0;
    for (unsigned i = 0; i < 5; ++i)
    {
        if (!isfinite(l->p[i].x) || !isfinite(l->p[i].y))
            return false;
        cx += reference[i].x / 5;
        cy += reference[i].y / 5;
        dx += l->p[i].x / 5;
        dy += l->p[i].y / 5;
    }
    float denom = 0, a = 0, b = 0;
    for (unsigned i = 0; i < 5; ++i)
    {
        float x = reference[i].x - cx, y = reference[i].y - cy;
        float u = l->p[i].x - dx, v = l->p[i].y - dy;
        denom += x * x + y * y;
        a += x * u + y * v;
        b += x * v - y * u;
    }
    a /= denom;
    b /= denom;
    float scale2 = a * a + b * b;
    if (!isfinite(scale2) || scale2 < 0.01f || scale2 > 400)
        return false;
    float error = 0;
    for (unsigned i = 0; i < 5; ++i)
    {
        float x = reference[i].x - cx, y = reference[i].y - cy;
        float ex = a * x - b * y + dx - l->p[i].x;
        float ey = b * x + a * y + dy - l->p[i].y;
        error += ex * ex + ey * ey;
    }
    // RMS residual <= 25% of canonical inter-eye distance. Geometric heuristic, not calibrated
    // quality.
    return error / 5 <= scale2 * 35.24f * 35.24f * 0.0625f;
}
static unsigned linear(unsigned v)
{
    return v > 16 ? (v - 16) * 255 / 239 : 0;
}
static void bayer(const uint8_t *raw, unsigned rw, unsigned sx, unsigned sy, unsigned c[3])
{
    const uint8_t *p = raw + (size_t)(sy & ~1U) * rw + (sx & ~1U);
    c[0] = linear(p[rw]);
    c[1] = (linear(p[0]) + linear(p[rw + 1])) / 2;
    c[2] = linear(p[1]);
}
bool hb_id_raw_roi(const uint8_t *raw, size_t bytes, unsigned rw, unsigned rh, hb_id_roi roi,
                   uint8_t *rgb, size_t capacity)
{
    if (!raw || !rgb || rw < 32 || rh < 32 || rw > 4096 || rh > 4096 || (rw & 1) || (rh & 1) ||
        bytes < (size_t)rw * rh || roi.x < 0 || roi.y < 0 || roi.w < 32 || roi.h < 32 ||
        roi.w > 640 || roi.h > 640 || roi.x > (int)rh - roi.w || roi.y > (int)rw - roi.h ||
        capacity < (size_t)roi.w * roi.h * 3)
        return false;
    // Native Bayer cells, black-level correction, bounded gray-world WB and sRGB LUT.
    // This is intentionally independent of detection's RGB565 quantization.
    float sum[3] = {0};
    unsigned accepted = 0;
    for (unsigned y = 0; y < 12; ++y)
        for (unsigned x = 0; x < 16; ++x)
        {
            unsigned sx, sy, c[3];
            hb_id_to_sensor(roi.x + x * (roi.w - 1) / 15, roi.y + y * (roi.h - 1) / 11, rw, rh, &sx,
                            &sy);
            bayer(raw, rw, sx, sy, c);
            if (c[0] < 16 || c[1] < 16 || c[2] < 16 || c[0] > 239 || c[1] > 239 || c[2] > 239)
                continue;
            for (unsigned j = 0; j < 3; ++j)
                sum[j] += c[j];
            ++accepted;
        }
    float gain[3] = {1, 1, 1};
    if (accepted >= 24)
    {
        gain[0] = fminf(3, fmaxf(0.5f, sum[1] / sum[0]));
        gain[2] = fminf(3, fmaxf(0.5f, sum[1] / sum[2]));
    }
    uint8_t lut[3][256];
    for (unsigned j = 0; j < 3; ++j)
        for (unsigned i = 0; i < 256; ++i)
        {
            float v = fminf(1, i * gain[j] / 255);
            lut[j][i] = (uint8_t)lroundf(
                255 * (v <= 0.0031308f ? 12.92f * v : 1.055f * powf(v, 1 / 2.4f) - 0.055f));
        }
    for (int y = 0; y < roi.h; ++y)
        for (int x = 0; x < roi.w; ++x)
        {
            unsigned sx, sy, c[3];
            hb_id_to_sensor(roi.x + x, roi.y + y, rw, rh, &sx, &sy);
            bayer(raw, rw, sx, sy, c);
            uint8_t *d = rgb + ((size_t)y * roi.w + x) * 3;
            for (unsigned j = 0; j < 3; ++j)
                d[j] = lut[j][c[j]];
        }
    return true;
}
bool hb_id_to_sensor(unsigned x, unsigned y, unsigned rw, unsigned rh, unsigned *sx, unsigned *sy)
{
    if (!sx || !sy || !rw || !rh || x >= rh || y >= rw)
        return false;
    *sx = rw - 1 - y;
    *sy = x;
    return true;
}
bool hb_id_roi_map(int x1, int y1, int x2, int y2, unsigned iw, unsigned ih, unsigned rw,
                   unsigned rh, hb_id_roi *out)
{
    if (!out || !iw || !ih || iw > 4096 || ih > 4096 || rw < 32 || rh < 32 || rw > 4096 ||
        rh > 4096 || x1 < 0 || y1 < 0 || x2 <= x1 || y2 <= y1 || x2 >= (int)iw || y2 >= (int)ih)
        return false;
    // Match hb_raw_thumbnail_upright's (dimension - 2) Bayer-cell sampling convention.
    int left = x1 * (rh - 2) / iw;
    int top = y1 * (rw - 2) / ih;
    int right = (x2 + 1) * (rh - 2) / iw;
    int bottom = (y2 + 1) * (rw - 2) / ih;
    int mx = (right - left) / 5, my = (bottom - top) / 5;
    left = left > mx ? left - mx : 0;
    top = top > my ? top - my : 0;
    right = right + mx < (int)rh ? right + mx : (int)rh;
    bottom = bottom + my < (int)rw ? bottom + my : (int)rw;
    *out = (hb_id_roi){left, top, right - left, bottom - top};
    // Bounded native-resolution ROI, never downsample it to the detection thumbnail.
    return out->w >= 32 && out->h >= 32 && out->w <= 640 && out->h <= 640;
}
void hb_id_database_init(hb_id_database *db, uint32_t nonce)
{
    memset(db, 0, sizeof(*db));
    memcpy(db->model, HB_ID_MODEL, sizeof(db->model));
    db->next_id = ((uint64_t)nonce << 32) | 1;
}
hb_id_person *hb_id_find(hb_id_database *db, uint64_t id)
{
    if (!db || !id || memcmp(db->model, HB_ID_MODEL, sizeof(db->model)))
        return NULL;
    for (unsigned i = 0; i < HB_ID_PEOPLE; ++i)
        if (db->people[i].id == id)
            return &db->people[i];
    return NULL;
}
uint64_t hb_id_create(hb_id_database *db, const char *name)
{
    if (!db || memcmp(db->model, HB_ID_MODEL, sizeof(db->model)) || !name_valid(name) ||
        !db->next_id || db->next_id == UINT64_MAX)
        return 0;
    for (unsigned i = 0; i < HB_ID_PEOPLE; ++i)
        if (!db->people[i].id)
        {
            hb_id_person *p = &db->people[i];
            memset(p, 0, sizeof(*p));
            p->id = db->next_id++;
            p->enabled = true;
            strcpy(p->name, name);
            return p->id;
        }
    return 0;
}
bool hb_id_rename(hb_id_database *db, uint64_t id, const char *name)
{
    hb_id_person *p = hb_id_find(db, id);
    if (!p || !name_valid(name))
        return false;
    strcpy(p->name, name);
    return true;
}
bool hb_id_delete(hb_id_database *db, uint64_t id)
{
    hb_id_person *p = hb_id_find(db, id);
    if (!p)
        return false;
    memset(p, 0, sizeof(*p));
    return true;
}
static bool policy_valid(const hb_id_policy *p)
{
    return p && isfinite(p->threshold) && isfinite(p->margin) && isfinite(p->agreement) &&
           p->threshold >= -1 && p->threshold <= 1 && p->margin > 0 && p->margin <= 2 &&
           p->agreement >= -1 && p->agreement <= p->threshold && p->confirmations >= 3 &&
           p->confirmations <= 10;
}
hb_id_match hb_id_match_embedding(const hb_id_database *db, const hb_id_embedding *e,
                                  const hb_id_policy *policy)
{
    hb_id_match r = {HB_ID_UNAVAILABLE, 0, -1, -1};
    if (!db || memcmp(db->model, HB_ID_MODEL, sizeof(db->model)) || !policy_valid(policy) ||
        !hb_id_embedding_valid(e))
        return r;
    r.state = HB_ID_UNKNOWN;
    unsigned best_agreement = 0;
    for (unsigned i = 0; i < HB_ID_PEOPLE; ++i)
    {
        const hb_id_person *p = &db->people[i];
        if (!p->id || !p->enabled || p->count < HB_ID_MIN_SAMPLES)
            continue;
        if (p->count > HB_ID_TEMPLATES)
        {
            r.state = HB_ID_UNAVAILABLE;
            return r;
        }
        float top[3] = {-1, -1, -1};
        unsigned agreement = 0;
        for (unsigned j = 0; j < p->count; ++j)
        {
            float s = hb_id_similarity(e, &p->templates[j]);
            if (!isfinite(s))
            {
                r.state = HB_ID_UNAVAILABLE;
                r.person_id = 0;
                return r;
            }
            agreement += s >= policy->agreement;
            for (unsigned k = 0; k < 3; ++k)
                if (s > top[k])
                {
                    for (unsigned z = 2; z > k; --z)
                        top[z] = top[z - 1];
                    top[k] = s;
                    break;
                }
        }
        // Fixed top-three mean, never a sum over a person's template count.
        float score = (top[0] + top[1] + top[2]) / 3;
        if (score > r.best)
        {
            r.second = r.best;
            r.best = score;
            r.person_id = p->id;
            best_agreement = agreement;
        }
        else if (score > r.second)
            r.second = score;
    }
    if (r.best >= policy->threshold && best_agreement >= 3)
        r.state = r.best - r.second < policy->margin ? HB_ID_AMBIGUOUS : HB_ID_KNOWN;
    if (r.state != HB_ID_KNOWN)
        r.person_id = 0;
    return r;
}
void hb_id_tracker_reset(hb_id_tracker *t, uint32_t session)
{
    memset(t, 0, sizeof(*t));
    t->session = session;
}
bool hb_id_confirm(hb_id_tracker *t, hb_id_match m, uint32_t session, uint32_t seq, uint64_t frame,
                   uint64_t completed, uint64_t now, const hb_id_policy *p)
{
    if (!t || !policy_valid(p))
        return false;
    if (session != t->session)
        return false;
    if (!seq || !frame || frame > completed || completed > now || now - frame > 5000 ||
        seq <= t->sequence || frame <= t->frame_ms || completed <= t->completed_ms)
    {
        t->hits = 0;
        t->person_id = 0;
        return false;
    }
    if (t->frame_ms && frame - t->frame_ms > 6000)
        t->hits = 0;
    t->sequence = seq;
    t->frame_ms = frame;
    t->completed_ms = completed;
    if (m.state != HB_ID_KNOWN || !m.person_id)
    {
        t->hits = 0;
        t->person_id = 0;
        return false;
    }
    if (t->person_id != m.person_id)
    {
        t->hits = 0;
        t->person_id = m.person_id;
    }
    if (t->hits < p->confirmations)
        ++t->hits;
    return t->hits >= p->confirmations;
}
bool hb_id_enroll_sample(hb_id_enrollment *en, const hb_id_embedding *e, unsigned faces,
                         bool alignment, uint32_t seq, uint64_t ms)
{
    if (!en || !en->target_id || faces != 1 || !alignment || !hb_id_embedding_valid(e) ||
        en->count >= HB_ID_TEMPLATES || !seq || seq <= en->last_sequence || !ms ||
        ms <= en->last_ms || (en->last_ms && ms - en->last_ms < 1000))
        return false;
    for (unsigned i = 0; i < en->count; ++i)
        if (hb_id_similarity(e, &en->samples[i]) > 0.995f)
            return false;
    en->samples[en->count++] = *e;
    en->last_sequence = seq;
    en->last_ms = ms;
    en->verified = false;
    return true;
}
bool hb_id_enroll_verify(hb_id_enrollment *en, const hb_id_embedding *e, uint32_t seq, uint64_t ms,
                         const hb_id_policy *p)
{
    if (en)
        en->verified = false;
    if (!en || en->count < HB_ID_MIN_SAMPLES || en->count > HB_ID_TEMPLATES || !policy_valid(p) ||
        !hb_id_embedding_valid(e) || seq <= en->last_sequence || ms <= en->last_ms ||
        ms - en->last_ms < 1000)
        return false;
    unsigned hits = 0;
    for (unsigned i = 0; i < en->count; ++i)
        hits += hb_id_similarity(e, &en->samples[i]) >= p->threshold;
    en->verified = hits >= 3;
    return en->verified;
}
bool hb_id_enroll_commit(hb_id_database *db, hb_id_enrollment *en)
{
    if (!db || !en || !en->verified || en->count < HB_ID_MIN_SAMPLES || en->count > HB_ID_TEMPLATES)
        return false;
    hb_id_person *p = hb_id_find(db, en->target_id);
    if (!p)
        return false;
    for (unsigned i = 0; i < en->count; ++i)
        if (!hb_id_embedding_valid(&en->samples[i]))
            return false;
    // Commit all samples together. Cancellation before this call leaves existing templates intact.
    memcpy(p->templates, en->samples, en->count * sizeof(hb_id_embedding));
    p->count = en->count;
    memset(en, 0, sizeof(*en));
    return true;
}
