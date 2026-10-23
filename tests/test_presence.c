// SPDX-License-Identifier: Apache-2.0
#include "hb_presence.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "line %d: %s\n", __LINE__, #x);                                        \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static hb_presence_value view(hb_presence_tracker *t, uint64_t now)
{
    return hb_presence_read(t, true, true, now).value;
}
static void sample(hb_presence_tracker *t, unsigned seq, uint64_t frame, bool detected)
{
    CHECK(hb_presence_observe(t, seq, frame, frame + 800, detected, frame + 800));
}
int main(void)
{
    hb_presence_tracker t;
    hb_presence_reset(&t);
    CHECK(view(&t, 1000) == HB_PRESENCE_UNKNOWN);
    CHECK(hb_presence_read(&t, false, true, 1000).value == HB_PRESENCE_OFF);
    sample(&t, 1, 1000, true);
    CHECK(view(&t, 1800) == HB_PRESENCE_UNKNOWN);
    // Re-reading the same frame must never confirm a second hit.
    CHECK(!hb_presence_observe(&t, 1, 1000, 1800, true, 1801));
    CHECK(t.hits == 1);
    sample(&t, 2, 2000, true);
    CHECK(view(&t, 2800) == HB_PRESENCE_PRESENT);
    sample(&t, 3, 3000, false);
    CHECK(view(&t, 3800) == HB_PRESENCE_PRESENT);
    sample(&t, 4, 4000, true);
    CHECK(view(&t, 4800) == HB_PRESENCE_PRESENT);
    CHECK(t.misses == 0);
    // Four negative results alone are insufficient: require five seconds of evidence.
    for (unsigned i = 0; i < 4; i++)
        sample(&t, 5 + i, 5000 + i * 1000, false);
    CHECK(view(&t, 8800) == HB_PRESENCE_PRESENT);
    sample(&t, 9, 10000, false);
    CHECK(view(&t, 10800) == HB_PRESENCE_ABSENT);
    sample(&t, 10, 11000, true);
    CHECK(view(&t, 11800) == HB_PRESENCE_ABSENT);
    sample(&t, 11, 12000, true);
    CHECK(view(&t, 12800) == HB_PRESENCE_PRESENT);
    CHECK(view(&t, 15800) == HB_PRESENCE_PRESENT);
    CHECK(view(&t, 15801) == HB_PRESENCE_UNKNOWN);
    CHECK(hb_presence_read(&t, true, false, 12800).value == HB_PRESENCE_UNKNOWN);
    CHECK(hb_presence_read(&t, false, true, 12800).value == HB_PRESENCE_OFF);
    // Data gaps must clear old streaks; time passing never proves absence.
    sample(&t, 12, 17000, false);
    CHECK(view(&t, 17800) == HB_PRESENCE_UNKNOWN);
    sample(&t, 13, 22000, false);
    CHECK(view(&t, 22800) == HB_PRESENCE_UNKNOWN && t.misses == 1);
    // Reject malformed/future/old/duplicate/out-of-order samples without modifying evidence.
    CHECK(!hb_presence_observe(&t, 14, 23000, 22999, true, 23000));
    CHECK(!hb_presence_observe(&t, 14, 23000, 23800, true, 23799));
    CHECK(!hb_presence_observe(&t, 14, 23000, 23800, true, 25600));
    CHECK(!hb_presence_observe(&t, 14, 22000, 23800, true, 23800));
    CHECK(!hb_presence_observe(&t, 13, 23000, 23800, true, 23800));
    CHECK(t.misses == 1 && t.hits == 0);
    // Exact gap and sequence wrap do not lose distinct chronological evidence.
    hb_presence_reset(&t);
    sample(&t, UINT32_MAX, 1000, true);
    sample(&t, 0, 4000, true);
    CHECK(view(&t, 4800) == HB_PRESENCE_PRESENT);
    hb_presence_reset(&t);
    sample(&t, 1, 1000, true);
    sample(&t, 2, 4001, true);
    CHECK(view(&t, 4801) == HB_PRESENCE_UNKNOWN && t.hits == 1);
    // A missing-data episode must also invalidate an earlier ABSENT state.
    hb_presence_reset(&t);
    sample(&t, 1, 1000, false);
    sample(&t, 2, 3000, false);
    sample(&t, 3, 5000, false);
    sample(&t, 4, 6000, false);
    CHECK(view(&t, 6800) == HB_PRESENCE_ABSENT);
    CHECK(view(&t, 9801) == HB_PRESENCE_UNKNOWN);
    hb_presence_reset(&t);
    CHECK(view(&t, 10000) == HB_PRESENCE_UNKNOWN);
    CHECK(!hb_presence_observe(NULL, 1, 1000, 1800, true, 1800));
    CHECK(hb_presence_read(NULL, true, true, 1800).value == HB_PRESENCE_UNKNOWN);
    puts("presence: distinct evidence, hysteresis, dropout, reset and chronology OK");
    return 0;
}
