// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stdint.h>
// Appearance processing for display/model input only. Never modifies leased linear pixels used by
// the light meter.
bool hb_camera_make_preview(const uint16_t *source, unsigned width, unsigned height,
                            uint16_t *target, unsigned out_width, unsigned out_height);
