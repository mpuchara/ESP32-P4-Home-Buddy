// SPDX-License-Identifier: Apache-2.0
#include "hb_enrollment_flow.h"
#include <math.h>
#include <string.h>
_Static_assert(HB_ID_TEMPLATES == 15 && HB_ID_MIN_SAMPLES == 8 && sizeof(float) == 4,
               "Review coherent-selection scratch and search bounds after capacity changes");
bool hb_enroll_fresh(hb_id_database *db, hb_id_enrollment *en, uint64_t id)
{
    if (!hb_id_enroll_begin(db, en, id))
        return false;
    memset(en, 0, sizeof(*en));
    en->target_id = id;
    return true;
}
static unsigned bits(unsigned mask)
{
    unsigned n = 0;
    for (; mask; mask &= mask - 1)
        ++n;
    return n;
}
static bool supported(const float scores[HB_ID_TEMPLATES][HB_ID_TEMPLATES], unsigned count,
                      unsigned mask, unsigned size, const hb_id_policy *policy)
{
    for (unsigned i = 0; i < count; ++i)
    {
        if (!(mask & (1U << i)))
            continue;
        float top[3] = {-1, -1, -1};
        for (unsigned j = 0; j < count; ++j)
        {
            if (j == i || !(mask & (1U << j)))
                continue;
            float score = scores[i][j];
            // One agreeing cohort, not two disjoint groups that each self-match.
            if (score < policy->agreement)
                return false;
            for (unsigned k = 0; k < 3; ++k)
                if (score > top[k])
                {
                    for (unsigned z = 2; z > k; --z)
                        top[z] = top[z - 1];
                    top[k] = score;
                    break;
                }
        }
        if (size >= 4 && (top[0] + top[1] + top[2]) / 3 < policy->threshold)
            return false;
    }
    return true;
}
bool hb_enroll_sample_coherent(hb_id_enrollment *en, const hb_id_embedding *e, unsigned faces,
                               bool alignment, uint32_t sequence, uint64_t frame_ms,
                               const hb_id_policy *policy, unsigned *progress, bool *ready)
{
    if (!progress || !ready || !hb_id_policy_valid(policy) ||
        !hb_id_enroll_sample_rolling(en, e, faces, alignment, sequence, frame_ms))
        return false;
    *ready = false;
    *progress = 0;
    // Fixed 900-byte worker scratch, no allocation or persistent schema change.
    float scores[HB_ID_TEMPLATES][HB_ID_TEMPLATES] = {0};
    for (unsigned i = 0; i < en->count; ++i)
        for (unsigned j = 0; j < i; ++j)
        {
            float score = hb_id_similarity(&en->samples[i], &en->samples[j]);
            if (!isfinite(score))
                return false;
            scores[i][j] = scores[j][i] = score;
        }
    unsigned best = 0;
    // At most 32767 subsets of the existing 15 slots; stop at the first full set.
    for (unsigned mask = 1; mask < (1U << en->count); ++mask)
    {
        unsigned size = bits(mask);
        if (size <= *progress || size > HB_ID_MIN_SAMPLES ||
            !supported(scores, en->count, mask, size, policy))
            continue;
        best = mask;
        *progress = size;
        if (size == HB_ID_MIN_SAMPLES)
            break;
    }
    if (*progress != HB_ID_MIN_SAMPLES)
        return true;
    unsigned target = 0;
    for (unsigned i = 0; i < en->count; ++i)
        if (best & (1U << i))
        {
            // Sorted source indices are >= target, so no vector-sized temporary.
            if (target != i)
                en->samples[target] = en->samples[i];
            ++target;
        }
    memset(en->samples + target, 0, (HB_ID_TEMPLATES - target) * sizeof(*en->samples));
    en->count = target;
    *ready = true;
    return true;
}
hb_enroll_admission hb_enroll_sample_stable(hb_id_enrollment *en, const hb_id_embedding *e,
                                            unsigned faces, uint32_t sequence, uint64_t frame_ms)
{
    if (!en || !en->target_id || en->verified || en->count >= HB_ID_MIN_SAMPLES || faces != 1 ||
        !hb_id_embedding_valid(e) || !sequence || sequence <= en->last_sequence || !frame_ms ||
        frame_ms <= en->last_ms || (en->last_ms && frame_ms - en->last_ms < 1000))
        return HB_ENROLL_INVALID;
    unsigned anchors = en->count < 3 ? en->count : 3;
    unsigned votes = 0, anchor_support = 0;
    float best = -1;
    for (unsigned i = 0; i < en->count; ++i)
    {
        float score = hb_id_similarity(e, &en->samples[i]);
        if (!isfinite(score))
            return HB_ENROLL_INVALID;
        if (score > 0.995f)
            return HB_ENROLL_DUPLICATE;
        best = fmaxf(best, score);
        if (i < anchors)
        {
            votes += score >= (anchors < 3 ? 0.60f : 0.50f);
            anchor_support += score >= 0.43f;
        }
    }
    // A strong match to an accepted pose can support a different pose, but
    // never accept a chain of samples detached from the original three anchors.
    bool supported = votes >= (anchors < 3 ? anchors : 2);
    if (anchors == 3 && best >= 0.65f && anchor_support >= 2)
        supported = true;
    if (!supported)
        return HB_ENROLL_MISMATCH;
    bool accepted = hb_id_enroll_sample(en, e, faces, true, sequence, frame_ms);
    return accepted ? HB_ENROLL_ACCEPTED : HB_ENROLL_INVALID;
}
bool hb_enroll_verify_diverse(hb_id_enrollment *en, const hb_id_embedding *e, uint32_t seq,
                              uint64_t ms)
{
    if (en)
        en->verified = false;
    if (!en || !en->target_id || en->count < HB_ID_MIN_SAMPLES || en->count > HB_ID_TEMPLATES ||
        !hb_id_embedding_valid(e) || seq <= en->last_sequence || ms <= en->last_ms ||
        ms - en->last_ms < 1000)
        return false;
    float best = -1;
    unsigned support = 0;
    for (unsigned i = 0; i < en->count; ++i)
    {
        float score = hb_id_similarity(e, &en->samples[i]);
        if (!isfinite(score))
            return false;
        best = fmaxf(best, score);
        support += score >= 0.55f;
    }
    en->verified = best >= 0.65f && support >= 3;
    return en->verified;
}
hb_enroll_step hb_enroll_next(const hb_id_enrollment *en, unsigned faces, bool valid,
                              uint32_t sequence, uint64_t frame_ms)
{
    if (!en || !en->target_id)
        return HB_ENROLL_WAIT;
    if (en->verified)
        return HB_ENROLL_CONFIRM;
    if (!valid || faces != 1 || !sequence || sequence <= en->last_sequence || !frame_ms ||
        frame_ms <= en->last_ms || (en->last_ms && frame_ms - en->last_ms < 1000))
        return HB_ENROLL_WAIT;
    return en->count < HB_ID_MIN_SAMPLES ? HB_ENROLL_SAMPLE : HB_ENROLL_VERIFY;
}
