// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "esp_err.h"
#include "hb_face_quality.h"
#include "hb_identity_core.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum
    {
        HB_ID_STOP,
        HB_ID_TEST,
        HB_ID_BACKGROUND,
        HB_ID_ENROLL
    } hb_id_mode;
    typedef enum
    {
        HB_ID_CMD_CREATE,
        HB_ID_CMD_RENAME,
        HB_ID_CMD_DELETE,
        HB_ID_CMD_BEGIN,
        HB_ID_CMD_SAMPLE,
        HB_ID_CMD_VERIFY,
        HB_ID_CMD_SAVE,
        HB_ID_CMD_CANCEL,
        HB_ID_CMD_TEST,
        HB_ID_CMD_BACKGROUND,
        HB_ID_CMD_STOP,
        HB_ID_CMD_LANDMARK_METHOD,
        HB_ID_CMD_EVALUATE,
        HB_ID_CMD_EXPECTED
    } hb_id_command_type;
    typedef struct
    {
        hb_id_command_type type;
        uint64_t person_id;
        char name[HB_ID_NAME];
    } hb_id_command;
    typedef struct
    {
        uint64_t id;
        char name[HB_ID_NAME];
        unsigned samples;
    } hb_id_profile;
    typedef struct
    {
        hb_id_mode mode;
        hb_id_match match;
        bool ready, confirmed, verified, official_landmarks, detection_valid;
        unsigned samples, faces, profile_count;
        uint32_t session, sequence, detection_us, landmarks_us, embedding_us, total_us;
        uint64_t frame_ms, completed_ms, confirmed_id;
        uint32_t psram_free, psram_largest, internal_free, internal_largest;
        uint32_t psram_load_delta, internal_load_delta;
        hb_id_landmarks landmarks; // Full upright sensor coordinates.
        hb_id_roi roi;
        hb_face_quality quality;
        uint64_t expected_id;
        hb_id_evaluation evaluation;
        hb_id_profile profiles[HB_ID_PEOPLE];
        char message[160];
    } hb_id_status;
    // Bounded command queue; never performs inference in the caller. No LVGL calls in worker.
    esp_err_t hb_identity_command(const hb_id_command *command);
    void hb_identity_get(hb_id_status *out);
    bool hb_identity_preview(uint16_t *rgb565, size_t pixels, hb_id_status *status);
    // Called from UI thread. Revalidates session/freshness/profile and per-person cooldown.
    bool hb_identity_greeting(uint64_t now_ms, char name[HB_ID_NAME]);
    // Local in-process event only. No MQTT/network publication.
    typedef enum
    {
        HB_ID_SOURCE_FACE,
        HB_ID_SOURCE_VOICE,
        HB_ID_SOURCE_BOTH
    } hb_id_source;
    typedef enum
    {
        HB_ID_EVENT_RECOGNIZED,
        HB_ID_EVENT_UNKNOWN,
        HB_ID_EVENT_AMBIGUOUS,
        HB_ID_EVENT_ENTERED,
        HB_ID_EVENT_LEFT
    } hb_id_event_kind;
    typedef struct
    {
        uint64_t person_id, timestamp_ms;
        uint32_t session, sequence;
        hb_id_state state;
        float similarity, second;
        bool confirmed;
        hb_id_source source;
        hb_id_event_kind kind;
    } hb_id_event; // Source is FACE; VOICE/BOTH are intentionally not emitted.
    bool hb_identity_event(hb_id_event *out);
#ifdef __cplusplus
}
#endif
