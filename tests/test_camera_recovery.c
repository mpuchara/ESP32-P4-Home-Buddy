// SPDX-License-Identifier: Apache-2.0
#include "hb_camera_health.h"
#include "hb_frame_pool.h"
#include <stdint.h>
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
static void frames(void)
{
    hb_frame_pool p;
    hb_frame_lease held, recent;
    hb_frame_pool_init(&p);
    int first = hb_frame_reserve(&p);
    CHECK(hb_frame_complete(&p, first, 100, 100));
    CHECK(hb_frame_acquire(&p, &held));
    int partial = hb_frame_reserve(&p);
    CHECK(!hb_frame_complete(&p, partial, 99, 100));
    CHECK(!hb_frame_complete(&p, partial, 100, 100)); // aborted completion cannot publish later
    CHECK(hb_frame_acquire(&p, &recent) && recent.slot == first);
    CHECK(hb_frame_release(&p, recent));
    partial = hb_frame_reserve(&p);
    CHECK(!hb_frame_complete(&p, partial, 101, 100));
    partial = hb_frame_reserve(&p);
    CHECK(!hb_frame_complete(&p, partial, 0, 0));
    CHECK(!hb_frame_complete(&p, -1, 100, 100));
    int pending = hb_frame_reserve(&p);
    hb_frame_pool_stop(&p); // DMA quiesced, but the first reader is still working
    CHECK(!hb_frame_acquire(&p, &recent));
    CHECK(!hb_frame_complete(&p, pending, 100, 100));
    hb_frame_pool_stop(&p); // idempotent, no owner ref underflow
    CHECK(p.slot[first].references == 1);
    int next = hb_frame_reserve(&p);
    CHECK(next >= 0 && next != first); // held storage cannot be selected for DMA
    CHECK(hb_frame_complete(&p, next, 100, 100));
    CHECK(hb_frame_acquire(&p, &recent) && recent.slot == next);
    CHECK(hb_frame_release(&p, held));
    CHECK(!hb_frame_release(&p, held));
    CHECK(hb_frame_release(&p, recent));
    hb_frame_pool_stop(&p);
    hb_frame_lease saturated[HB_CAMERA_SLOTS];
    for (int n = 0; n < HB_CAMERA_SLOTS; n++)
    {
        next = hb_frame_reserve(&p);
        CHECK(next >= 0 && hb_frame_complete(&p, next, 100, 100));
        CHECK(hb_frame_acquire(&p, &saturated[n]));
    }
    hb_frame_pool_stop(&p);
    CHECK(hb_frame_reserve(&p) == -1); // all old consumers retain their bounded slots
    CHECK(hb_frame_release(&p, saturated[0]));
    next = hb_frame_reserve(&p);
    CHECK(next == saturated[0].slot && hb_frame_complete(&p, next, 100, 100));
    CHECK(!hb_frame_release(&p, saturated[0])); // stale release after storage reuse
    for (int n = 1; n < HB_CAMERA_SLOTS; n++)
        CHECK(hb_frame_release(&p, saturated[n]));
    hb_frame_pool_stop(&p);
    for (unsigned n = 0; n < 10000; n++)
    {
        next = hb_frame_reserve(&p);
        CHECK(next >= 0);
        CHECK(hb_frame_complete(&p, next, 100, 100));
        CHECK(hb_frame_acquire(&p, &recent));
        hb_frame_pool_stop(&p);
        CHECK(!hb_frame_acquire(&p, &held));
        CHECK(hb_frame_release(&p, recent));
        for (int slot = 0; slot < HB_CAMERA_SLOTS; slot++)
            CHECK(!p.slot[slot].references && !p.slot[slot].writing);
    }
}
static void health(void)
{
    hb_camera_health h;
    hb_camera_health_init(&h);
    hb_camera_health_start(&h, 100, 12);
    CHECK(!hb_camera_health_timeout(&h, 3100, 12));
    CHECK(hb_camera_health_timeout(&h, 3101, 12)); // retained/frozen frame is not progress
    const uint32_t waits[] = {5, 10, 20, 40, 80, 160, 300, 300};
    for (unsigned n = 0; n < sizeof(waits) / sizeof(waits[0]); n++)
    {
        CHECK(hb_camera_health_retry(&h) == waits[n]);
        hb_camera_health_start(&h, 10000 + n * 10000, 12); // failed attempts don't reset backoff
    }
    hb_camera_health_start(&h, 100000, UINT32_MAX);
    CHECK(!hb_camera_health_timeout(&h, 100100, 0)); // counter wrap is real progress
    CHECK(!hb_camera_health_timeout(&h, 103100, 0));
    CHECK(hb_camera_health_timeout(&h, 103101, 0));
    hb_camera_health_start(&h, 200000, 0);
    for (uint32_t n = 1; n <= 299; n++)
        CHECK(!hb_camera_health_timeout(&h, 200000 + n * 100, n));
    CHECK(h.retry_seconds == 300);
    CHECK(!hb_camera_health_timeout(&h, 230000, 300));
    CHECK(hb_camera_health_retry(&h) ==
          5); // thirty seconds of continued frames restores fast retry
    CHECK(hb_camera_health_timeout(&h, 229999, 300)); // clock regression is not accepted as healthy
}
int main(void)
{
    frames();
    health();
    puts("camera recovery: invalid completion, stale publication, held leases, timeout and bounded "
         "retries OK");
    return 0;
}
