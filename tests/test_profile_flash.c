// SPDX-License-Identifier: Apache-2.0
#include "esp_partition.h"
#include "hb_profile_flash.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static esp_partition_t part = {HB_PROFILE_PARTITION, false};
static bool encryption, secure, missing;
static unsigned reads, writes, erases, yields;
static unsigned fail_erase;
static bool fail_write;
void vTaskDelay(unsigned ticks)
{
    assert(ticks == 1);
    ++yields;
}
bool esp_flash_encryption_enabled(void)
{
    return encryption;
}
bool esp_secure_boot_enabled(void)
{
    return secure;
}
const esp_partition_t *esp_partition_find_first(int type, int subtype, const char *name)
{
    assert(type == 1 && subtype == 0x41 && !strcmp(name, "profiles"));
    return missing ? NULL : &part;
}
int esp_partition_read(const esp_partition_t *p, size_t o, void *b, size_t n)
{
    (void)p;
    (void)o;
    memset(b, 255, n);
    ++reads;
    return ESP_OK;
}
int esp_partition_write(const esp_partition_t *p, size_t o, const void *b, size_t n)
{
    (void)p;
    (void)o;
    (void)b;
    (void)n;
    ++writes;
    return fail_write ? 1 : ESP_OK;
}
int esp_partition_erase_range(const esp_partition_t *p, size_t o, size_t n)
{
    (void)p;
    (void)o;
    (void)n;
    assert(n == 4096 && o % 4096 == 0);
    ++erases;
    return erases == fail_erase ? 1 : ESP_OK;
}
int mbedtls_sha256(const unsigned char *b, size_t n, unsigned char *out, int mode)
{
    (void)b;
    (void)n;
    assert(!mode);
    memset(out, 0, 32);
    return 0;
}
int main(void)
{
    hb_profile_io io;
    for (unsigned mask = 0; mask < 8; ++mask)
    {
        encryption = (mask & 1) != 0;
        secure = (mask & 2) != 0;
        part.encrypted = (mask & 4) != 0;
#if CONFIG_HB_IDENTITY_LOCAL_UNENCRYPTED
        bool allowed = mask == 0;
#else
        bool allowed = mask == 7;
#endif
        assert(hb_profile_flash_open(&io) == allowed);
        if (allowed)
        {
#if CONFIG_HB_IDENTITY_LOCAL_UNENCRYPTED
            assert(io.storage_mode == HB_PROFILE_STORAGE_LOCAL_UNENCRYPTED);
#else
            assert(io.storage_mode == HB_PROFILE_STORAGE_HARDWARE_PROTECTED);
#endif
            assert(io.read && io.write && io.erase && io.sha256);
        }
    }
    assert(!reads && !writes && !erases); // Opening is read/write/erase-free.
#if CONFIG_HB_IDENTITY_LOCAL_UNENCRYPTED
    encryption = secure = part.encrypted = false;
#else
    encryption = secure = part.encrypted = true;
#endif
    assert(hb_profile_flash_open(&io));
    // Real adapter yields once per sector and after a bounded payload write.
    assert(io.erase(io.context, 0, HB_PROFILE_PARTITION));
    assert(erases == HB_PROFILE_PARTITION / 4096 && yields == erases);
    uint8_t payload[16] = {0};
    assert(io.write(io.context, 0, payload, sizeof(payload)));
    assert(writes == 1 && yields == erases + 1);
    unsigned before = erases;
    assert(!io.erase(io.context, 1, 4096));
    assert(!io.erase(io.context, 0, 4095));
    assert(erases == before);
    fail_erase = erases + 2;
    assert(!io.erase(io.context, 0, 16384));
    assert(erases == before + 2); // Stop at the failed sector.
    fail_write = true;
    assert(!io.write(io.context, 0, payload, sizeof(payload)));
    missing = true;
    assert(!hb_profile_flash_open(&io));
    missing = false;
    part.size--;
    assert(!hb_profile_flash_open(&io));
    puts("Actual Flash adapter: policy/security/capacity/no-side-effect admission passed");
}
