// SPDX-License-Identifier: Apache-2.0
#include "hb_voice.h"
#ifdef HB_VOICE_HOST_TEST
#include "voice_platform.hpp"
#else
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hb_compute_budget.h"
#include "sdkconfig.h"
#if CONFIG_HB_IDENTITY_EXPERIMENTAL
#include "dl_audio_common.hpp"
#include "dl_fft.hpp"
#include "dl_model_base.hpp"
#include "esp_partition.h"
#include "mbedtls/sha256.h"
#include "spi_flash_mmap.h"
#endif
#endif
#include <cmath>
#include <cstring>
#include <new>
#if CONFIG_HB_IDENTITY_EXPERIMENTAL
namespace
{
// Checked ownership around the locked low-level Fbank operations. The upstream
// Fbank constructor dereferences unchecked mel allocations; do not use it here.
#include "voice_features.hpp"
struct Mapping
{
    const void *data = nullptr;
    esp_partition_mmap_handle_t handle{};
    ~Mapping()
    {
        if (data)
            esp_partition_munmap(handle);
    }
    esp_err_t open(hb_voice_cancel_fn cancel, void *ctx)
    {
        const esp_partition_t *p = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA, (esp_partition_subtype_t)0x43, "sv_model_3s");
        if (!p)
            return ESP_ERR_NOT_FOUND;
        if (p->size != 0x300000 || p->address != 0xc20000 || p->encrypted)
            return ESP_ERR_INVALID_SIZE;
        if ((size_t)spi_flash_mmap_get_free_pages(SPI_FLASH_MMAP_DATA) * 65536 < p->size)
            return ESP_ERR_NO_MEM;
        esp_err_t err = esp_partition_mmap(p, 0, p->size, ESP_PARTITION_MMAP_DATA, &data, &handle);
        if (err != ESP_OK)
            return err;
        static const uint8_t expected[32] = {0xb5, 0x03, 0xff, 0x04, 0x9a, 0x0c, 0xd5, 0xc6,
                                             0x2b, 0x4c, 0x1a, 0xc3, 0x37, 0xcf, 0x20, 0xb7,
                                             0xde, 0x15, 0x49, 0x85, 0x7a, 0x4d, 0x53, 0x29,
                                             0xa1, 0x82, 0x1c, 0xfb, 0x55, 0xe9, 0xb3, 0xa8};
        mbedtls_sha256_context hash;
        mbedtls_sha256_init(&hash);
        int rc = mbedtls_sha256_starts(&hash, 0);
        uint8_t digest[32];
        for (size_t i = 0; !rc && i < 2844624; i += 65536)
        {
            if (cancel && cancel(ctx))
            {
                mbedtls_sha256_free(&hash);
                return ESP_ERR_INVALID_STATE;
            }
            size_t len = 2844624 - i;
            if (len > 65536)
                len = 65536;
            rc = mbedtls_sha256_update(&hash, (const uint8_t *)data + i, len);
            vTaskDelay(1);
        }
        if (!rc)
            rc = mbedtls_sha256_finish(&hash, digest);
        mbedtls_sha256_free(&hash);
        return rc || memcmp(digest, expected, 32) ? ESP_ERR_INVALID_CRC : ESP_OK;
    }
};
static void sample_memory(hb_voice_metrics *m)
{
    m->internal_free = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    m->psram_free = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    m->internal_largest =
        (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    m->psram_largest = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    if (!m->internal_low || m->internal_free < m->internal_low)
        m->internal_low = m->internal_free;
    if (!m->psram_low || m->psram_free < m->psram_low)
        m->psram_low = m->psram_free;
    uint32_t stack = uxTaskGetStackHighWaterMark(nullptr);
    if (!m->stack_min || stack < m->stack_min)
        m->stack_min = stack;
}
struct ComputeLease
{
    ~ComputeLease()
    {
        hb_compute_leave();
    }
};
} // namespace
#endif
esp_err_t hb_voice_embedding_run(const int16_t *pcm, size_t n, uint32_t rate,
                                 hb_voice_cancel_fn cancel, void *ctx, hb_voice_embedding *out,
                                 hb_voice_metrics *metrics)
{
    if (!out || !metrics)
        return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));
    memset(metrics, 0, sizeof(*metrics));
    if (!pcm || n != HB_VOICE_SAMPLES || rate != 16000)
        return ESP_ERR_INVALID_ARG;
    if (cancel && cancel(ctx))
        return ESP_ERR_INVALID_STATE;
    hb_voice_gate gate{};
    for (size_t offset = 0; offset < n; offset += 320)
    {
        size_t count = n - offset;
        if (count > 320)
            count = 320;
        if (hb_voice_gate_feed(&gate, pcm + offset, count, rate, 1 + (offset + count) / 16) ==
            HB_VAD_REJECTED)
            return ESP_ERR_INVALID_ARG;
    }
    if (gate.state != HB_VAD_READY)
        return ESP_ERR_INVALID_ARG;
