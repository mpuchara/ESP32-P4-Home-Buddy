// SPDX-License-Identifier: Apache-2.0
#include "hb_identity_model.h"
#include <string.h>
static uint32_t u32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
bool hb_identity_model_header_valid(const uint8_t *h, size_t n, size_t partition)
{
    static const char version[16] = "v1.0";
    static const char name[] = "human_face_feat_mbf_s8_v1.espdl";
    if (!h || n < HB_MBF_OFFSET || partition < HB_MBF_PACKAGE_BYTES || memcmp(h, "PDL3", 4) ||
        memcmp(h + 4, version, 16) || u32(h + 20) != HB_MBF_PACKAGE_BYTES || u32(h + 56) != 1 ||
        u32(h + 60) != HB_MBF_OFFSET || u32(h + 64) != 72 || u32(h + 68) != sizeof(name) - 1 ||
        memcmp(h + 72, name, sizeof(name) - 1))
        return false;
    for (size_t i = 72 + sizeof(name) - 1; i < HB_MBF_OFFSET; i++)
        if (h[i])
            return false;
    return true;
}
