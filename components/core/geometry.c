// SPDX-License-Identifier: Apache-2.0
#include "hb_geometry.h"
#include <limits.h>
#include <stddef.h>

bool hb_cover(uint32_t sw, uint32_t sh, uint32_t tw, uint32_t th, hb_cover_t *out)
{
    if (!sw || !sh || !tw || !th || !out)
        return false;
    uint64_t w = tw, h = th;
    if ((uint64_t)tw * sh >= (uint64_t)th * sw)
    {
        h = ((uint64_t)sh * tw + sw - 1) / sw;
    }
    else
    {
        w = ((uint64_t)sw * th + sh - 1) / sh;
    }
    if (w > UINT32_MAX || h > UINT32_MAX)
        return false;
    *out = (hb_cover_t){(uint32_t)w, (uint32_t)h, (uint32_t)(w - tw) / 2, (uint32_t)(h - th) / 2};
    return true;
}

bool hb_touch_landscape(int px, int py, int *x, int *y)
{
    if (!x || !y || px < 0 || px >= 480 || py < 0 || py >= 800)
        return false;
    *x = py;
    *y = 479 - px;
    return true;
}
