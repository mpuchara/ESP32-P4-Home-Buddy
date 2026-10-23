// SPDX-License-Identifier: Apache-2.0
#include "esp_err.h"
#include "sdkconfig.h"
#if CONFIG_HB_IDENTITY_EXPERIMENTAL
#include "board_p4_pins.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
esp_err_t hb_identity_model_storage(void)
{
    static sdmmc_card_t *card;
    if (card)
        return ESP_OK;
    // Existing BSP supplies TF_VCC using LDO4. Never reset/reconfigure C6 slot 1.
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    // SDMMC_HOST_DEFAULT uses slot-scoped deinit, including on failed mounts.
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 4;
    slot.clk = BOARD_P4_SD_CLK;
    slot.cmd = BOARD_P4_SD_CMD;
    slot.d0 = BOARD_P4_SD_D0;
    slot.d1 = BOARD_P4_SD_D1;
    slot.d2 = BOARD_P4_SD_D2;
    slot.d3 = BOARD_P4_SD_D3;
    esp_vfs_fat_sdmmc_mount_config_t mount = {
        .format_if_mount_failed = false,
        .max_files = 3,
        .allocation_unit_size = 16 * 1024,
    };
    // No format, partition writes, runtime download or biometric files.
    return esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot, &mount, &card);
}
#else
esp_err_t hb_identity_model_storage(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}
#endif
