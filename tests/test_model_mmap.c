// SPDX-License-Identifier: Apache-2.0
#include "esp_partition.h"
#include "hb_identity_model.h"
#include "mbedtls/sha256.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
esp_err_t hb_identity_model_storage(void);
static unsigned char image[4 * 1024 * 1024];
static esp_partition_t part = {sizeof(image)};
static int missing, map_error, hash_error, active;
static unsigned pages, maps, unmaps, delays, frees;
static size_t hashed;
static const unsigned char expected[32] = {
    0xf8, 0xaa, 0x1d, 0x21, 0xc4, 0x78, 0x14, 0xbf, 0x32, 0x4b, 0x78, 0xec, 0x3a, 0x05, 0x0f, 0x99,
    0x3d, 0xfa, 0x38, 0xb1, 0xf6, 0xa5, 0x82, 0xb6, 0xaf, 0xb5, 0xce, 0x02, 0x64, 0xda, 0xf2, 0x94};
const esp_partition_t *esp_partition_find_first(int type, int subtype, const char *name)
{
    assert(type == ESP_PARTITION_TYPE_DATA && subtype == ESP_PARTITION_SUBTYPE_ANY);
    assert(!strcmp(name, "human_face_feat"));
    return missing ? NULL : &part;
}
unsigned spi_flash_mmap_get_free_pages(int type)
{
    assert(type == 0);
    return pages;
}
esp_err_t esp_partition_mmap(const esp_partition_t *p, size_t offset, size_t size, int type,
                             const void **out, esp_partition_mmap_handle_t *handle)
{
    assert(p == &part && !offset && size == part.size && type == 0 && !active);
    ++maps;
    if (map_error)
        return ESP_FAIL;
    active = 1;
    *out = image;
    *handle = 123;
    return ESP_OK;
}
void esp_partition_munmap(esp_partition_mmap_handle_t handle)
{
    assert(handle == 123 && active);
    active = 0;
    ++unmaps;
}
void vTaskDelay(unsigned ticks)
{
    assert(active && ticks == 1);
    ++delays;
}
// Hash double checks full mapped-byte coverage/lifetime and injects failures.
// Actual SHA-256/package identity is covered by the pinned artifact tooling.
void mbedtls_sha256_init(mbedtls_sha256_context *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
}
int mbedtls_sha256_starts(mbedtls_sha256_context *ctx, int sha224)
{
    (void)ctx;
    assert(!sha224 && active);
    return hash_error == 1 ? -1 : 0;
}
int mbedtls_sha256_update(mbedtls_sha256_context *ctx, const unsigned char *p, size_t n)
{
    assert(active && p == image + ctx->bytes && n <= 65536);
    assert(ctx->bytes + n <= HB_MBF_PACKAGE_BYTES);
    if (hash_error == 2)
        return -1;
    for (size_t i = 0; i < n; ++i)
        if (ctx->bytes + i >= HB_MBF_OFFSET && p[i])
            ctx->corrupt = 1;
    ctx->bytes += n;
    hashed = ctx->bytes;
    return 0;
}
int mbedtls_sha256_finish(mbedtls_sha256_context *ctx, unsigned char *out)
{
    assert(active && ctx->bytes == HB_MBF_PACKAGE_BYTES);
    memcpy(out, expected, 32);
    out[0] ^= (unsigned char)ctx->corrupt;
    return hash_error == 3 ? -1 : 0;
}
void mbedtls_sha256_free(mbedtls_sha256_context *ctx)
{
    (void)ctx;
    assert(active);
    ++frees;
}
static void put32(unsigned offset, unsigned value)
{
    for (unsigned i = 0; i < 4; ++i)
        image[offset + i] = (unsigned char)(value >> (i * 8));
}
static void reset(void)
{
    assert(!active);
    memset(image, 0, sizeof(image));
    memcpy(image, "PDL3", 4);
    memcpy(image + 4, "v1.0", 4);
    put32(20, HB_MBF_PACKAGE_BYTES);
    put32(56, 1);
    put32(60, HB_MBF_OFFSET);
    put32(64, 72);
    const char name[] = "human_face_feat_mbf_s8_v1.espdl";
    put32(68, sizeof(name) - 1);
    memcpy(image + 72, name, sizeof(name) - 1);
    missing = map_error = hash_error = 0;
    maps = unmaps = delays = frees = 0;
    hashed = 0;
    pages = 64;
    part.size = sizeof(image);
}
int main(void)
{
    reset();
    for (unsigned i = 0; i < 20; ++i)
    {
        assert(hb_identity_model_storage() == ESP_OK);
        assert(!active && maps == i + 1 && unmaps == maps && frees == maps);
        assert(hashed == HB_MBF_PACKAGE_BYTES);
    }
    assert(delays == 20 * ((HB_MBF_PACKAGE_BYTES + 65535) / 65536));
    reset();
    missing = 1;
    assert(hb_identity_model_storage() == ESP_ERR_NOT_FOUND && !maps);
    reset();
    part.size = HB_MBF_PACKAGE_BYTES - 1;
    assert(hb_identity_model_storage() == ESP_ERR_INVALID_SIZE && !maps);
    reset();
    pages = 63;
    assert(hb_identity_model_storage() == ESP_ERR_NO_MEM && !maps);
    reset();
    map_error = 1;
    assert(hb_identity_model_storage() == ESP_FAIL && maps == 1 && !unmaps);
    reset();
    image[0] = 0;
    assert(hb_identity_model_storage() == ESP_ERR_INVALID_VERSION && unmaps == 1 && !frees);
    for (int fail = 1; fail <= 3; ++fail)
    {
        reset();
        hash_error = fail;
        assert(hb_identity_model_storage() == ESP_ERR_INVALID_CRC);
        assert(!active && unmaps == 1 && frees == 1);
    }
    reset();
    image[HB_MBF_PACKAGE_BYTES - 1] = 1;
    assert(hb_identity_model_storage() == ESP_ERR_INVALID_CRC && !active && unmaps == 1);
    puts("model mmap: re-entry, full coverage, corruption and all failure cleanups pass");
    return 0;
}
