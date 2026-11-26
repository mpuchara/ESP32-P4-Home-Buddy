// SPDX-License-Identifier: Apache-2.0
#include "hb_voice.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int16_t pcm[HB_VOICE_SAMPLES];
static bool cancel(void *ctx)
{
    unsigned *calls = ctx;
    return ++*calls >= 5;
}
int main(void)
{
    float fraction = -1;
    assert(hb_voice_vad_window(pcm, HB_VOICE_SAMPLES, NULL, NULL, &fraction) == ESP_OK);
    assert(fraction == 0);
    assert(hb_voice_vad_window(pcm, 320, NULL, NULL, &fraction) == ESP_ERR_INVALID_ARG);
    unsigned calls = 0;
    assert(hb_voice_vad_window(pcm, HB_VOICE_SAMPLES, cancel, &calls, &fraction) ==
           ESP_ERR_INVALID_STATE);
    assert(fraction == 0);
    for (unsigned i = 0; i < HB_VOICE_SAMPLES; ++i)
        pcm[i] = (int16_t)(9000 * sin(6.283185307 * 440 * i / 16000));
    assert(hb_voice_vad_window(pcm, HB_VOICE_SAMPLES, NULL, NULL, &fraction) == ESP_OK);
    assert(fraction >= 0 && fraction <= 1);
    // Statistical VAD alone can classify a steady tone as voice; the screen must veto it.
    float support;
    assert(hb_voice_screen_window(pcm, HB_VOICE_SAMPLES, &support) == HB_VOICE_SCREEN_STATIONARY);
    printf("Pinned statistical VAD: silence rejected; tone vote %.3f; acoustic veto passed\n",
           (double)fraction);
    memset(pcm, 0, sizeof(pcm));
    assert(hb_voice_vad_window(pcm, HB_VOICE_SAMPLES, NULL, NULL, &fraction) == ESP_OK);
    assert(fraction == 0);
}
