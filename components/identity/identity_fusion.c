// SPDX-License-Identifier: Apache-2.0
#include "hb_identity_future.h"
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
