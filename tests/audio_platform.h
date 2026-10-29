// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_audio.h"
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_NOT_FOUND 0x105
#define I2C_NUM_0 0
#define I2C_ADDR_BIT_LEN_7 0
#define I2S_NUM_0 0
#define I2S_ROLE_MASTER 0
#define I2S_DATA_BIT_WIDTH_16BIT 16
#define I2S_SLOT_MODE_STEREO 2
#define I2S_GPIO_UNUSED -1
#define BOARD_P4_I2S_MCLK 13
#define BOARD_P4_I2S_BCLK 12
#define BOARD_P4_I2S_LRCK 10
#define BOARD_P4_I2S_DIN 48
#define ES8311_CODEC_DEFAULT_ADDR 0x30
#define ESP_CODEC_DEV_WORK_MODE_ADC 1
#define ESP_CODEC_DEV_TYPE_IN 1
#define ESP_CODEC_DEV_OK 0
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
#define MALLOC_CAP_SPIRAM 4
#define pdPASS 1
#define pdMS_TO_TICKS(x) (x)
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
#define ESP_LOGI(...) ((void)0)
typedef int portMUX_TYPE;
typedef void *TaskHandle_t;
typedef void *i2s_chan_handle_t;
typedef void *i2c_master_bus_handle_t;
typedef void *i2c_master_dev_handle_t;
typedef void *esp_codec_dev_handle_t;
typedef int audio_codec_ctrl_if_t, audio_codec_data_if_t, audio_codec_if_t;
typedef struct
{
    size_t size;
} i2s_event_data_t;
typedef struct
{
    int dev_addr_length, device_address, scl_speed_hz;
} i2c_device_config_t;
typedef struct
{
    int dma_desc_num, dma_frame_num;
} i2s_chan_config_t;
#define I2S_CHANNEL_DEFAULT_CONFIG(a, b) ((i2s_chan_config_t){0})
typedef struct
{
    int clk_cfg, slot_cfg;
    struct
    {
        int mclk, bclk, ws, dout, din;
    } gpio_cfg;
} i2s_std_config_t;
#define I2S_STD_CLK_DEFAULT_CONFIG(a) (a)
#define I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(a, b) (a)
typedef struct
{
    bool (*on_recv_q_ovf)(i2s_chan_handle_t, i2s_event_data_t *, void *);
} i2s_event_callbacks_t;
typedef struct
{
    int port, addr;
    void *bus_handle;
} audio_codec_i2c_cfg_t;
typedef struct
{
    int port;
    void *rx_handle;
} audio_codec_i2s_cfg_t;
typedef struct
{
    const void *ctrl_if;
    int codec_mode, pa_pin;
    bool use_mclk, digital_mic, no_dac_ref;
    int mclk_div;
} es8311_codec_cfg_t;
typedef struct
{
    int dev_type;
    const void *codec_if, *data_if;
} esp_codec_dev_cfg_t;
typedef struct
{
    int sample_rate, channel, bits_per_sample;
} esp_codec_dev_sample_info_t;
static jmp_buf idle_jump;
static void (*entry)(void *);
static int resources, writes, reads, failure, samples_seen;
static int token;
static int64_t clock_us;
static void vTaskDelay(int ticks)
{
    (void)ticks;
    longjmp(idle_jump, 1);
}
static int xTaskCreate(void (*f)(void *), const char *name, unsigned stack, void *arg,
                       unsigned priority, TaskHandle_t *task)
{
    (void)name;
    (void)stack;
    (void)arg;
    (void)priority;
    entry = f;
    *task = &token;
    return pdPASS;
}
static uint32_t uxTaskGetStackHighWaterMark(void *task)
{
    (void)task;
    return 1000;
}
static size_t heap_caps_get_free_size(int caps)
{
    (void)caps;
    return 100000;
}
static size_t heap_caps_get_largest_free_block(int caps)
{
    (void)caps;
    return 100000;
}
static int64_t esp_timer_get_time(void)
{
    return clock_us += 20000;
}
static const char *esp_err_to_name(int err)
{
    return err ? "error" : "ok";
}
static int i2c_master_get_bus_handle(int n, void **bus)
{
    (void)n;
    *bus = &token;
    return failure == 1 ? ESP_FAIL : ESP_OK;
}
static int i2c_master_bus_add_device(void *bus, const i2c_device_config_t *c, void **dev)
{
    (void)bus;
    (void)c;
    *dev = &token;
    ++resources;
    return ESP_OK;
}
static int i2c_master_transmit_receive(void *dev, uint8_t *reg, size_t a, uint8_t *out, size_t b,
                                       int timeout)
{
    (void)dev;
    (void)reg;
    (void)a;
    (void)b;
    (void)timeout;
    out[0] = failure == 2 ? 0 : 0x83;
    out[1] = 0x11;
    return ESP_OK;
}
static int i2c_master_bus_rm_device(void *dev)
{
    (void)dev;
    --resources;
    return ESP_OK;
}
static int i2s_new_channel(i2s_chan_config_t *c, void *tx, void **rx)
{
    (void)c;
    (void)tx;
    if (failure == 3)
        return ESP_ERR_NO_MEM;
    *rx = &token;
    ++resources;
    return ESP_OK;
}
static int i2s_channel_init_std_mode(void *rx, i2s_std_config_t *c)
{
    (void)rx;
    assert(c->gpio_cfg.din == 48);
    return ESP_OK;
}
static int i2s_channel_register_event_callback(void *rx, i2s_event_callbacks_t *cb, void *ctx)
{
    (void)rx;
    (void)ctx;
    i2s_event_data_t event = {.size = 16};
    cb->on_recv_q_ovf(rx, &event, ctx);
    return ESP_OK;
}
static int i2s_channel_disable(void *rx)
{
    (void)rx;
    return ESP_OK;
}
static int i2s_del_channel(void *rx)
{
    (void)rx;
    --resources;
    return ESP_OK;
}
static const int *audio_codec_new_i2c_ctrl(audio_codec_i2c_cfg_t *c)
{
    (void)c;
    ++resources;
    ++writes;
    return &token;
}
static const int *audio_codec_new_i2s_data(audio_codec_i2s_cfg_t *c)
{
    (void)c;
    ++resources;
    return &token;
}
static const int *es8311_codec_new(es8311_codec_cfg_t *c)
{
    assert(c->codec_mode == ESP_CODEC_DEV_WORK_MODE_ADC && c->pa_pin == -1);
    ++resources;
    return &token;
}
static void *esp_codec_dev_new(esp_codec_dev_cfg_t *c)
{
    (void)c;
    ++resources;
    return &token;
}
static int esp_codec_dev_open(void *dev, esp_codec_dev_sample_info_t *c)
{
    (void)dev;
    assert(c->sample_rate == 16000 && c->channel == 2);
    return failure == 4 ? -1 : 0;
}
static int esp_codec_dev_set_in_gain(void *dev, float gain)
{
    (void)dev;
    (void)gain;
    return 0;
}
static int esp_codec_dev_close(void *dev)
{
    (void)dev;
    return 0;
}
static void esp_codec_dev_delete(void *dev)
{
    (void)dev;
    --resources;
}
static void audio_codec_delete_codec_if(const int *dev)
{
    (void)dev;
    --resources;
}
static void audio_codec_delete_data_if(const int *dev)
{
    (void)dev;
    --resources;
}
static void audio_codec_delete_ctrl_if(const int *dev)
{
    (void)dev;
    --resources;
}
static int i2s_channel_read(void *rx, void *out, size_t len, size_t *bytes, int timeout)
{
    (void)rx;
    (void)timeout;
    ++reads;
    if (failure == 5)
        return ESP_FAIL;
    *bytes = failure == 6 ? 3 : len;
    int16_t *p = out;
    for (size_t i = 0; i < len / 2; ++i)
        p[i] = (int16_t)(i * 13);
    if (reads > 2)
        hb_audio_stop();
    return ESP_OK;
}
