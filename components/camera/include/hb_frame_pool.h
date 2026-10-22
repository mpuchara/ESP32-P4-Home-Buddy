// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stdint.h>
#define HB_CAMERA_SLOTS 3
typedef struct
{
    uint32_t generation, references, leases, issued;
    bool writing;
} hb_frame_slot;
typedef struct
{
    hb_frame_slot slot[HB_CAMERA_SLOTS];
    int latest;
    uint32_t dropped;
} hb_frame_pool;
typedef struct
{
    int slot;
    uint32_t generation, token;
} hb_frame_lease;
// Caller serializes these short operations. Never free storage while leases exist.
void hb_frame_pool_init(hb_frame_pool *pool);
int hb_frame_reserve(hb_frame_pool *pool);
bool hb_frame_publish(hb_frame_pool *pool, int slot);
bool hb_frame_acquire(hb_frame_pool *pool, hb_frame_lease *lease);
bool hb_frame_release(hb_frame_pool *pool, hb_frame_lease lease);
bool hb_frame_abort(hb_frame_pool *pool, int slot);
