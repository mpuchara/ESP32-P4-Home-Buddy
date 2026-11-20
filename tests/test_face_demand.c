// SPDX-License-Identifier: Apache-2.0
#include "hb_face_demand.h"
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
int main(void)
{
    hb_face_demand d = {0};
    CHECK(!hb_face_demand_active(&d, 100));
    CHECK(hb_face_demand_monitor(&d, true, 100));
    CHECK(hb_face_demand_remaining(&d, 100) == 600000);
    CHECK(hb_face_demand_active(&d, 600099));
    CHECK(hb_face_demand_remaining(&d, 600099) == 1);
    CHECK(!hb_face_demand_active(&d, 600100));
    hb_face_demand_expire(&d, 600100);
    CHECK(d.monitor_until_ms == 0);
    CHECK(hb_face_demand_period(&d) == 1500);
    // Screen/monitor ownership: closing the page cannot stop a live lease.
    CHECK(hb_face_demand_monitor(&d, true, 1000));
    d.diagnostic = true;
    CHECK(hb_face_demand_period(&d) == 500);
    d.diagnostic = false;
    CHECK(hb_face_demand_active(&d, 2000));
    // Conversely, monitor expiry/stop must not cancel the page's own request.
    d.diagnostic = true;
    hb_face_demand_expire(&d, 601000);
    CHECK(hb_face_demand_active(&d, 601000));
    CHECK(hb_face_demand_remaining(&d, 601000) == 0);
    CHECK(hb_face_demand_monitor(&d, true, 700000));
    CHECK(hb_face_demand_monitor(&d, false, 700001));
    CHECK(hb_face_demand_active(&d, 700001));
    d.diagnostic = false;
    CHECK(!hb_face_demand_active(&d, 700001));
    // Only a new explicit start renews the lease; reads and page changes do not.
    CHECK(hb_face_demand_monitor(&d, true, 800000));
    uint64_t deadline = d.monitor_until_ms;
    for (unsigned i = 0; i < 100; i++)
    {
        CHECK(hb_face_demand_active(&d, 800000 + i));
        hb_face_demand_expire(&d, 800000 + i);
        CHECK(d.monitor_until_ms == deadline);
    }
    CHECK(hb_face_demand_monitor(&d, true, 900000));
    CHECK(d.monitor_until_ms == 1500000);
    CHECK(!hb_face_demand_monitor(&d, true, UINT64_MAX));
    CHECK(!hb_face_demand_active(&d, UINT64_MAX));
    CHECK(hb_face_demand_monitor(&d, true, 100));
    CHECK(hb_face_demand_remaining(&d, 99) == 0); // clock anomaly fails closed
    hb_face_demand_expire(&d, 99);
    CHECK(d.monitor_until_ms == 0);
    CHECK(!hb_face_demand_monitor(NULL, true, 1));
    CHECK(!hb_face_demand_active(NULL, 1));
    CHECK(hb_face_demand_remaining(NULL, 1) == 0);
    puts("face demand: ownership, hard lease, no implicit renewal and overflow OK");
    return 0;
}
