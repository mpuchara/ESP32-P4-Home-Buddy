// SPDX-License-Identifier: Apache-2.0
#ifdef HB_AUDIO_HOST_TEST
#include "audio_platform.h"
#else
#include "board_p4_pins.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hb_audio.h"
#endif
#include "hb_audio.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
static atomic_bool requested, busy;
static atomic_uint dropped;
static TaskHandle_t task;
static unsigned selected_slot;
static hb_audio_consumer consumer;
static void *consumer_context;
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE consumer_lock = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE control_lock = portMUX_INITIALIZER_UNLOCKED;
static hb_audio_status published;
static bool overflow(i2s_chan_handle_t handle, i2s_event_data_t *event, void *context)
{
    (void)handle;
    (void)context;
    atomic_fetch_add(&dropped, (unsigned)(event->size / 4));
    return false;
}
static void publish(hb_audio_status *s)
{
    s->dropped_samples = atomic_load(&dropped);
    s->stack_min = uxTaskGetStackHighWaterMark(NULL);
    s->internal_free = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s->internal_largest =
        (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s->psram_free = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    s->psram_largest = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    portENTER_CRITICAL(&lock);
    published = *s;
    portEXIT_CRITICAL(&lock);
}
static void worker(void *unused)
{
    (void)unused;
    for (;;)
    {
        portENTER_CRITICAL(&control_lock);
        bool idle = !atomic_load(&requested);
        if (idle)
            atomic_store(&busy, false); // Stop may precede worker startup.
        portEXIT_CRITICAL(&control_lock);
        if (idle)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        hb_audio_status s = {.sample_rate = 16000, .dma_bytes = 4 * 320 * 4};
        atomic_store(&dropped, 0);
        i2s_chan_handle_t rx = NULL;
        const audio_codec_ctrl_if_t *ctrl = NULL;
        const audio_codec_data_if_t *data = NULL;
        const audio_codec_if_t *codec = NULL;
        esp_codec_dev_handle_t dev = NULL;
        i2c_master_bus_handle_t bus = NULL;
        i2c_master_dev_handle_t probe = NULL;
        esp_err_t err = i2c_master_get_bus_handle(I2C_NUM_0, &bus);
        if (err == ESP_OK)
        {
            i2c_device_config_t cfg = {.dev_addr_length = I2C_ADDR_BIT_LEN_7,
                                       .device_address = 0x18,
                                       .scl_speed_hz = 100000};
            err = i2c_master_bus_add_device(bus, &cfg, &probe);
        }
        uint8_t reg = 0xfd, id[2] = {0};
        if (err == ESP_OK)
            err = i2c_master_transmit_receive(probe, &reg, 1, id, 2, 100);
        if (probe)
            i2c_master_bus_rm_device(probe);
        if (err == ESP_OK && (id[0] != 0x83 || id[1] != 0x11))
            err = ESP_ERR_NOT_FOUND;
        if (err == ESP_OK)
        {
            i2s_chan_config_t cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
            cfg.dma_desc_num = 4;
            cfg.dma_frame_num = 320;
            err = i2s_new_channel(&cfg, NULL, &rx);
        }
        if (err == ESP_OK)
        {
            i2s_std_config_t cfg = {.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
                                    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                                        I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
                                    .gpio_cfg = {.mclk = BOARD_P4_I2S_MCLK,
                                                 .bclk = BOARD_P4_I2S_BCLK,
                                                 .ws = BOARD_P4_I2S_LRCK,
                                                 .dout = I2S_GPIO_UNUSED,
                                                 .din = BOARD_P4_I2S_DIN}};
            err = i2s_channel_init_std_mode(rx, &cfg);
            i2s_event_callbacks_t cb = {.on_recv_q_ovf = overflow};
            if (err == ESP_OK)
                err = i2s_channel_register_event_callback(rx, &cb, NULL);
        }
        if (err == ESP_OK)
        {
            audio_codec_i2c_cfg_t cc = {
                .port = 0, .addr = ES8311_CODEC_DEFAULT_ADDR, .bus_handle = bus};
            ctrl = audio_codec_new_i2c_ctrl(&cc);
            audio_codec_i2s_cfg_t dc = {.port = 0, .rx_handle = rx};
            data = audio_codec_new_i2s_data(&dc);
            if (!ctrl || !data)
                err = ESP_ERR_NO_MEM;
        }
        if (err == ESP_OK)
        {
            es8311_codec_cfg_t cfg = {.ctrl_if = ctrl,
                                      .codec_mode = ESP_CODEC_DEV_WORK_MODE_ADC,
                                      .pa_pin = -1,
                                      .use_mclk = true,
                                      .digital_mic = false,
                                      .no_dac_ref = true,
                                      .mclk_div = 256};
            codec = es8311_codec_new(&cfg);
            esp_codec_dev_cfg_t dc = {
                .dev_type = ESP_CODEC_DEV_TYPE_IN, .codec_if = codec, .data_if = data};
            if (codec)
                dev = esp_codec_dev_new(&dc);
            if (!dev)
                err = ESP_ERR_NO_MEM;
        }
        if (err == ESP_OK)
        {
            esp_codec_dev_sample_info_t cfg = {
                .sample_rate = 16000, .channel = 2, .bits_per_sample = 16};
            if (esp_codec_dev_open(dev, &cfg) != ESP_CODEC_DEV_OK ||
                esp_codec_dev_set_in_gain(dev, 24.0f) != ESP_CODEC_DEV_OK)
                err = ESP_FAIL;
        }
        s.available = err == ESP_OK;
        s.active = s.available;
        snprintf(s.message, sizeof(s.message),
                 s.active ? "Mikrofon: lokalny test 60 s; bez zapisu" : "Mikrofon niedostępny: %s",
                 esp_err_to_name(err));
        if (err != ESP_OK)
            ++s.errors;
        publish(&s);
        int64_t end = esp_timer_get_time() + 60000000;
        int16_t stereo[640] = {0}, mono[320] = {0};
        unsigned previous_dropped = 0;
        int64_t last_log = 0;
        while (err == ESP_OK && atomic_load(&requested) && esp_timer_get_time() < end)
        {
            size_t bytes = 0;
            err = i2s_channel_read(rx, stereo, sizeof(stereo), &bytes, 100);
            int64_t processing_start = esp_timer_get_time();
            s.frame_end_us = (uint64_t)processing_start;
            if (err != ESP_OK || bytes % 4 ||
                !hb_audio_pcm_slot(stereo, bytes / 2, 16000, selected_slot, mono, 320, &s))
            {
                ++s.errors;
                if (err == ESP_OK)
                    err = ESP_FAIL;
                snprintf(s.message, sizeof(s.message), "Błąd PCM: %s", esp_err_to_name(err));
            }
            else
            {
                ++s.frames;
                publish(&s);
                unsigned current_dropped = atomic_load(&dropped);
                portENTER_CRITICAL(&consumer_lock);
                if (consumer && atomic_load(&requested))
                    consumer(mono, bytes / 4, 16000, s.frame_end_us,
                             current_dropped != previous_dropped, consumer_context);
                portEXIT_CRITICAL(&consumer_lock);
                previous_dropped = current_dropped;
            }
            s.processing_us = (uint32_t)(esp_timer_get_time() - processing_start);
            if (processing_start - last_log >= 1000000)
            {
                ESP_LOGI(
                    "audio",
                    "PCM slot=%u samples=%llu rms=%u peak=%u dc=%ld clip=%u repeat=%u other_rms=%u "
                    "other_dc=%ld other_clip=%u other_repeat=%u errors=%u dropped=%u internal=%u "
                    "largest=%u psram=%u largest_psram=%u stack=%u processing_us=%u",
                    selected_slot, (unsigned long long)s.sample_count, (unsigned)s.rms,
                    (unsigned)s.peak, (long)s.dc_offset, s.clipped, s.suspicious,
                    (unsigned)s.right_rms, (long)s.right_dc_offset, (unsigned)s.right_clipped,
                    s.right_suspicious, (unsigned)s.errors, atomic_load(&dropped),
                    (unsigned)s.internal_free, (unsigned)s.internal_largest, (unsigned)s.psram_free,
                    (unsigned)s.psram_largest, (unsigned)s.stack_min, (unsigned)s.processing_us);
                last_log = processing_start;
            }
            memset(stereo, 0, sizeof(stereo));
            memset(mono, 0, sizeof(mono));
            publish(&s);
        }
        if (dev)
        {
            esp_codec_dev_close(dev);
            esp_codec_dev_delete(dev);
        }
        if (codec)
            audio_codec_delete_codec_if(codec);
        if (data)
            audio_codec_delete_data_if(data);
        if (ctrl)
            audio_codec_delete_ctrl_if(ctrl);
        if (rx)
        {
            i2s_channel_disable(rx);
            i2s_del_channel(rx);
        }
        s.active = false;
        atomic_store(&requested, false);
        publish(&s);
        atomic_store(&busy, false);
    }
}
void hb_audio_detach_consumer(hb_audio_consumer callback, void *context)
{
    portENTER_CRITICAL(&consumer_lock);
    if (consumer == callback && consumer_context == context)
    {
        consumer = NULL;
        consumer_context = NULL;
    }
    portEXIT_CRITICAL(&consumer_lock);
}
bool hb_audio_busy(void)
{
    return atomic_load(&busy);
}
esp_err_t hb_audio_start(void)
{
    return hb_audio_start_capture(0, NULL, NULL);
}
esp_err_t hb_audio_start_capture(unsigned slot, hb_audio_consumer callback, void *context)
{
    if (slot > 1)
        return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&control_lock);
    if (atomic_load(&busy))
    {
        portEXIT_CRITICAL(&control_lock);
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&busy, true);
    selected_slot = slot;
    portENTER_CRITICAL(&consumer_lock);
    consumer = callback;
    consumer_context = context;
    portEXIT_CRITICAL(&consumer_lock);
    atomic_store(&requested, true);
    portEXIT_CRITICAL(&control_lock);
    if (!task && xTaskCreate(worker, "audio_diag", 6144, NULL, 1, &task) != pdPASS)
    {
        atomic_store(&requested, false);
        atomic_store(&busy, false);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
void hb_audio_stop(void)
{
    atomic_store(&requested, false);
}
void hb_audio_get(hb_audio_status *out)
{
    if (!out)
        return;
    portENTER_CRITICAL(&lock);
    *out = published;
    portEXIT_CRITICAL(&lock);
}
