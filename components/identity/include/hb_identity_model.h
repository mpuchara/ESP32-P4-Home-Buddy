// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define HB_MBF_BYTES 3522832U
#define HB_MBF_OFFSET 112U
#define HB_MBF_PACKAGE_BYTES (HB_MBF_BYTES + HB_MBF_OFFSET)
#ifdef __cplusplus
extern "C"
{
#endif
    bool hb_identity_model_header_valid(const uint8_t *, size_t, size_t);
#ifdef __cplusplus
}
#endif
