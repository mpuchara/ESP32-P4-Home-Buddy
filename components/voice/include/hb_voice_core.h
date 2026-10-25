// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
#define HB_VOICE_SAMPLES 48240
#define HB_VOICE_FRAMES 300
#define HB_VOICE_BINS 80
#define HB_VOICE_DIM 512
#define HB_VOICE_PEOPLE 8
#define HB_VOICE_TEMPLATES 4
#define HB_VOICE_MODEL "sv_tdnn_tiny_3s:p4:736ca24b:v1"
    typedef bool (*hb_voice_cancel_fn)(void *context);
    typedef struct
    {
        float v[HB_VOICE_DIM];
    } hb_voice_embedding;
    typedef enum
    {
        HB_VOICE_UNAVAILABLE,
        HB_VOICE_UNKNOWN,
        HB_VOICE_AMBIGUOUS,
        HB_VOICE_KNOWN
    } hb_voice_state;
    typedef struct
    {
        hb_voice_state state;
        uint64_t person_id;
        float best, second;
    } hb_voice_match;
    typedef struct
    {
        uint64_t id;
        unsigned count;
        uint64_t last_end;
        hb_voice_embedding samples[HB_VOICE_TEMPLATES];
    } hb_voice_person;
    // Separate RAM schema. Stable IDs must be supplied by the household owner.
    typedef struct
    {
        uint32_t schema;
        char model[sizeof(HB_VOICE_MODEL)];
        hb_voice_person people[HB_VOICE_PEOPLE];
    } hb_voice_database;
    typedef struct
    {
        bool calibrated;
        float threshold, margin, agreement, duplicate;
        unsigned confirmations;
    } hb_voice_policy;
    typedef struct
    {
        uint32_t epoch;
        uint64_t end_ms, person_id;
        unsigned hits;
    } hb_voice_tracker;
    typedef enum
    {
        HB_VAD_IDLE,
        HB_VAD_ACTIVITY,
        HB_VAD_READY,
        HB_VAD_REJECTED
    } hb_voice_gate_state;
    typedef struct
    {
        hb_voice_gate_state state;
        uint32_t samples, active_samples, pause_samples;
        uint64_t end_ms;
    } hb_voice_gate;
    // Preliminary energy/variation gate, NOT a validated speech or overlap detector.
    hb_voice_gate_state hb_voice_gate_feed(hb_voice_gate *gate, const int16_t *pcm, size_t count,
                                           uint32_t rate, uint64_t end_ms);
    void hb_voice_gate_reset(hb_voice_gate *gate);
    bool hb_voice_normalize(hb_voice_embedding *embedding);
    // Inputs must be validated normalized embeddings (same model namespace).
    float hb_voice_similarity(const hb_voice_embedding *, const hb_voice_embedding *);
    bool hb_voice_dequantize(const int8_t *input, size_t size, int exponent,
                             hb_voice_embedding *out);
    bool hb_voice_quantize(float *features, size_t count, int exponent, int16_t *out);
    bool hb_voice_pcm_frame(const int16_t *pcm, size_t count, unsigned frame, float out[400]);
    void hb_voice_database_init(hb_voice_database *db);
    bool hb_voice_database_valid(const hb_voice_database *db);
    bool hb_voice_policy_valid(const hb_voice_policy *policy);
    bool hb_voice_person_add(hb_voice_person *person, uint64_t id, const hb_voice_embedding *e,
                             uint64_t end_ms, const hb_voice_policy *policy);
    bool hb_voice_enroll(hb_voice_database *db, uint64_t id, const hb_voice_embedding *e,
                         uint64_t end_ms, const hb_voice_policy *policy);
    void hb_voice_delete(hb_voice_database *db, uint64_t id);
    hb_voice_match hb_voice_identify(const hb_voice_database *db, const hb_voice_embedding *e,
                                     const hb_voice_policy *policy);
    bool hb_voice_confirm(hb_voice_tracker *tracker, hb_voice_match match, uint32_t epoch,
                          uint64_t end_ms, uint64_t now_ms, const hb_voice_policy *policy);
#ifdef __cplusplus
}
#endif
