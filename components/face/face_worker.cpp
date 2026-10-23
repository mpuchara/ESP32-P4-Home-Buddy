// SPDX-License-Identifier: Apache-2.0
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hb_face.h"
#include "human_face_detect.hpp"
extern "C"
{
#include "hb_camera.h"
#include "hb_camera_preview.h"
#include "hb_compute_budget.h"
#include "hb_raw_frame.h"
}
#include <atomic>
#include <cstdlib>
#include <new>
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static std::atomic<bool> requested{false}, started{false};
static std::atomic<unsigned> epoch{0};
static hb_face_status status{};
static uint64_t now_ms()
{
    return esp_timer_get_time() / 1000;
}
static bool admission(bool loading)
{
    return hb_face_admit(heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                         heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
                         heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                         heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                         loading);
}
static void fail(esp_err_t error)
{
    requested.store(false);
    portENTER_CRITICAL(&lock);
    status.error = error;
    status.count = 0;
    portEXIT_CRITICAL(&lock);
    ESP_LOGW("face", "Diagnostic stopped: %s", esp_err_to_name(error));
}
static void worker(void *)
{
    HumanFaceDetect *model = nullptr;
    uint16_t *pixels = nullptr;
    uint32_t last_sequence = 0;
    for (;;)
    {
        if (!requested.load())
        {
            if ((model || pixels) && hb_compute_enter(0))
            {
                delete model;
                model = nullptr;
                free(pixels);
                pixels = nullptr;
                portENTER_CRITICAL(&lock);
                status.ready = false;
                status.count = 0;
                portEXIT_CRITICAL(&lock);
                hb_compute_leave();
                ESP_LOGI("face", "Diagnostic allocations released; psram=%u",
                         (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        if (!hb_compute_enter(0))
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        if (!admission(model == nullptr))
        {
            hb_compute_leave();
            fail(ESP_ERR_NO_MEM);
            continue;
        }
        unsigned ticket = epoch.load();
        if (!model)
        {
            size_t before_p = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
            size_t before_i = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            free(pixels);
            pixels = nullptr;
            pixels = (uint16_t *)heap_caps_malloc(HB_FACE_INPUT_BYTES,
                                                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!pixels)
            {
                hb_compute_leave();
                fail(ESP_ERR_NO_MEM);
                continue;
            }
            model = new (std::nothrow)
                HumanFaceDetect(HumanFaceDetect::ESPDET_PICO_416_416_FACE, false);
            if (!model)
            {
                free(pixels);
                pixels = nullptr;
                hb_compute_leave();
                fail(ESP_ERR_NO_MEM);
                continue;
            }
            size_t after_p = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
            size_t after_i = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            size_t used_p = before_p > after_p ? before_p - after_p : 0;
            size_t used_i = before_i > after_i ? before_i - after_i : 0;
            ESP_LOGI("face", "ESPDet416 loaded; psram_delta=%u internal_delta=%u input=%u",
                     (unsigned)used_p, (unsigned)used_i, (unsigned)HB_FACE_INPUT_BYTES);
            if (used_p > 4 * 1024 * 1024 || used_i > 64 * 1024)
            {
                delete model;
                model = nullptr;
                free(pixels);
                pixels = nullptr;
                hb_compute_leave();
                fail(ESP_ERR_NO_MEM);
                continue;
            }
            portENTER_CRITICAL(&lock);
            status.ready = true;
            portEXIT_CRITICAL(&lock);
        }
        if (!requested.load() || epoch.load() != ticket)
        {
            hb_compute_leave();
            continue;
        }
        hb_camera_status camera{};
        hb_camera_get_status(&camera);
        hb_camera_frame frame{};
        if (!camera.running || camera.test_pattern || !hb_camera_acquire(&frame))
        {
            hb_compute_leave();
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        uint32_t seq = frame.sequence;
        uint64_t stamp = frame.timestamp_ms;
        if (seq == last_sequence || stamp > now_ms() || now_ms() - stamp > 500)
        {
            hb_camera_release(&frame);
            hb_compute_leave();
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        hb_raw_thumbnail_upright(frame.pixels, frame.width, frame.height, pixels,
                                 HB_FACE_INPUT_WIDTH, HB_FACE_INPUT_HEIGHT);
        hb_camera_release(&frame); // Never hold sensor storage during model inference.
        hb_camera_make_preview(pixels, HB_FACE_INPUT_WIDTH, HB_FACE_INPUT_HEIGHT, pixels,
                               HB_FACE_INPUT_WIDTH, HB_FACE_INPUT_HEIGHT);
        dl::image::img_t image{.data = pixels,
                               .width = HB_FACE_INPUT_WIDTH,
                               .height = HB_FACE_INPUT_HEIGHT,
                               .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565LE};
        int64_t begin = esp_timer_get_time();
        auto &results = model->run(image);
        hb_face_status result{};
        result.ready = true;
        result.error = ESP_OK;
        result.frame_ms = stamp;
        result.sequence = seq;
        result.inference_us = (uint32_t)(esp_timer_get_time() - begin);
        for (auto &r : results)
        {
            if (result.count >= HB_FACE_MAX)
                break;
            if (r.box.size() != 4 || r.score < 0.7f)
                continue;
            hb_face_box b{r.box[0], r.box[1], r.box[2], r.box[3], r.score};
            if (hb_face_map_box(&b))
                result.boxes[result.count++] = b;
        }
        if (requested.load() && epoch.load() == ticket)
        {
            portENTER_CRITICAL(&lock);
            status = result;
            portEXIT_CRITICAL(&lock);
            ESP_LOGI("face", "sequence=%u count=%u inference_us=%u psram=%u largest=%u dma=%u",
                     (unsigned)seq, (unsigned)result.count, (unsigned)result.inference_us,
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                     (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
        }
        last_sequence = seq;
        hb_compute_leave();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
esp_err_t hb_face_enable(bool enabled)
{
    epoch.fetch_add(1);
    requested.store(enabled);
    portENTER_CRITICAL(&lock);
    status.count = 0;
    status.frame_ms = 0;
    if (enabled)
        status.error = ESP_OK;
    portEXIT_CRITICAL(&lock);
    if (enabled && !started.exchange(true))
    {
        if (xTaskCreate(worker, "face_diagnostic", 8192, nullptr, 1, nullptr) != pdPASS)
        {
            started.store(false);
            requested.store(false);
            return ESP_ERR_NO_MEM;
        }
    }
    return ESP_OK;
}
void hb_face_get(hb_face_status *out)
{
    if (!out)
        return;
    portENTER_CRITICAL(&lock);
    *out = status;
    portEXIT_CRITICAL(&lock);
    out->enabled = requested.load();
}
