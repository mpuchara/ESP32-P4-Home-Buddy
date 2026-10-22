// SPDX-License-Identifier: Apache-2.0
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hb_config.h"
#include "hb_services.h"
#include "hb_ui.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static atomic_bool synchronized;
static QueueHandle_t searches;
bool hb_time_valid(void)
{
    return atomic_load(&synchronized);
}
static void time_sync(struct timeval *tv)
{
    (void)tv;
    atomic_store(&synchronized, true);
}
bool hb_weather_search(const char *query)
{
    if (!searches || !query || strlen(query) < 2 || strlen(query) >= 128)
        return false;
    char copy[128] = "";
    strcpy(copy, query);
    return xQueueOverwrite(searches, copy) == pdTRUE;
}
static char *get(const char *url, size_t *length)
{
    esp_http_client_config_t cfg = {.url = url,
                                    .timeout_ms = 8000,
                                    .disable_auto_redirect = true,
                                    .crt_bundle_attach = esp_crt_bundle_attach,
                                    .buffer_size = 2048};
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client)
        return NULL;
    char *body = NULL;
    bool good = false;
    size_t used = 0;
    int status = 0;
    int64_t deadline = esp_timer_get_time() + 20000000;
    if (esp_http_client_open(client, 0) != ESP_OK)
        goto done;
    int64_t expected = esp_http_client_fetch_headers(client);
    status = esp_http_client_get_status_code(client);
    if (status != 200 || expected > 8192)
        goto done;
    body = heap_caps_malloc(8193, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!body)
        goto done;
    while (used < 8192)
    {
        int64_t remaining = (deadline - esp_timer_get_time()) / 1000;
        if (remaining <= 0)
            goto done;
        esp_http_client_set_timeout_ms(client, remaining < 8000 ? (int)remaining : 8000);
        int got =
            esp_http_client_read(client, body + used, 8192 - used > 2048 ? 2048 : 8192 - used);
        if (got < 0)
            goto done;
        if (!got)
            break;
        used += (size_t)got;
    }
    good = used && esp_http_client_is_complete_data_received(client);
    if (good)
    {
        body[used] = 0;
        *length = used;
    }
done:
    ESP_LOGI("weather", "HTTPS status=%d body=%u complete=%d", status, (unsigned)used, good);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    if (!good)
    {
        free(body);
        body = NULL;
    }
    return body;
}
static void geocode(const char *query)
{
    if (!hb_time_valid())
    {
        hb_ui_diagnostic("Pogoda", "Poczekaj na synchronizację czasu");
        return;
    }
    char encoded[384], url[640];
    if (!hb_url_encode(query, encoded, sizeof(encoded)))
        return;
    snprintf(
        url, sizeof(url),
        "https://geocoding-api.open-meteo.com/v1/search?name=%s&count=5&language=pl&format=json",
        encoded);
    size_t n = 0;
    char *body = get(url, &n);
    hb_places_t *places = calloc(1, sizeof(*places));
    bool good = body && places && hb_places_parse(body, n, places);
    if (places)
    {
        snprintf(places->query, sizeof(places->query), "%s", query);
        if (good)
            hb_ui_weather_places(places);
    }
    hb_ui_diagnostic("Pogoda",
                     good ? (places->count ? "Wybierz miejscowość z listy" : "Brak miejscowości")
                          : "Błąd wyszukiwania / sieci");
    free(places);
    free(body);
}
static void worker(void *arg)
{
    (void)arg;
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb(time_sync);
    esp_sntp_init();
    hb_weather_config_t previous = {0};
    uint32_t next = 0;
    unsigned failures = 0;
    for (;;)
    {
        char query[128];
        if (xQueueReceive(searches, query, pdMS_TO_TICKS(1000)) == pdTRUE)
            geocode(query);
        hb_weather_config_t c;
        hb_config_weather_get(&c);
        if (strcmp(previous.city, c.city) || previous.has_location != c.has_location ||
            previous.latitude != c.latitude || previous.longitude != c.longitude)
        {
            previous = c;
            next = 0;
            failures = 0;
            hb_ui_temperature(false, 0, 0, 3600);
        }
        uint32_t now = xTaskGetTickCount();
        if (!c.has_location || !hb_time_valid() || (int32_t)(now - next) < 0)
            continue;
        char url[256];
        snprintf(url, sizeof(url),
                 "https://api.open-meteo.com/v1/"
                 "forecast?latitude=%.6f&longitude=%.6f&current=temperature_2m&temperature_unit="
                 "celsius&timeformat=unixtime&timezone=GMT&forecast_days=1",
                 c.latitude, c.longitude);
        size_t n = 0;
        char *body = get(url, &n);
        hb_temperature_t value;
        bool good = body && hb_weather_parse(body, n, time(NULL), &value);
        hb_weather_config_t latest;
        hb_config_weather_get(&latest);
        bool same =
            latest.has_location && latest.latitude == c.latitude && latest.longitude == c.longitude;
        ESP_LOGI("weather", "Current response accepted=%d same_location=%d", good, same);
        if (good && same)
            hb_ui_temperature(true, value.value, value.timestamp, 3600);
        if (same)
            hb_ui_diagnostic("Pogoda", good ? "Open-Meteo: OK" : "Błąd Open-Meteo; ponowię");
        free(body);
        failures = good ? 0 : (failures < 7 ? failures + 1 : 7);
        next = xTaskGetTickCount() + pdMS_TO_TICKS(hb_retry_seconds(failures, 600) * 1000);
    }
}
esp_err_t hb_services_start(void)
{
    esp_netif_init();
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    searches = xQueueCreate(1, 128);
    if (!searches)
        return ESP_ERR_NO_MEM;
    return xTaskCreate(worker, "weather_time", 12288, NULL, 2, NULL) == pdPASS ? ESP_OK
                                                                               : ESP_ERR_NO_MEM;
}
