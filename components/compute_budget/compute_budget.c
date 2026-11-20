// SPDX-License-Identifier: Apache-2.0
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "hb_compute_budget.h"
static StaticSemaphore_t storage;
static SemaphoreHandle_t mutex;
void hb_compute_start(void)
{
    mutex = xSemaphoreCreateMutexStatic(&storage);
}
bool hb_compute_enter(uint32_t timeout_ms)
{
    return mutex && xSemaphoreTake(mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}
void hb_compute_leave(void)
{
    xSemaphoreGive(mutex);
}
