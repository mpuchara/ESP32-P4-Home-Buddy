// SPDX-License-Identifier: Apache-2.0
#include "esp_err.h"
#include "sdkconfig.h"
#if CONFIG_HB_IDENTITY_EXPERIMENTAL
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hb_identity_model.h"
#include "mbedtls/sha256.h"
#include "spi_flash_mmap.h"
#include <string.h>
esp_err_t hb_identity_model_storage(void)
{
    const esp_partition_t *p = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "human_face_feat");
    if (!p)
        return ESP_ERR_NOT_FOUND;
    if (p->size < HB_MBF_PACKAGE_BYTES)
        return ESP_ERR_INVALID_SIZE;
    uint8_t header[HB_MBF_OFFSET];
    esp_err_t e = esp_partition_read(p, 0, header, sizeof(header));
    if (e)
        return e;
    if (!hb_identity_model_header_valid(header, sizeof(header), p->size))
        return ESP_ERR_INVALID_VERSION;
    // Pin the complete upstream PDL3 image, including version, index and raw model.
    static const uint8_t expected[32] = {0xf8, 0xaa, 0x1d, 0x21, 0xc4, 0x78, 0x14, 0xbf,
                                         0x32, 0x4b, 0x78, 0xec, 0x3a, 0x05, 0x0f, 0x99,
                                         0x3d, 0xfa, 0x38, 0xb1, 0xf6, 0xa5, 0x82, 0xb6,
                                         0xaf, 0xb5, 0xce, 0x02, 0x64, 0xda, 0xf2, 0x94};
    mbedtls_sha256_context hash;
    mbedtls_sha256_init(&hash);
    int rc = mbedtls_sha256_starts(&hash, 0);
    uint8_t block[1024], digest[32];
    for (size_t offset = 0; !rc && offset < HB_MBF_PACKAGE_BYTES; offset += sizeof(block))
    {
        size_t n = HB_MBF_PACKAGE_BYTES - offset;
        if (n > sizeof(block))
            n = sizeof(block);
        e = esp_partition_read(p, offset, block, n);
        if (e)
            break;
        rc = mbedtls_sha256_update(&hash, block, n);
        if (!(offset % 65536))
            vTaskDelay(1);
    }
    if (!rc && !e)
        rc = mbedtls_sha256_finish(&hash, digest);
    mbedtls_sha256_free(&hash);
    if (e)
        return e;
    if (rc || memcmp(digest, expected, 32))
        return ESP_ERR_INVALID_CRC;
    // Preflight the mmap operation used by the standard locked loader.
    if ((size_t)spi_flash_mmap_get_free_pages(SPI_FLASH_MMAP_DATA) * 65536 < p->size)
        return ESP_ERR_NO_MEM;
    const void *mapped = NULL;
    esp_partition_mmap_handle_t handle;
    e = esp_partition_mmap(p, 0, p->size, ESP_PARTITION_MMAP_DATA, &mapped, &handle);
    if (!e)
        esp_partition_munmap(handle);
    return e;
}
#else
esp_err_t hb_identity_model_storage(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}
#endif
