// SPDX-License-Identifier: Apache-2.0
#include "hb_identity_future.h"
#include "hb_voice_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static hb_voice_database database;
static float features[24000];
static int16_t quantized[24001], pcm[HB_VOICE_SAMPLES + 100];
static hb_voice_embedding vector(float x, float y)
{
    hb_voice_embedding e = {{0}};
    e.v[0] = x;
    e.v[1] = y;
    assert(hb_voice_normalize(&e));
    return e;
}
static void enroll(uint64_t id)
{
    hb_voice_person *p = &database.people[id - 1];
    p->id = id;
    p->count = 3;
    p->samples[0] = vector(1, 0);
    p->samples[1] = vector(1, 0.1f);
    p->samples[2] = vector(1, -0.1f);
}
int main(void)
{
    hb_voice_database_init(&database);
    hb_voice_embedding zero = {{0}}, invalid = vector(1, 0);
    assert(!hb_voice_normalize(&zero));
    invalid.v[1] = NAN;
    assert(!hb_voice_normalize(&invalid));
    int8_t raw[512] = {0};
    assert(!hb_voice_dequantize(raw, 512, -7, &invalid));
    raw[0] = 100;
    raw[1] = -100;
    assert(hb_voice_dequantize(raw, 512, -7, &invalid));
    assert(fabsf(invalid.v[0] - 0.70710678f) < 1e-6f && invalid.v[1] < 0);
    assert(!hb_voice_dequantize(raw, 511, -7, &invalid));
    quantized[24000] = 1234;
    for (unsigned t = 0; t < 300; ++t)
        for (unsigned d = 0; d < 80; ++d)
            features[t * 80 + d] = 3;
    assert(hb_voice_quantize(features, 24000, -8, quantized));
    for (unsigned i = 0; i < 24000; ++i)
        assert(quantized[i] == 0);
    assert(quantized[24000] == 1234);
    for (unsigned t = 0; t < 300; ++t)
        features[t * 80] = (t % 2 ? 0.5f : -0.5f);
    assert(hb_voice_quantize(features, 24000, 0, quantized));
    assert(quantized[0] == 0 && quantized[80] == 0);
    for (unsigned t = 0; t < 300; ++t)
        features[t * 80] = (t % 2 ? 1.5f : -1.5f);
    assert(hb_voice_quantize(features, 24000, 0, quantized));
    assert(quantized[0] == -2 && quantized[80] == 2);
    features[5] = NAN;
    assert(!hb_voice_quantize(features, 24000, 0, quantized));
    assert(!hb_voice_quantize(features, 23999, 0, quantized));
    features[5] = 0;
    for (unsigned t = 0; t < 300; ++t)
        for (unsigned d = 0; d < 80; ++d)
            features[t * 80 + d] = t % 2 ? 1000000.0f : -1000000.0f;
    assert(hb_voice_quantize(features, 24000, -30, quantized));
    assert(quantized[0] == -32768 && quantized[80] == 32767);
    assert(!hb_voice_quantize(features, 24000, -31, quantized));
    assert(!hb_voice_quantize(features, 24000, 17, quantized));
    features[1] = INFINITY;
    assert(!hb_voice_quantize(features, 24000, -8, quantized));
    for (unsigned i = 0; i < HB_VOICE_SAMPLES + 100; ++i)
        pcm[i] = (int16_t)(i % 30000);
    float frame[401];
    frame[400] = 99;
    assert(hb_voice_pcm_frame(pcm, HB_VOICE_SAMPLES, 299, frame));
    assert(frame[399] == pcm[48239] / 32768.0f && frame[400] == 99);
    assert(hb_voice_pcm_frame(pcm, HB_VOICE_SAMPLES + 100, 0, frame));
    assert(frame[0] == pcm[50] / 32768.0f);
    assert(hb_voice_pcm_frame(pcm, 10, 0, frame));
    assert(frame[9] != 0 && frame[10] == 0);
    assert(!hb_voice_pcm_frame(pcm, HB_VOICE_SAMPLES, 300, frame));
    hb_voice_gate g = {0};
    int16_t signal[320];
    for (unsigned i = 0; i < 320; ++i)
        signal[i] = (int16_t)((i * 971) % 6000 - 3000);
    for (unsigned i = 0; i < 150; ++i)
        assert(hb_voice_gate_feed(&g, signal, 320, 16000, 20 * (i + 1)) != HB_VAD_REJECTED);
    assert(hb_voice_gate_feed(&g, signal, 240, 16000, 3015) == HB_VAD_READY);
    hb_voice_gate_reset(&g);
    assert(hb_voice_gate_feed(&g, signal, 320, 48000, 20) == HB_VAD_REJECTED);
    hb_voice_gate_reset(&g);
    assert(hb_voice_gate_feed(&g, signal, 320, 16000, 20) == HB_VAD_IDLE);
    assert(hb_voice_gate_feed(&g, signal, 320, 16000, 80) == HB_VAD_REJECTED);
    memset(signal, 0, sizeof(signal));
    hb_voice_gate_reset(&g);
    for (unsigned i = 0; i < 25; ++i)
        hb_voice_gate_feed(&g, signal, 320, 16000, 20 * (i + 1));
    assert(g.state == HB_VAD_REJECTED);
    hb_voice_gate_reset(&g);
    for (unsigned i = 0; i < 320; ++i)
        signal[i] = 32767;
    assert(hb_voice_gate_feed(&g, signal, 320, 16000, 20) == HB_VAD_REJECTED);
    hb_voice_policy policy = {true, 0.85f, 0.1f, 0.7f, 0.9999f, 2};
    hb_voice_embedding e = vector(1, 0);
    enroll(1);
    hb_voice_match match = hb_voice_identify(&database, &e, &policy);
    assert(match.state == HB_VOICE_KNOWN && match.person_id == 1);
    hb_voice_embedding unknown = vector(0, 1);
    assert(hb_voice_identify(&database, &unknown, &policy).state == HB_VOICE_UNKNOWN);
    database.schema = 2;
    assert(hb_voice_identify(&database, &e, &policy).state == HB_VOICE_UNAVAILABLE);
    database.schema = 1;
    enroll(2);
    assert(hb_voice_identify(&database, &e, &policy).state == HB_VOICE_AMBIGUOUS);
    hb_voice_delete(&database, 2);
    assert(database.people[1].count == 0 && database.people[1].samples[0].v[0] == 0);
    assert(!hb_voice_enroll(&database, 1, &e, 10000, &policy)); // duplicate window/vector
    hb_voice_embedding distinct = vector(1, 0.2f);
    assert(hb_voice_enroll(&database, 1, &distinct, 10000, &policy));
    assert(!hb_voice_enroll(&database, 1, &distinct, 10000, &policy));
    policy.calibrated = false;
    assert(hb_voice_identify(&database, &e, &policy).state == HB_VOICE_UNAVAILABLE);
    policy.calibrated = true;
    hb_voice_tracker tracker = {0};
    assert(!hb_voice_confirm(&tracker, match, 1, 4000, 4001, &policy));
    assert(hb_voice_confirm(&tracker, match, 1, 7100, 7101, &policy));
    assert(!hb_voice_confirm(&tracker, match, 1, 7100, 7101, &policy));
    assert(!hb_voice_confirm(&tracker, match, 1, 7100, 13000, &policy));
    hb_identity_fusion_input in = {0};
    in.session = 1;
    in.face_session = in.voice_session = 1;
    in.face.state = HB_ID_KNOWN;
    in.face.person_id = 1;
    in.voice.state = HB_ID_KNOWN;
    in.voice.person_id = 2;
    in.face_confirmed = in.voice_confirmed = true;
    in.face_frame_ms = in.voice_window_end_ms = 9000;
    assert(hb_identity_fuse(&in, 1, 9001, false).source == HB_ID_SOURCE_FACE);
    assert(hb_identity_fuse(&in, 1, 9001, true).status == HB_FUSION_CONFLICT);
    in.voice.person_id = 1;
    assert(hb_identity_fuse(&in, 1, 9001, true).source == HB_ID_SOURCE_BOTH);
    in.voice_session = 2;
    assert(hb_identity_fuse(&in, 1, 9001, true).source == HB_ID_SOURCE_FACE);
    in.voice_session = 1;
    in.face_session = 2;
    assert(hb_identity_fuse(&in, 1, 9001, true).source == HB_ID_SOURCE_VOICE);
    in.face_session = 1;
    in.face_frame_ms = 1;
    assert(hb_identity_fuse(&in, 1, 9001, true).source == HB_ID_SOURCE_VOICE);
    assert(hb_identity_fuse(&in, 2, 9001, true).status == HB_FUSION_UNKNOWN);
    assert(hb_identity_fuse(&in, 1, 20000, true).status == HB_FUSION_UNKNOWN);
    puts("Voice numerical, frame-boundary, activity, enrollment, open-set, confirmation and fusion "
         "safety passed");
    return 0;
}
