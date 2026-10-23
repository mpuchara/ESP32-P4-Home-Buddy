// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define HB_FACE_WIDTH 180
#define HB_FACE_HEIGHT 318
#define HB_FACE_MAX 4
// MSR input aspect is4:3; center the portrait thumbnail without distorting faces.
#define HB_FACE_INPUT_WIDTH 424
#define HB_FACE_PAD_X 122
#define HB_FACE_INPUT_BYTES (HB_FACE_INPUT_WIDTH * HB_FACE_HEIGHT * 2)
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct
    {
        int x1, y1, x2, y2;
        float score;
    } hb_face_box;
    typedef struct
    {
        bool enabled, ready;
        esp_err_t error;
        uint32_t sequence, inference_us, count;
        uint64_t frame_ms;
        hb_face_box boxes[HB_FACE_MAX];
    } hb_face_status;
    esp_err_t hb_face_enable(bool enabled);
    void hb_face_get(hb_face_status *status);
    // Pure admission and bounds helpers are also used by host regression tests.
    bool hb_face_admit(uint32_t psram_free, uint32_t psram_largest, uint32_t internal_free,
                       uint32_t internal_largest, bool loading);
    bool hb_face_clip(hb_face_box *box);
    bool hb_face_letterbox(uint16_t *pixels, size_t capacity_bytes);
    bool hb_face_fresh(const hb_face_status *status, uint64_t now_ms);
#ifdef __cplusplus
}
#endif
