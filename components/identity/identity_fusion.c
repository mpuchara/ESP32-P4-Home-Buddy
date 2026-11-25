// SPDX-License-Identifier: Apache-2.0
#include "hb_identity_future.h"
#include <math.h>
static bool fresh(uint64_t at, uint64_t now)
{
    return at && at <= now && now - at <= 5000;
}
hb_identity_fusion_result hb_identity_fuse(const hb_identity_fusion_input *in, uint32_t session,
                                           uint64_t now, bool accepted)
{
    hb_identity_fusion_result r = {HB_FUSION_UNKNOWN, HB_ID_SOURCE_FACE, 0};
    if (!in || !session || in->session != session)
        return r;
    bool face_session = in->face_session == session;
    bool voice_session = in->voice_session == session;
    bool face = face_session && in->face_confirmed && in->face.state == HB_ID_KNOWN &&
                in->face.person_id && fresh(in->face_frame_ms, now);
    bool voice = accepted && voice_session && in->voice_confirmed &&
                 in->voice.state == HB_ID_KNOWN && in->voice.person_id &&
                 fresh(in->voice_window_end_ms, now);
    if (face && voice && in->face.person_id != in->voice.person_id)
    {
        r.status = HB_FUSION_CONFLICT;
        return r;
    }
    if (face || voice)
    {
        r.status = HB_FUSION_CONFIRMED;
        r.source = face && voice ? HB_ID_SOURCE_BOTH
                   : voice       ? HB_ID_SOURCE_VOICE
                                 : HB_ID_SOURCE_FACE;
        r.person_id = face ? in->face.person_id : in->voice.person_id;
    }
    else if ((face_session && in->face.state == HB_ID_AMBIGUOUS && fresh(in->face_frame_ms, now)) ||
             (accepted && voice_session && in->voice.state == HB_ID_AMBIGUOUS &&
              fresh(in->voice_window_end_ms, now)))
        r.status = HB_FUSION_AMBIGUOUS;
    return r;
}

static float unit(float value)
{
    if (!isfinite(value) || value <= 0)
        return 0;
    return value >= 1 ? 1 : value;
}
static float support(hb_id_match match, hb_fusion_evidence e)
{
    if (!isfinite(match.best) || !isfinite(match.second) || !isfinite(e.threshold) ||
        !isfinite(e.margin) || e.threshold <= 0 || e.threshold >= 1 || e.margin <= 0 ||
        e.margin >= 1 || match.best < e.threshold || match.best - match.second < e.margin)
        return 0;
    float excess = unit((match.best - e.threshold) / (1 - e.threshold));
    float gap = unit((match.best - match.second - e.margin) / (1 - e.margin));
    return (excess + gap) * 0.5f;
}
static bool usable(hb_id_match m, hb_fusion_evidence e)
{
    return m.state == HB_ID_KNOWN && m.person_id && unit(e.quality) > 0 &&
           isfinite(m.best) && isfinite(m.second) && isfinite(e.threshold) &&
           isfinite(e.margin) && e.threshold > 0 && e.threshold < 1 && e.margin > 0 &&
           e.margin < 1 && m.best >= e.threshold && m.best - m.second >= e.margin;
}
hb_fusion_trial hb_identity_trial_fuse(const hb_identity_fusion_input *in, uint32_t session,
                                       uint64_t now, hb_fusion_evidence face,
                                       hb_fusion_evidence voice)
{
    hb_fusion_trial out = {{HB_FUSION_UNKNOWN, HB_ID_SOURCE_FACE, 0}, 0, 0, 0};
    if (!in || !session || in->session != session)
        return out;
    hb_identity_fusion_input input = *in;
    bool f = input.face_session == session && fresh(input.face_frame_ms, now) &&
             usable(input.face, face);
    bool v = input.voice_session == session && fresh(input.voice_window_end_ms, now) &&
             usable(input.voice, voice);
    if (f && v)
    {
        uint64_t a = input.face_frame_ms, b = input.voice_window_end_ms;
        if ((a > b ? a - b : b - a) > 3000)
        {
            // Unrelated observations cannot corroborate or contradict one another.
            f = a > b;
            v = b > a;
        }
        else if (input.face.person_id != input.voice.person_id)
        {
            out.decision.status = HB_FUSION_CONFLICT;
            return out;
        }
    }
    input.face_confirmed &= f;
    input.voice_confirmed &= v;
    if (!f && input.face.state == HB_ID_KNOWN)
        input.face.state = HB_ID_UNAVAILABLE;
    if (!v && input.voice.state == HB_ID_KNOWN)
        input.voice.state = HB_ID_UNAVAILABLE;
    out.decision = hb_identity_fuse(&input, session, now, true);
    if (out.decision.status != HB_FUSION_CONFIRMED)
        return out;
    float fw = input.face_confirmed ? unit(face.quality) : 0;
    float vw = input.voice_confirmed ? unit(voice.quality) : 0;
    float total = fw + vw;
    if (total > 0)
    {
        out.face_weight = fw / total;
        out.voice_weight = vw / total;
        out.score = out.face_weight * support(input.face, face) +
                    out.voice_weight * support(input.voice, voice);
    }
    return out;
}
