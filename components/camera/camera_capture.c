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
#error "Local RGB888 capture requires the audited project CSI DMA output-length adapter"
#endif
#define WIDTH 1288
#define HEIGHT 728
#define FRAME_BYTES (WIDTH * HEIGHT * 3U)
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static hb_frame_pool pool;
static uint8_t *storage[HB_CAMERA_SLOTS];
static uint64_t frame_time[HB_CAMERA_SLOTS];
static uint32_t frame_seq[HB_CAMERA_SLOTS];
static uint64_t settle_until;
static unsigned face_x, face_y, face_w, face_h;
static uint64_t face_ms;
static hb_camera_status status;
static esp_cam_ctlr_handle_t controller;
static isp_proc_handle_t isp;
static i2c_master_dev_handle_t sensor;
static bool controller_enabled, controller_started, isp_enabled;
static bool isp_quarantined;
static bool bf_enabled, demosaic_enabled, ccm_enabled, gamma_enabled, color_enabled;
static bool sharpen_enabled;
static float wb_red = 1.65f, wb_blue = 1.55f;
static unsigned tuning_band;
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
        f->settled = f->timestamp_ms >= settle_until;
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
void hb_camera_face_hint(unsigned x, unsigned y, unsigned w, unsigned h, uint64_t frame_ms)
{
    if (!w || !h || x >= HEIGHT || y >= WIDTH || w > HEIGHT - x || h > WIDTH - y)
        return;
    portENTER_CRITICAL(&lock);
    if (frame_ms >= face_ms)
    {
        face_x = x;
        face_y = y;
        face_w = w;
        face_h = h;
        face_ms = frame_ms;
    }
    portEXIT_CRITICAL(&lock);
}
static void settling(uint64_t now)
{
    portENTER_CRITICAL(&lock);
    settle_until = now + 200;
    portEXIT_CRITICAL(&lock);
}
void hb_camera_set_sample_seconds(uint32_t seconds)
{
    if (seconds >= 1 && seconds <= 60)
        atomic_store(&sample_seconds, seconds);
}
// OV02C10 SDK-inspired conservative fixed tuning. Keep the sensor/CSI owner
// and audited DMA adapter; no second capture owner or competing AE controller.
static esp_err_t configure_gamma_channel(color_component_t channel, uint32_t (*curve)(uint32_t))
{
    isp_gamma_curve_points_t points;
    esp_err_t e = esp_isp_gamma_fill_curve_points(curve, &points);
    if (e != ESP_OK)
        return e;
    return esp_isp_gamma_configure(isp, channel, &points);
}
static uint32_t red_curve(uint32_t x)
{
    return hb_isp_gamma_gain(x, wb_red);
}
static uint32_t blue_curve(uint32_t x)
{
    return hb_isp_gamma_gain(x, wb_blue);
}
static esp_err_t tune_detail(unsigned band)
{
    esp_isp_bf_config_t bf = {.padding_mode = ISP_BF_EDGE_PADDING_MODE_SRND_DATA,
                              .bf_template = {{2, 4, 2}, {4, 5, 4}, {2, 4, 2}},
                              .denoising_level = 3};
    if (band)
    {
        const uint8_t matrix[3][3] = {{1, 3, 1}, {3, 5, 3}, {1, 3, 1}};
        memcpy(bf.bf_template, matrix, sizeof(matrix));
        bf.denoising_level = band == 2 ? 4 : 3;
    }
    esp_err_t e = esp_isp_bf_configure(isp, &bf);
    if (e != ESP_OK)
        return e;
    esp_isp_demosaic_config_t dm = {.grad_ratio = {.integer = 1},
                                    .padding_mode = ISP_DEMOSAIC_EDGE_PADDING_MODE_SRND_DATA};
    unsigned fraction = 1U << ISP_DEMOSAIC_GRAD_RATIO_DEC_BITS;
    if (band < 2)
        dm.grad_ratio.decimal = fraction / (band ? 4 : 2);
    e = esp_isp_demosaic_configure(isp, &dm);
    if (e != ESP_OK)
        return e;
    esp_isp_sharpen_config_t sh = {.h_freq_coeff = {.integer = 1},
                                   .m_freq_coeff = {.integer = 1},
                                   .h_thresh = 25,
                                   .l_thresh = 5,
                                   .padding_mode = ISP_SHARPEN_EDGE_PADDING_MODE_SRND_DATA,
                                   .sharpen_template = {{1, 2, 1}, {2, 2, 2}, {1, 2, 1}}};
    sh.h_freq_coeff.decimal = (1U << ISP_SHARPEN_H_FREQ_COEF_DEC_BITS) / (band ? 4 : 2);
    sh.m_freq_coeff.decimal = (1U << ISP_SHARPEN_M_FREQ_COEF_DEC_BITS) / (band ? 8 : 4);
    return esp_isp_sharpen_configure(isp, &sh);
}
static esp_err_t configure_isp(void)
{
    esp_isp_bf_config_t bf = {.padding_mode = ISP_BF_EDGE_PADDING_MODE_SRND_DATA,
                              .bf_template = {{2, 4, 2}, {4, 5, 4}, {2, 4, 2}},
                              .denoising_level = 3};
    esp_err_t e = esp_isp_bf_configure(isp, &bf);
    if (e != ESP_OK)
        return e;
    e = esp_isp_bf_enable(isp);
    if (e != ESP_OK)
        return e;
    bf_enabled = true;
    esp_isp_demosaic_config_t dm = {.grad_ratio = {.integer = 1, .decimal = 0},
                                    .padding_mode = ISP_DEMOSAIC_EDGE_PADDING_MODE_SRND_DATA};
    e = esp_isp_demosaic_configure(isp, &dm);
    if (e != ESP_OK)
        return e;
    e = esp_isp_demosaic_enable(isp);
    if (e != ESP_OK)
        return e;
    demosaic_enabled = true;
    esp_isp_ccm_config_t ccm = {.matrix = {{1.408f, -0.094f, -0.314f},
                                           {-0.13f, 1.28f, -0.15f},
                                           {-0.072f, -0.173f, 1.245f}}};
    e = esp_isp_ccm_configure(isp, &ccm);
    if (e != ESP_OK)
        return e;
    e = esp_isp_ccm_enable(isp);
    if (e != ESP_OK)
        return e;
    ccm_enabled = true;
    wb_red = 1.65f;
    wb_blue = 1.55f;
    e = configure_gamma_channel(COLOR_COMPONENT_R, red_curve);
    if (e == ESP_OK)
        e = configure_gamma_channel(COLOR_COMPONENT_G, hb_isp_gamma_black_corrected);
    if (e == ESP_OK)
        e = configure_gamma_channel(COLOR_COMPONENT_B, blue_curve);
    if (e != ESP_OK)
        return e;
    e = esp_isp_gamma_enable(isp);
    if (e != ESP_OK)
        return e;
    gamma_enabled = true;
    // OV02C10 SDK ACC saturation=136 in Q1.7 (unity=128).
    // Leave contrast at unity and brightness/hue at zero to retain the corrected black.
    esp_isp_color_config_t color = {.color_contrast = {.val = 128},
                                    .color_saturation = {.val = 136},
                                    .color_hue = 0,
                                    .color_brightness = 0};
    e = esp_isp_color_configure(isp, &color);
    if (e != ESP_OK)
        return e;
    e = esp_isp_color_enable(isp);
    color_enabled = e == ESP_OK;
    if (e != ESP_OK)
        return e;
    tuning_band = 0;
    e = tune_detail(tuning_band);
    if (e != ESP_OK)
        return e;
    e = esp_isp_sharpen_enable(isp);
    sharpen_enabled = e == ESP_OK;
    return e;
}
static esp_err_t disable_isp_modules(void)
{
    esp_err_t e = ESP_OK;
    if (sharpen_enabled && (e = esp_isp_sharpen_disable(isp)) != ESP_OK)
        return e;
    sharpen_enabled = false;
    if (color_enabled && (e = esp_isp_color_disable(isp)) != ESP_OK)
        return e;
    color_enabled = false;
    if (gamma_enabled && (e = esp_isp_gamma_disable(isp)) != ESP_OK)
        return e;
    gamma_enabled = false;
    if (ccm_enabled && (e = esp_isp_ccm_disable(isp)) != ESP_OK)
        return e;
    ccm_enabled = false;
    if (demosaic_enabled && (e = esp_isp_demosaic_disable(isp)) != ESP_OK)
        return e;
    demosaic_enabled = false;
    if (bf_enabled && (e = esp_isp_bf_disable(isp)) != ESP_OK)
        return e;
    bf_enabled = false;
    return e;
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
                                           .output_data_color_type = CAM_CTLR_COLOR_RGB888,
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
                                      .output_data_color_type = ISP_COLOR_RGB888,
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
    e = configure_isp();
    if (e != ESP_OK)
        return e;
    e = esp_cam_ctlr_start(controller);
    if (e != ESP_OK)
        return e;
    controller_started = true;
    e = write_reg(0x0100, 1);
    if (e != ESP_OK)
        return e;
    portENTER_CRITICAL(&lock);
    status.running = true;
    settle_until = (uint64_t)esp_timer_get_time() / 1000 + 200;
    face_ms = 0;
    status.width = WIDTH;
    status.height = HEIGHT;
    portEXIT_CRITICAL(&lock);
    ESP_LOGI("camera",
             "CSI RAW10 1288x728 1 lane 400Mbps -> ISP RGB888 tuned; pool=%u + discard=%u",
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
        (isp_enabled && disable_isp_modules() != ESP_OK) ||
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
        uint64_t last_control = 0, changed = started, last_wb = 0;
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
                settling(now);
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
                        size_t offset = (size_t)(HEIGHT / 2) * WIDTH;
                        offset += (2 * bar + 1) * WIDTH / 16;
                        const uint8_t *p = frame.pixels + offset * 3;
                        ESP_LOGI("camera_pattern", "bar=%u R=%u G=%u B=%u", bar, p[2], p[1], p[0]);
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
                    unsigned x = HEIGHT / 8, y = WIDTH / 8;
                    unsigned w = HEIGHT * 3 / 4, h = WIDTH * 3 / 4;
                    portENTER_CRITICAL(&lock);
                    bool face = face_ms && face_ms <= now && now - face_ms < 2000;
                    if (face)
                    {
                        x = face_x;
                        y = face_y;
                        w = face_w;
                        h = face_h;
                    }
                    portEXIT_CRITICAL(&lock);
                    hb_rgb888_stats q = hb_rgb888_quality(frame.pixels, WIDTH, HEIGHT, x, y, w, h);
                    hb_light_reading light = {.luminance = q.mean, .rejected_fraction = q.clipped};
                    // Scene-only, bounded neutral-patch adaptation. Hold during face acquisition.
                    if (!face && now - last_wb >= 2000 && q.mean > 55 && q.mean < 210)
                    {
                        float r, b;
                        last_wb = now;
                        if (hb_rgb888_neutral(frame.pixels, WIDTH, HEIGHT, &r, &b))
                        {
                            wb_red = fminf(2.1f, fmaxf(1.25f, wb_red * (1 + 0.15f * (r - 1))));
                            wb_blue = fminf(2.05f, fmaxf(1.2f, wb_blue * (1 + 0.15f * (b - 1))));
                            e = configure_gamma_channel(COLOR_COMPONENT_R, red_curve);
                            if (e == ESP_OK)
                                e = configure_gamma_channel(COLOR_COMPONENT_B, blue_curve);
                            changed = now;
                            settling(now);
                        }
                    }
                    unsigned band = ag >= 6 ? 2 : ag >= 3 ? 1 : 0;
                    if (meta && e == ESP_OK && band != tuning_band)
                    {
                        e = tune_detail(band);
                        if (e == ESP_OK)
                            tuning_band = band;
                        changed = now;
                        settling(now);
                    }
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
                        // RGB888 is gamma/CCM corrected: never reuse RAW light calibration.
                        status.light = (hb_light_reading){0};
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
                    if (meta && e == ESP_OK && ag >= 1 && ag <= 8 && q.valid)
                    {
                        unsigned old_lines = (exposure[0] << 8) | exposure[1];
                        unsigned vts = (timing[2] << 8) | timing[3];
                        unsigned max_lines = vts > 19 ? vts - 15 : 4;
                        unsigned gain_raw = (gain[0] << 8) | gain[1];
                        unsigned lines = old_lines, next_gain = gain_raw;
                        float line_us = ((timing[0] << 8) | timing[1]) / 81.6667f;
                        hb_camera_exposure_plan(line_us, max_lines, old_lines, gain_raw, q.mean,
                                                face, &lines, &next_gain);
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
                        {
                            changed = now;
                            settling(now);
                        }
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
    if (xTaskCreatePinnedToCore(worker, "local_camera", 6144, NULL, 2, NULL, 0) == pdPASS)
        return ESP_OK;
    atomic_store(&started, false);
    return ESP_ERR_NO_MEM;
}
