// SPDX-License-Identifier: Apache-2.0
#include "hb_enrollment_flow.h"
#include <string.h>
bool hb_enroll_fresh(hb_id_database *db, hb_id_enrollment *en, uint64_t id)
{
    if (!hb_id_enroll_begin(db, en, id))
        return false;
    memset(en, 0, sizeof(*en));
    en->target_id = id;
    return true;
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
