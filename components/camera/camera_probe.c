// SPDX-License-Identifier: Apache-2.0
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "hb_camera.h"

esp_err_t hb_camera_probe(uint16_t *sensor_id)
{
    if (!sensor_id)
        return ESP_ERR_INVALID_ARG;
    *sensor_id = 0;
    i2c_master_bus_handle_t bus;
    esp_err_t err = i2c_master_get_bus_handle(I2C_NUM_0, &bus);
    if (err != ESP_OK)
        return err;
    err = i2c_master_probe(bus, 0x36, 100);
    if (err != ESP_OK)
        return err;
    i2c_master_dev_handle_t device;
    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = 0x36, .scl_speed_hz = 100000};
    err = i2c_master_bus_add_device(bus, &config, &device);
    if (err != ESP_OK)
        return err;
    uint8_t id[2] = {0};
    for (unsigned i = 0; i < 2 && err == ESP_OK; ++i)
    {
        const uint8_t address[] = {0x30, (uint8_t)(0x0a + i)};
        err = i2c_master_transmit_receive(device, address, sizeof(address), &id[i], 1, 100);
    }
    i2c_master_bus_rm_device(device);
    if (err == ESP_OK)
    {
        *sensor_id = ((uint16_t)id[0] << 8) | id[1];
        ESP_LOGI("camera", "SCCB=0x36 ID=0x%04x; no configuration registers written", *sensor_id);
        if (*sensor_id != 0x5602)
            err = ESP_ERR_NOT_SUPPORTED;
    }
    return err;
}
