// SPDX-License-Identifier: Apache-2.0
#include "hb_audio.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    int16_t raw[] = {-32768, 1234, 32767, -1234, 0, 300};
    int16_t mono[4] = {1, 2, 3, 99};
    hb_audio_status s = {0};
    assert(hb_audio_pcm(raw, 6, 16000, mono, 3, &s));
    assert(mono[0] == -32768 && mono[1] == 32767 && mono[2] == 0 && mono[3] == 99);
    assert(s.peak == 32768 && s.clipped == 2 && s.rms > 26000);
    assert(!hb_audio_pcm(raw, 5, 16000, mono, 3, &s));
    assert(!hb_audio_pcm(raw, 6, 48000, mono, 3, &s));
    assert(!hb_audio_pcm(raw, 6, 16000, mono, 2, &s));
    assert(!hb_audio_pcm(NULL, 6, 16000, mono, 3, &s));
    assert(!hb_audio_pcm(raw, 642, 16000, mono, 400, &s));
    int16_t dc[] = {1000, 0, 1000, 0};
    assert(hb_audio_pcm(dc, 4, 16000, mono, 3, &s));
    assert(s.rms == 0 && s.peak == 1000);
    puts("Bounded PCM16, stereo-left routing, clipping, DC and format validation passed");
    return 0;
}
