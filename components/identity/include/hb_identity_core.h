// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
#define HB_ID_DIM 512
#define HB_ID_PEOPLE 8
#define HB_ID_TEMPLATES 15
#define HB_ID_MIN_SAMPLES 8
#define HB_ID_NAME 61
#define HB_ID_MODEL "mbf_s8_v1:p4:112:espdl-3.3.14:v1"
    typedef enum
    {
        HB_ID_UNAVAILABLE,
        HB_ID_UNKNOWN,
        HB_ID_AMBIGUOUS,
        HB_ID_KNOWN
    } hb_id_state;
    typedef struct
    {
        float x, y;
    } hb_id_point;
    // Espressif order: image-left eye, image-left mouth, nose, image-right eye, image-right mouth.
    typedef struct
    {
        hb_id_point p[5];
    } hb_id_landmarks;
    typedef struct
    {
        int x, y, w, h;
    } hb_id_roi;
    typedef struct
    {
        float v[HB_ID_DIM];
    } hb_id_embedding;
    typedef struct
    {
        uint64_t id;
        char name[HB_ID_NAME];
        bool enabled;
        unsigned count;
        hb_id_embedding templates[HB_ID_TEMPLATES];
        uint64_t last_greeting_ms;
        uint32_t last_sample_sequence;
        uint64_t last_sample_ms;
    } hb_id_person;
    typedef struct
    {
        char model[sizeof(HB_ID_MODEL)];
        uint64_t next_id;
        hb_id_person people[HB_ID_PEOPLE];
    } hb_id_database;
    typedef struct
    {
        float threshold, margin, agreement;
        unsigned confirmations;
    } hb_id_policy;
    typedef struct
    {
        hb_id_state state;
        uint64_t person_id;
        float best, second;
    } hb_id_match;
    typedef struct
    {
        uint32_t session, sequence;
        uint64_t frame_ms, completed_ms, person_id;
        unsigned hits;
    } hb_id_tracker;
    typedef struct
    {
        uint64_t target_id, last_ms;
        uint32_t last_sequence;
        unsigned count;
        bool verified;
        hb_id_embedding samples[HB_ID_TEMPLATES];
    } hb_id_enrollment;
    typedef struct
    {
        unsigned known_trials, unknown_trials, true_accept, false_reject, confusion;
        unsigned unknown_reject, unknown_accept, ambiguous, unavailable;
        uint32_t last_sequence;
        uint64_t last_frame_ms;
    } hb_id_evaluation;
    bool hb_id_evaluate(hb_id_evaluation *stats, uint64_t expected_id, hb_id_match match,
                        bool confirmed, uint32_t sequence, uint64_t frame_ms);
    bool hb_id_embedding_valid(const hb_id_embedding *e);
    float hb_id_similarity(const hb_id_embedding *a, const hb_id_embedding *b);
    bool hb_id_landmarks_valid(const hb_id_landmarks *l, unsigned w, unsigned h);
    bool hb_id_alignment_valid(const hb_id_landmarks *l);
    bool hb_id_raw_roi(const uint8_t *raw, size_t bytes, unsigned rw, unsigned rh, hb_id_roi roi,
                       uint8_t *rgb, size_t capacity);
    bool hb_id_roi_map(int x1, int y1, int x2, int y2, unsigned iw, unsigned ih, unsigned raw_w,
                       unsigned raw_h, hb_id_roi *out);
    // Inverse 90-degree CCW transform, upright pixel -> sensor pixel.
    bool hb_id_to_sensor(unsigned x, unsigned y, unsigned raw_w, unsigned raw_h, unsigned *sx,
                         unsigned *sy);
    void hb_id_database_init(hb_id_database *db, uint32_t boot_nonce);
    hb_id_person *hb_id_find(hb_id_database *db, uint64_t id);
    uint64_t hb_id_create(hb_id_database *db, const char *name);
    bool hb_id_rename(hb_id_database *db, uint64_t id, const char *name);
    bool hb_id_delete(hb_id_database *db, uint64_t id);
    hb_id_match hb_id_match_embedding(const hb_id_database *db, const hb_id_embedding *e,
                                      const hb_id_policy *policy);
    void hb_id_tracker_reset(hb_id_tracker *t, uint32_t session);
    bool hb_id_confirm(hb_id_tracker *t, hb_id_match match, uint32_t session, uint32_t sequence,
                       uint64_t frame_ms, uint64_t completed_ms, uint64_t now_ms,
                       const hb_id_policy *policy);
    bool hb_id_enroll_sample(hb_id_enrollment *en, const hb_id_embedding *e, unsigned faces,
                             bool alignment, uint32_t seq, uint64_t ms);
    bool hb_id_enroll_begin(hb_id_database *db, hb_id_enrollment *en, uint64_t person_id);
    bool hb_id_enroll_verify(hb_id_enrollment *en, const hb_id_embedding *e, uint32_t seq,
                             uint64_t ms, const hb_id_policy *policy);
    bool hb_id_enroll_commit(hb_id_database *db, hb_id_enrollment *en);
#ifdef __cplusplus
}
#endif
