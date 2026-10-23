// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
    enum
    {
        HB_FQ_EDGE = 1,
        HB_FQ_SMALL = 2,
        HB_FQ_DARK = 4,
        HB_FQ_BRIGHT = 8,
        HB_FQ_LOW_CONTRAST = 16,
        HB_FQ_LOW_DETAIL = 32
    };
    typedef struct
    {
        bool valid;
        uint16_t width, height;
        uint32_t flags;
        float mean, contrast, detail, near_limit_percent;
    } hb_face_quality;
    // Metrics on post-appearance RGB565 input. Flags are uncalibrated diagnostic hints only.
    hb_face_quality hb_face_quality_measure(const uint16_t *pixels, size_t capacity, unsigned width,
                                            unsigned height, int x1, int y1, int x2, int y2);
#ifdef __cplusplus
}
#endif
