// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
    void hb_compute_start(void); // Main task, before workers/UI.
    bool hb_compute_enter(uint32_t timeout_ms);
    void hb_compute_leave(void);
#ifdef __cplusplus
}
#endif
