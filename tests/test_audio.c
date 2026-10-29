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
    int16_t block[640];
    for (unsigned i = 0; i < 640; ++i)
        block[i] = 0;
    int16_t output[320];
    assert(hb_audio_pcm(block, 640, 16000, output, 320, &s));
    assert(s.suspicious && s.dc_offset == 0 && s.right_rms == 0);
    for (unsigned i = 0; i < 320; ++i)
    {
        block[2 * i] = 32767;
        block[2 * i + 1] = (int16_t)(i * 30 - 4000);
    }
    assert(hb_audio_pcm(block, 640, 16000, output, 320, &s));
    assert(s.suspicious && s.clipped == 320 && s.dc_offset == 32767 && s.right_rms > 2000);
    for (unsigned i = 0; i < 320; ++i)
        block[2 * i] = (int16_t)(i * 31 - 5000);
    assert(hb_audio_pcm(block, 640, 16000, output, 320, &s));
    assert(!s.suspicious && s.rms > 2000);
    assert(hb_audio_pcm_slot(block, 640, 16000, 1, output, 320, &s));
    assert(output[100] == block[201] && s.selected_slot == 1 && !s.right_suspicious);
    assert(!hb_audio_pcm_slot(block, 640, 16000, 2, output, 320, &s));
    puts("Bounded PCM16, stereo-left routing, clipping, DC and format validation passed");
    return 0;
}
