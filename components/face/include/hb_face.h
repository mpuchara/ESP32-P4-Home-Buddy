// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include "hb_face_demand.h"
#include "hb_face_quality.h"
#include "hb_presence.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define HB_FACE_WIDTH 180
#define HB_FACE_HEIGHT 318
#define HB_FACE_MAX 4
// Portrait sampling retains sensor aspect; ESPDet handles its own square letterbox.
#define HB_FACE_INPUT_WIDTH 234
#define HB_FACE_INPUT_HEIGHT 416
#define HB_FACE_INPUT_BYTES (HB_FACE_INPUT_WIDTH * HB_FACE_INPUT_HEIGHT * 2)
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
        bool enabled, ready, diagnostic_enabled;
        uint32_t monitor_remaining_ms;
        esp_err_t error;
        uint32_t sequence, inference_us, quality_us, count;
        uint64_t frame_ms, completed_ms;
        hb_face_box boxes[HB_FACE_MAX];
        hb_face_quality quality[HB_FACE_MAX];
        hb_presence_status presence;
    } hb_face_status;
    // The page and bounded background test are independent owners of one worker.
    esp_err_t hb_face_enable(bool enabled);
    esp_err_t hb_face_monitor(bool enabled);
    void hb_face_get(hb_face_status *status);
    // Pure admission and bounds helpers are also used by host regression tests.
    bool hb_face_admit(uint32_t psram_free, uint32_t psram_largest, uint32_t internal_free,
                       uint32_t internal_largest, bool loading);
    bool hb_face_clip(hb_face_box *box);
    bool hb_face_map_box(hb_face_box *box);
    bool hb_face_fresh(const hb_face_status *status, uint64_t now_ms);
#ifdef __cplusplus
}
#endif
