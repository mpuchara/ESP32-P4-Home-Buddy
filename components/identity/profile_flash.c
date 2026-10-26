// SPDX-License-Identifier: Apache-2.0
#include "esp_flash_encrypt.h"
#include "esp_partition.h"
#include "esp_secure_boot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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
    bool ok = esp_partition_write(p, o, b, n) == ESP_OK;
    vTaskDelay(1);
    return ok;
}
static bool erase_flash(void *p, size_t o, size_t n)
{
    if (!n || o % 4096 || n % 4096)
        return false;
    // Also used by legacy migration/deletion scrub. Bound those erases too.
    for (size_t done = 0; done < n; done += 4096)
    {
        if (esp_partition_erase_range(p, o + done, 4096) != ESP_OK)
            return false;
        vTaskDelay(1);
    }
    return true;
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
    if (!CONFIG_HB_IDENTITY_PERSISTENCE || !p || p->size != HB_PROFILE_PARTITION)
        return false;
    hb_profile_storage_mode mode = HB_PROFILE_STORAGE_HARDWARE_PROTECTED;
#if CONFIG_HB_IDENTITY_LOCAL_UNENCRYPTED
    // Refuse secured devices: never silently downgrade the protected pathway.
    if (esp_flash_encryption_enabled() || esp_secure_boot_enabled() || p->encrypted)
        return false;
    mode = HB_PROFILE_STORAGE_LOCAL_UNENCRYPTED;
#else
    if (!p->encrypted || !esp_flash_encryption_enabled() || !esp_secure_boot_enabled())
        return false;
#endif
    *io = (hb_profile_io){(void *)p, p->size, mode, read_flash, write_flash, erase_flash, digest};
    return true;
}
