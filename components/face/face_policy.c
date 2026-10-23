// SPDX-License-Identifier: Apache-2.0
#include "hb_face.h"
#include <math.h>
#include <string.h>
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
    return s && s->enabled && s->ready && s->error == ESP_OK && s->frame_ms <= now &&
           now - s->frame_ms <= 750 && s->count <= HB_FACE_MAX;
}

bool hb_face_letterbox(uint16_t *p, size_t capacity)
{
    if (!p || capacity < HB_FACE_INPUT_BYTES)
        return false;
    // Grow compact portrait rows bottom-up into the same bounded canvas.
    for (int y = HB_FACE_HEIGHT - 1; y >= 0; y--)
    {
        uint16_t *row = p + (size_t)y * HB_FACE_INPUT_WIDTH;
        memmove(row + HB_FACE_PAD_X, p + (size_t)y * HB_FACE_WIDTH, HB_FACE_WIDTH * 2);
        memset(row, 0, HB_FACE_PAD_X * 2);
        memset(row + HB_FACE_PAD_X + HB_FACE_WIDTH, 0, HB_FACE_PAD_X * 2);
    }
    return true;
}