#if CONFIG_HB_IDENTITY_EXPERIMENTAL
    if (!hb_compute_enter(0))
        return ESP_ERR_TIMEOUT;
    ComputeLease lease;
    sample_memory(metrics);
    // Conservative engineering admission, not measured simultaneous-model fit.
    if (metrics->internal_largest < 65536 || metrics->internal_free < 131072 ||
        metrics->psram_largest < 8 * 1024 * 1024)
        return ESP_ERR_NO_MEM;
    Mapping mapping;
    esp_err_t err = mapping.open(cancel, ctx);
    if (err != ESP_OK)
        return err;
    int64_t start = esp_timer_get_time();
    // Flash-backed parameters reduce residency. The pinned loader still contains
    // internal STL allocations: physical low-memory fault acceptance remains pending.
    auto *model = new (std::nothrow)
        dl::Model((const char *)mapping.data, fbs::MODEL_LOCATION_IN_FLASH_RODATA, 0,
                  dl::MEMORY_MANAGER_GREEDY, nullptr, false);
    if (!model)
        return ESP_ERR_NO_MEM;
    struct ModelOwner
    {
        dl::Model *p;
        ~ModelOwner()
        {
            delete p;
        }
    } owner{model};
    metrics->load_us = (uint32_t)(esp_timer_get_time() - start);
    sample_memory(metrics);
    auto &inputs = model->get_inputs();
    auto &outputs = model->get_outputs();
    if (inputs.size() != 1 || outputs.size() != 1)
        return ESP_ERR_INVALID_VERSION;
    auto *in = inputs.begin()->second, *output = outputs.begin()->second;
    if (!in || !output || !in->data || !output->data ||
        in->shape != std::vector<int>({1, 300, 80}) ||
        output->shape != std::vector<int>({1, 512}) || in->dtype != dl::DATA_TYPE_INT16 ||
        output->dtype != dl::DATA_TYPE_INT8 || in->size != 24000 || output->size != 512 ||
        !in->exponent.is_valid() || !output->exponent.is_valid() || in->exponent.is_per_channel() ||
        output->exponent.is_per_channel() || (int)in->exponent < -30 || (int)in->exponent > 16 ||
        (int)output->exponent < -30 || (int)output->exponent > 16)
        return ESP_ERR_INVALID_VERSION;
    Features features;
    if (!features.init())
        return ESP_ERR_NO_MEM;
    start = esp_timer_get_time();
    err = features.run(pcm, n, cancel, ctx);
    if (err != ESP_OK)
        return err;
    if (!hb_voice_quantize(features.values, 24000, in->exponent, (int16_t *)in->data))
        return ESP_ERR_INVALID_ARG;
    metrics->features_us = (uint32_t)(esp_timer_get_time() - start);
    sample_memory(metrics);
    // Staged inference permits cancellation/yield between graph sections.
    // Output may alias the model input in the greedy arena. Do not clear it
    // before execution: that would corrupt freshly quantized input features.
    start = esp_timer_get_time();
    for (int stage = 0; stage < 16; ++stage)
    {
        if (cancel && cancel(ctx))
            return ESP_ERR_INVALID_STATE;
        model->run(16, stage);
        vTaskDelay(1);
    }
    metrics->inference_us = (uint32_t)(esp_timer_get_time() - start);
    sample_memory(metrics);
    if (cancel && cancel(ctx))
        return ESP_ERR_INVALID_STATE;
    return hb_voice_dequantize((int8_t *)output->data, 512, output->exponent, out) ? ESP_OK
                                                                                   : ESP_FAIL;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}
