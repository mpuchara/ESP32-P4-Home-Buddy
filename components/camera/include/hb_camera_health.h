// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct
{
    uint64_t started_ms, progress_ms;
    uint32_t sequence, retry_seconds;
} hb_camera_health;
void hb_camera_health_init(hb_camera_health *health);
void hb_camera_health_start(hb_camera_health *health, uint64_t now_ms, uint32_t sequence);
// Called by the owner, not ISR. Sequence equality handles frozen/retained frames.
bool hb_camera_health_timeout(hb_camera_health *health, uint64_t now_ms, uint32_t sequence);
uint32_t hb_camera_health_retry(hb_camera_health *health);
