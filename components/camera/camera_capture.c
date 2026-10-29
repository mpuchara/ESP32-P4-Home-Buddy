// SPDX-License-Identifier: Apache-2.0
#include "driver/i2c_master.h"
#include "driver/isp.h"
#include "esp_attr.h"
#include "esp_cam_ctlr.h"
#include "esp_cam_ctlr_csi.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hb_camera.h"
#include "hb_camera_health.h"
#include "hb_raw_frame.h"
#include "vendor/ov02c10_mode.h"
#include <math.h>
#include <stdatomic.h>
#include <string.h>
#ifndef HB_CAMERA_DMA_OUTPUT_BITS
#error "Local RAW8 capture requires the audited project CSI DMA output-length adapter"
#endif
#define WIDTH 1288
#define HEIGHT 728
#define FRAME_BYTES (WIDTH * HEIGHT)
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static hb_frame_pool pool;
static uint8_t *storage[HB_CAMERA_SLOTS];
static uint64_t frame_time[HB_CAMERA_SLOTS];
static uint32_t frame_seq[HB_CAMERA_SLOTS];
static hb_camera_status status;
static esp_cam_ctlr_handle_t controller;
static isp_proc_handle_t isp;
static i2c_master_dev_handle_t sensor;
static bool controller_enabled, controller_started, isp_enabled;
static bool isp_quarantined;
static atomic_uint sample_seconds = 5;
static atomic_bool started;
static atomic_bool requested_pattern;
void hb_camera_test_pattern(bool enabled)
{
    atomic_store(&requested_pattern, enabled);
}
static esp_err_t write_reg(uint16_t address, uint8_t value)
{
    uint8_t bytes[] = {(uint8_t)(address >> 8), (uint8_t)address, value};
    return i2c_master_transmit(sensor, bytes, sizeof(bytes), 100);
}
static esp_err_t read_regs(uint16_t address, uint8_t *bytes, size_t size)
{
    uint8_t reg[] = {(uint8_t)(address >> 8), (uint8_t)address};
    return i2c_master_transmit_receive(sensor, reg, 2, bytes, size, 100);
}
static bool IRAM_ATTR camera_next_buffer(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans,
                                         void *arg)
{
    (void)handle;
    (void)arg;
    portENTER_CRITICAL_ISR(&lock);
    int i = hb_frame_reserve(&pool);
    if (i >= 0)
    {
        trans->buffer = storage[i];
        trans->buflen = FRAME_BYTES;
        // The pinned CSI adapter invalidates the full output buffer on completion.
        trans->received_size = 0;
    }
    // If all slots are leased, IDF uses its own bounded discard buffer.
    portEXIT_CRITICAL_ISR(&lock);
    return false;
}
static bool IRAM_ATTR camera_frame_finished(esp_cam_ctlr_handle_t handle,
                                            esp_cam_ctlr_trans_t *trans, void *arg)
{
    (void)handle;
    (void)arg;
    portENTER_CRITICAL_ISR(&lock);
    for (int i = 0; i < HB_CAMERA_SLOTS; i++)
        if (trans->buffer == storage[i])
        {
            if (hb_frame_complete(&pool, i, trans->received_size, FRAME_BYTES))
            {
                frame_time[i] = (uint64_t)esp_timer_get_time() / 1000;
                frame_seq[i] = ++status.frames;
            }
            else
            {
                status.errors++;
            }
            break;
        }
    portEXIT_CRITICAL_ISR(&lock);
    return false;
}
bool hb_camera_acquire(hb_camera_frame *f)
{
    if (!f)
        return false;
    portENTER_CRITICAL(&lock);
    bool ok = status.running && hb_frame_acquire(&pool, &f->lease);
    if (ok)
    {
        int i = f->lease.slot;
        f->pixels = storage[i];
        f->width = WIDTH;
        f->height = HEIGHT;
        f->rotation_ccw = 90;
        f->sequence = frame_seq[i];
        f->timestamp_ms = frame_time[i];
    }
    portEXIT_CRITICAL(&lock);
    return ok;
}
void hb_camera_release(hb_camera_frame *f)
{
    if (!f || !f->pixels)
        return;
    portENTER_CRITICAL(&lock);
    hb_frame_release(&pool, f->lease);
    portEXIT_CRITICAL(&lock);
    f->pixels = NULL;
}
void hb_camera_get_status(hb_camera_status *out)
{
    if (!out)
        return;
    portENTER_CRITICAL(&lock);
    *out = status;
    out->dropped = pool.dropped;
    portEXIT_CRITICAL(&lock);
}
void hb_camera_set_sample_seconds(uint32_t seconds)
{
    if (seconds >= 1 && seconds <= 60)
        atomic_store(&sample_seconds, seconds);
}
static esp_err_t initialize(void)
{
    uint16_t id;
    esp_err_t e = hb_camera_probe(&id);
    portENTER_CRITICAL(&lock);
    status.sensor_id = id;
    portEXIT_CRITICAL(&lock);
    if (e != ESP_OK)
        return e;
    i2c_master_bus_handle_t bus;
    e = i2c_master_get_bus_handle(I2C_NUM_0, &bus);
    if (e != ESP_OK)
        return e;
    i2c_device_config_t dev = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = 0x36, .scl_speed_hz = 100000};
    e = i2c_master_bus_add_device(bus, &dev, &sensor);
    if (e != ESP_OK)
        return e;
    e = write_reg(0x0100, 0);
    if (e == ESP_OK)
        e = write_reg(0x0103, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    for (unsigned i = 0; e == ESP_OK && hb_sensor_mode[i].reg != OV02C10_REG_END; i++)
    {
        if (hb_sensor_mode[i].reg == OV02C10_REG_DELAY)
            vTaskDelay(pdMS_TO_TICKS(hb_sensor_mode[i].val));
        else
            e = write_reg(hb_sensor_mode[i].reg, hb_sensor_mode[i].val);
    }
    if (e != ESP_OK)
        return e;
    // Source driver's stream operation overrides the table's gated-clock setting
    // when CSI line-sync packets are disabled. Match that operation explicitly.
    e = write_reg(0x4800, 0x00);
    if (e != ESP_OK)
        return e;
    // Mode table advertises one 400 Mbit/s RAW10 lane and 24 MHz module oscillator.
    // DSI already owns LDO3=2.5 V. Never reacquire/reconfigure that shared rail.
    const esp_cam_ctlr_csi_config_t csi = {.ctlr_id = 0,
                                           .h_res = WIDTH,
                                           .v_res = HEIGHT,
                                           .data_lane_num = 1,
                                           .lane_bit_rate_mbps = 400,
                                           .input_data_color_type = CAM_CTLR_COLOR_RAW10,
                                           .output_data_color_type = CAM_CTLR_COLOR_RAW8,
                                           .queue_items = 1};
    e = esp_cam_new_csi_ctlr(&csi, &controller);
    if (e != ESP_OK)
        return e;
    for (int i = 0; i < HB_CAMERA_SLOTS; i++)
        if (!storage[i])
        {
            storage[i] = esp_cam_ctlr_alloc_buffer(controller, FRAME_BYTES,
                                                   MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
            if (!storage[i])
                return ESP_ERR_NO_MEM;
        }
    esp_cam_ctlr_evt_cbs_t callbacks = {.on_get_new_trans = camera_next_buffer,
                                        .on_trans_finished = camera_frame_finished};
    e = esp_cam_ctlr_register_event_callbacks(controller, &callbacks, NULL);
    if (e != ESP_OK)
        return e;
    e = esp_cam_ctlr_enable(controller);
    if (e != ESP_OK)
        return e;
    controller_enabled = true;
    esp_isp_processor_cfg_t config = {.clk_src = ISP_CLK_SRC_PLL160,
                                      .clk_hz = 160000000,
                                      .input_data_source = ISP_INPUT_DATA_SOURCE_CSI,
                                      .input_data_color_type = ISP_COLOR_RAW10,
                                      .output_data_color_type = ISP_COLOR_RAW8,
                                      .h_res = WIDTH,
                                      .v_res = HEIGHT,
                                      .bayer_order = COLOR_RAW_ELEMENT_ORDER_GBRG};
    e = esp_isp_new_processor(&config, &isp);
    if (e != ESP_OK)
    {
        // IDF5.5.1 constructor frees its object on failure without releasing every
        // global claim. No public handle exists to undo that partial construction.
        // Quarantine instead of repeatedly initializing/leaking this peripheral.
        isp_quarantined = true;
        return e;
    }
    e = esp_isp_enable(isp);
    if (e != ESP_OK)
        return e;
    isp_enabled = true;
    e = esp_cam_ctlr_start(controller);
    if (e != ESP_OK)
        return e;
    controller_started = true;
    e = write_reg(0x0100, 1);
    if (e != ESP_OK)
        return e;
    portENTER_CRITICAL(&lock);
    status.running = true;
    status.width = WIDTH;
    status.height = HEIGHT;
    portEXIT_CRITICAL(&lock);
    ESP_LOGI("camera", "CSI RAW10 1288x728 1 lane 400Mbps -> ISP RAW8 GBRG; pool=%u + discard=%u",
             (unsigned)(HB_CAMERA_SLOTS * FRAME_BYTES), (unsigned)FRAME_BYTES);
    return ESP_OK;
}
static void stop_driver(void)
{
    portENTER_CRITICAL(&lock);
    status.running = false;
    status.light.valid = false;
    status.test_pattern = false;
    status.fps = 0;
    portEXIT_CRITICAL(&lock);
    if (sensor)
        write_reg(0x0100, 0);
    if ((controller_started && esp_cam_ctlr_stop(controller) != ESP_OK) ||
        (controller_enabled && esp_cam_ctlr_disable(controller) != ESP_OK) ||
        (isp_enabled && esp_isp_disable(isp) != ESP_OK) ||
        (isp && esp_isp_del_processor(isp) != ESP_OK) ||
        (controller && esp_cam_ctlr_del(controller) != ESP_OK) ||
        (sensor && i2c_master_bus_rm_device(sensor) != ESP_OK))
    {
        // Unknown driver/DMA lifetime: retain all storage and stop retries.
        isp_quarantined = true;
        ESP_LOGE("camera", "Driver cleanup failed; retaining bounded resources until reboot");
        return;
    }
    controller = NULL;
    isp = NULL;
    sensor = NULL;
    controller_started = false;
    controller_enabled = false;
    isp_enabled = false;
    // Keep the three allocations stable across recovery: consumers may still hold leases.
    portENTER_CRITICAL(&lock);
    hb_frame_pool_stop(&pool);
    portEXIT_CRITICAL(&lock);
}
static void worker(void *arg)
{
    (void)arg;
    hb_camera_health health;
    hb_camera_health_init(&health);
    bool startup_pattern = true;
    for (;;)
    {
        esp_err_t e = initialize();
        uint64_t started = (uint64_t)esp_timer_get_time() / 1000, last_light = 0;
        uint64_t last_control = 0, changed = started;
        bool pattern = false;
        bool pattern_reported = false;
        uint32_t base_sequence;
        portENTER_CRITICAL(&lock);
        base_sequence = status.frames;
        status.error = e;
        status.retry_seconds = 0;
        portEXIT_CRITICAL(&lock);
        hb_camera_health_start(&health, started, base_sequence);
        uint32_t sequence = base_sequence;
        while (e == ESP_OK)
        {
            uint64_t now = (uint64_t)esp_timer_get_time() / 1000;
            bool request = startup_pattern || atomic_load(&requested_pattern);
            if (request != pattern)
            {
                uint8_t original;
                e = read_regs(0x4503, &original, 1);
                if (e == ESP_OK)
                    e = write_reg(0x4503, request ? (uint8_t)((original & 0x7c) | 0x80)
                                                  : (uint8_t)(original & 0x7f));
                if (e != ESP_OK)
                    break;
                pattern = request;
                pattern_reported = false;
                changed = now;
                portENTER_CRITICAL(&lock);
                status.test_pattern = pattern;
                status.light.valid = false;
                portEXIT_CRITICAL(&lock);
            }
            hb_camera_frame frame = {0};
            if (hb_camera_acquire(&frame))
            {
                sequence = frame.sequence;
                if (pattern && !pattern_reported && frame.timestamp_ms >= changed + 200)
                {
                    // Numeric synthetic-pattern diagnostics only; never log a room frame.
                    for (unsigned bar = 0; bar < 8; bar++)
                    {
                        const uint8_t *raw = frame.pixels + (HEIGHT / 2) * WIDTH +
                                             (((2 * bar + 1) * WIDTH / 16) & ~1U);
                        uint16_t p = (uint16_t)(((raw[WIDTH] >> 3) << 11) |
                                                ((((unsigned)raw[0] + raw[WIDTH + 1]) / 8) << 5) |
                                                (raw[1] >> 3));
                        ESP_LOGI("camera_pattern",
                                 "bar=%u R=%u G=%u B=%u swapR=%u swapG=%u swapB=%u", bar,
                                 (p >> 11) & 31, (p >> 5) & 63, p & 31,
                                 ((p << 8 | p >> 8) >> 11) & 31, ((p << 8 | p >> 8) >> 5) & 63,
                                 (p << 8 | p >> 8) & 31);
                    }
                    pattern_reported = true;
                    startup_pattern = false;
                }
                if (!pattern && now - last_control >= 500 && frame.timestamp_ms >= changed + 200)
                {
                    uint8_t exposure[2], gain[2], digital[3], timing[4];
                    bool meta = read_regs(0x3501, exposure, 2) == ESP_OK &&
                                read_regs(0x3508, gain, 2) == ESP_OK &&
                                read_regs(0x350a, digital, 3) == ESP_OK &&
                                read_regs(0x380c, timing, 4) == ESP_OK;
                    float exp = 0, ag = 0, dg = 0;
                    if (meta)
                    {
                        exp = ((exposure[0] << 8) | exposure[1]) * ((timing[0] << 8) | timing[1]) /
                              81.6667f;
                        ag = ((gain[0] << 8) | gain[1]) / 256.0f;
                        dg = (((digital[0] & 15) << 16) | (digital[1] << 8) | (digital[2] & 0xc0)) /
                             65536.0f;
                    }
                    hb_light_reading light =
                        hb_raw_light(frame.pixels, WIDTH, HEIGHT, exp, ag, dg, meta);
                    unsigned measured_lines =
                        meta ? (unsigned)((exposure[0] << 8) | exposure[1]) : 0;
                    unsigned measured_vts = meta ? (unsigned)((timing[2] << 8) | timing[3]) : 0;
                    bool at_limit = meta && measured_vts > 19 &&
                                    measured_lines == measured_vts - 15 && ag == 8.0f;
                    light = hb_light_apply_dark_limit(light, exp, ag, dg, at_limit);
                    bool publish =
                        now - last_light >= (uint64_t)atomic_load(&sample_seconds) * 1000;
                    portENTER_CRITICAL(&lock);
                    if (publish)
                    {
                        status.light = light;
                        status.light_ms = frame.timestamp_ms;
                    }
                    status.exposure_us = exp;
                    status.analogue_gain = ag;
                    status.digital_gain = dg;
                    status.fps = (float)(sequence - base_sequence) * 1000 / (now - started + 1);
                    portEXIT_CRITICAL(&lock);
                    if (publish)
                    {
                        last_light = now;
                        ESP_LOGI("camera",
                                 "frames=%u fps=%.1f light_valid=%d below_range=%d metric=%.3f "
                                 "luma=%.1f "
                                 "rejected=%.2f exposure_us=%.1f "
                                 "gain=%.2f/%.2f psram=%u largest=%u dma=%u",
                                 (unsigned)sequence,
                                 (double)((float)(sequence - base_sequence) * 1000 /
                                          (now - started + 1)),
                                 light.valid, light.below_range, (double)light.metric,
                                 (double)light.luminance, (double)light.rejected_fraction,
                                 (double)exp, (double)ag, (double)dg,
                                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
                                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL |
                                                                   MALLOC_CAP_DMA));
                    }
                    // Local host auto exposure: no cloud, no permanent fixed exposure.
                    // Read actual gain/exposure each time; discard settling frames after writes.
                    if (meta && ag >= 1 && ag <= 8 && light.luminance >= 0 &&
                        (light.luminance < 64 || light.luminance > 112))
                    {
                        unsigned old_lines = (exposure[0] << 8) | exposure[1];
                        unsigned vts = (timing[2] << 8) | timing[3];
                        unsigned max_lines = vts > 19 ? vts - 15 : 4;
                        float ratio = fminf(1.5f, fmaxf(0.67f, 80 / (light.luminance + 1)));
                        unsigned lines = (unsigned)fminf(max_lines, fmaxf(4, old_lines * ratio));
                        unsigned gain_raw = (gain[0] << 8) | gain[1];
                        unsigned next_gain = gain_raw;
                        if ((lines == max_lines && ratio > 1) || (lines == 4 && ratio < 1))
                            next_gain = (unsigned)fminf(2048, fmaxf(256, gain_raw * ratio));
                        next_gain &= ~1U;
                        esp_err_t control = ESP_OK;
                        if (lines != old_lines)
                        {
                            control = write_reg(0x3501, (uint8_t)(lines >> 8));
                            if (control == ESP_OK)
                                control = write_reg(0x3502, (uint8_t)lines);
                        }
                        if (control == ESP_OK && next_gain != gain_raw)
                        {
                            control = write_reg(0x3508, (uint8_t)(next_gain >> 8));
                            if (control == ESP_OK)
                                control = write_reg(0x3509, (uint8_t)next_gain);
                        }
                        if (control != ESP_OK)
                            e = control;
                        if (lines != old_lines || next_gain != gain_raw)
                            changed = now;
                    }
                    last_control = now;
                }
                hb_camera_release(&frame);
            }
            if (e == ESP_OK && hb_camera_health_timeout(&health, now, sequence))
                e = ESP_ERR_TIMEOUT;
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        portENTER_CRITICAL(&lock);
        status.error = e;
        status.errors++;
        portEXIT_CRITICAL(&lock);
        stop_driver();
        if (isp_quarantined)
        {
            portENTER_CRITICAL(&lock);
            status.quarantined = true;
            portEXIT_CRITICAL(&lock);
            ESP_LOGE("camera", "Driver lifetime uncertain; camera quarantined until reboot; "
                               "dashboard/manual brightness remain available");
            vTaskDelete(NULL);
        }
        uint32_t backoff = hb_camera_health_retry(&health);
        portENTER_CRITICAL(&lock);
        status.retry_seconds = backoff;
        status.restarts++;
        portEXIT_CRITICAL(&lock);
        ESP_LOGW("camera", "capture unavailable: %s; retry in %us", esp_err_to_name(e), backoff);
        vTaskDelay(pdMS_TO_TICKS(backoff * 1000));
    }
}
esp_err_t hb_camera_start(void)
{
    if (atomic_exchange(&started, true))
        return ESP_ERR_INVALID_STATE;
    hb_frame_pool_init(&pool);
    if (xTaskCreate(worker, "local_camera", 6144, NULL, 1, NULL) == pdPASS)
        return ESP_OK;
    atomic_store(&started, false);
    return ESP_ERR_NO_MEM;
}
