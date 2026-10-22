// SPDX-License-Identifier: Apache-2.0
#include "driver/jpeg_decode.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "hb_dashboard.h"
#include "hb_image.h"
#include <stdlib.h>
#include <string.h>
esp_err_t hb_image_decode(const uint8_t *data, size_t n, uint16_t **pixels)
{
    uint16_t w, h;
    *pixels = NULL;
    if (!hb_jpeg_bounds(data, n, &w, &h))
        return ESP_ERR_INVALID_ARG;
    jpeg_decoder_handle_t decoder = NULL;
    jpeg_decode_engine_cfg_t engine = {.timeout_ms = 1000};
    esp_err_t e = jpeg_new_decoder_engine(&engine, &decoder);
    if (e)
        return e;
    size_t insz, outsz;
    jpeg_decode_memory_alloc_cfg_t in = {.buffer_direction = JPEG_DEC_ALLOC_INPUT_BUFFER};
    jpeg_decode_memory_alloc_cfg_t out = {.buffer_direction = JPEG_DEC_ALLOC_OUTPUT_BUFFER};
    uint8_t *input = jpeg_alloc_decoder_mem(n, &in, &insz);
    uint32_t stride = (w + 15u) & ~15u, rows = (h + 15u) & ~15u;
    uint16_t *raw = jpeg_alloc_decoder_mem((size_t)stride * rows * 2, &out, &outsz);
    uint16_t *frame = heap_caps_malloc(800 * 480 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!input || !raw || !frame)
    {
        e = ESP_ERR_NO_MEM;
        goto done;
    }
    memcpy(input, data, n);
    jpeg_decode_cfg_t config = {.output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
                                .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_RGB};
    uint32_t written = 0;
    int64_t start = esp_timer_get_time();
    e = jpeg_decoder_process(decoder, &config, input, n, (uint8_t *)raw, outsz, &written);
    if (!e && written < (size_t)stride * rows * 2)
        e = ESP_ERR_INVALID_SIZE;
    if (!e && !hb_scale_cover(raw, w, h, stride, frame))
        e = ESP_FAIL;
    if (!e)
    {
        *pixels = frame;
        frame = NULL;
        ESP_LOGI("image",
                 "JPEG %ux%u; decode+cover=%lldms; internal_free=%u largest=%u psram_free=%u "
                 "largest=%u staging=%u decoded=%u output=768000",
                 w, h, (long long)((esp_timer_get_time() - start) / 1000),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM), (unsigned)insz,
                 (unsigned)outsz);
    }
done:
    free(frame);
    free(raw);
    free(input);
    jpeg_del_decoder_engine(decoder);
    return e;
}
