// SPDX-License-Identifier: Apache-2.0
#include "hb_face.h"
#include <math.h>
bool hb_face_admit(uint32_t p, uint32_t pl, uint32_t i, uint32_t il, bool loading)
{
    return p >= (loading ? 16U : 12U) * 1024 * 1024 && pl >= (loading ? 8U : 4U) * 1024 * 1024 &&
           i >= (loading ? 192U : 64U) * 1024 && il >= (loading ? 128U : 32U) * 1024;
}
bool hb_face_clip(hb_face_box *b)
{
    if (!b || !isfinite(b->score) || b->score < 0 || b->score > 1)
        return false;
    if (b->x1 < 0)
        b->x1 = 0;
    if (b->y1 < 0)
        b->y1 = 0;
    if (b->x2 >= HB_FACE_WIDTH)
        b->x2 = HB_FACE_WIDTH - 1;
    if (b->y2 >= HB_FACE_HEIGHT)
        b->y2 = HB_FACE_HEIGHT - 1;
    return b->x1 < b->x2 && b->y1 < b->y2;
}
bool hb_face_fresh(const hb_face_status *s, uint64_t now)
{
    return s && s->enabled && s->ready && s->error == ESP_OK && s->frame_ms > 0 &&
           s->frame_ms <= s->completed_ms && s->completed_ms <= now && now - s->frame_ms <= 2500 &&
           now - s->completed_ms <= 1200 && s->count <= HB_FACE_MAX;
}

bool hb_face_map_box(hb_face_box *b)
{
    if (!b || !isfinite(b->score) || b->score < 0 || b->score > 1)
        return false;
    // Clip before integer products; SDK coordinates describe the original input.
    if (b->x1 < 0)
        b->x1 = 0;
    if (b->y1 < 0)
        b->y1 = 0;
    if (b->x2 >= HB_FACE_INPUT_WIDTH)
        b->x2 = HB_FACE_INPUT_WIDTH - 1;
    if (b->y2 >= HB_FACE_INPUT_HEIGHT)
        b->y2 = HB_FACE_INPUT_HEIGHT - 1;
    if (b->x1 >= b->x2 || b->y1 >= b->y2)
        return false;
    b->x1 = b->x1 * HB_FACE_WIDTH / HB_FACE_INPUT_WIDTH;
    b->x2 = b->x2 * HB_FACE_WIDTH / HB_FACE_INPUT_WIDTH;
    b->y1 = b->y1 * HB_FACE_HEIGHT / HB_FACE_INPUT_HEIGHT;
    b->y2 = b->y2 * HB_FACE_HEIGHT / HB_FACE_INPUT_HEIGHT;
    return hb_face_clip(b);
}
