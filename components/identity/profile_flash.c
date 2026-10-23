// SPDX-License-Identifier: Apache-2.0
#include "esp_flash_encrypt.h"
#include "esp_partition.h"
#include "esp_secure_boot.h"
#include "hb_profile_flash.h"
#include "mbedtls/sha256.h"
#include "sdkconfig.h"
#include <string.h>
#ifndef CONFIG_HB_IDENTITY_PERSISTENCE
#define CONFIG_HB_IDENTITY_PERSISTENCE 0
#endif
static bool read_flash(void *p, size_t o, void *b, size_t n)
{
    return esp_partition_read(p, o, b, n) == ESP_OK;
}
static bool write_flash(void *p, size_t o, const void *b, size_t n)
{
    return esp_partition_write(p, o, b, n) == ESP_OK;
}
static bool erase_flash(void *p, size_t o, size_t n)
{
    return esp_partition_erase_range(p, o, n) == ESP_OK;
}
static bool digest(const void *data, size_t n, uint8_t out[32])
{
    return mbedtls_sha256(data, n, out, 0) == 0;
}
bool hb_profile_flash_open(hb_profile_io *io)
{
    memset(io, 0, sizeof(*io));
    const esp_partition_t *p = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x41, "profiles");
    // Opening NEVER generates keys, burns eFuses, formats storage or changes security.
    // Both runtime hardware checks are mandatory even if the firmware opt-in is set.
    if (!CONFIG_HB_IDENTITY_PERSISTENCE || !p || !p->encrypted || !esp_flash_encryption_enabled() ||
        !esp_secure_boot_enabled())
        return false;
    *io = (hb_profile_io){(void *)p, p->size, true, read_flash, write_flash, erase_flash, digest};
    return true;
}
