// SPDX-License-Identifier: Apache-2.0
#include "../components/audio/audio_service.c"
#include "audio_platform.h"
#include <stdio.h>
static void consume(const int16_t *pcm, size_t n, uint32_t rate, uint64_t end, bool gap, void *ctx)
{
    (void)gap;
    (void)ctx;
    assert(pcm && n == 320 && rate == 16000 && end);
    ++samples_seen;
    hb_audio_stop();
}
static void exercise(int fail)
{
    failure = fail;
    reads = writes = samples_seen = 0;
    clock_us = 0;
    assert(hb_audio_start_capture(1, consume, NULL) == ESP_OK);
    assert(hb_audio_start() == ESP_ERR_INVALID_STATE);
    if (!setjmp(idle_jump))
        entry(NULL);
    hb_audio_status result;
    hb_audio_get(&result);
    assert(!hb_audio_busy() && !result.active && resources == 0);
    if (fail)
        assert(result.errors == 1 && samples_seen == 0);
    else
        assert(result.frames == 1 && samples_seen == 1 && result.dropped_samples == 4);
    if (fail == 1 || fail == 2)
        assert(writes == 0);
}
int main(void)
{
    assert(hb_audio_start_capture(2, NULL, NULL) == ESP_ERR_INVALID_ARG);
    for (int i = 0; i < 30; ++i)
        exercise(0);
    for (int fail = 1; fail <= 6; ++fail)
        exercise(fail);
    assert(hb_audio_start() == ESP_OK);
    hb_audio_stop();
    // Stop before worker startup must still retire the accepted Start.
    // The worker must perform cleanup rather than sleep forever with busy=true.
    if (!setjmp(idle_jump))
        entry(NULL);
    assert(!hb_audio_busy());
    puts("Real audio owner: repeated Start/Stop, missing codec, allocation/read/format failures "
         "and bounded cleanup");
}
