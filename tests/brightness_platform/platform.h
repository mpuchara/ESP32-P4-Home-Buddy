// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_INVALID_ARG 1
#define ESP_ERR_INVALID_STATE 2
#define ESP_ERR_NO_MEM 3
#define ESP_ERR_NVS_NOT_FOUND 4
#define ESP_ERR_NVS_INVALID_LENGTH 5
#define ESP_FAIL 6
#define ESP_LOGW(tag, ...) ((void)(tag))
typedef void *SemaphoreHandle_t;
#define portMAX_DELAY UINT32_MAX
#define pdPASS 1
#define pdMS_TO_TICKS(ms) (ms)
SemaphoreHandle_t xSemaphoreCreateMutex(void);
int xSemaphoreTake(SemaphoreHandle_t mutex, uint32_t timeout);
int xSemaphoreGive(SemaphoreHandle_t mutex);
int xTaskCreate(void (*task)(void *), const char *name, unsigned stack, void *arg,
                unsigned priority, void *handle);
void vTaskDelay(uint32_t ticks);
int64_t esp_timer_get_time(void);
void board_p4_set_brightness(uint8_t pwm);
typedef unsigned nvs_handle_t;
#define NVS_READONLY 0
#define NVS_READWRITE 1
esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle);
esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *value, size_t *size);
esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *value, size_t size);
esp_err_t nvs_commit(nvs_handle_t handle);
void nvs_close(nvs_handle_t handle);
