// SPDX-License-Identifier: Apache-2.0
#include "hb_camera_health.h"
void hb_camera_health_init(hb_camera_health *h)
{
    *h = (hb_camera_health){.retry_seconds = 5};
}
void hb_camera_health_start(hb_camera_health *h, uint64_t now, uint32_t sequence)
{
    h->started_ms = now;
    h->progress_ms = now;
    h->sequence = sequence;
}
bool hb_camera_health_timeout(hb_camera_health *h, uint64_t now, uint32_t sequence)
{
    if (now < h->progress_ms || now < h->started_ms)
        return true;
    if (sequence != h->sequence)
    {
        h->sequence = sequence;
        h->progress_ms = now;
        if (now - h->started_ms >= 30000)
            h->retry_seconds = 5;
    }
    return now - h->progress_ms > 3000;
}
uint32_t hb_camera_health_retry(hb_camera_health *h)
{
    uint32_t delay = h->retry_seconds;
    h->retry_seconds = delay >= 150 ? 300 : delay * 2;
    return delay;
}
