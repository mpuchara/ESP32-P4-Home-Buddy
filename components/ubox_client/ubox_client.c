// SPDX-License-Identifier: Apache-2.0
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hb_config.h"
#include "hb_dashboard.h"
#include "hb_image.h"
#include "hb_ubox.h"
#include "hb_ui.h"
#include <stdlib.h>
#include <string.h>
static char etag[128], candidate[128];
static esp_err_t event(esp_http_client_event_t *e)
{
    if (e->event_id == HTTP_EVENT_ON_HEADER && e->header_key && e->header_value &&
        strcasecmp(e->header_key, "ETag") == 0 && strlen(e->header_value) < sizeof(candidate))
        strcpy(candidate, e->header_value);
    return ESP_OK;
}
static void worker(void *arg)
{
    (void)arg;
    unsigned failures = 0;
    char previous[512] = "";
    for (;;)
    {
        hb_config_t c;
        hb_config_get(&c);
        if (!c.ubox_host[0] || !c.ubox_token[0])
        {
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }
        char url[256], source[512];
        // Host is a hostname/IP only. No path, credentials or token in a URL.
        if (strpbrk(c.ubox_host, "/\\@?# \r\n"))
        {
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        snprintf(url, sizeof(url), "http://%s:%u/snapshot.jpg", c.ubox_host, c.ubox_port);
        snprintf(source, sizeof(source), "%s|%s", url, c.ubox_token);
        if (strcmp(source, previous))
        {
            etag[0] = 0;
            strcpy(previous, source);
        }
        if (hb_ui_image_is_test())
            etag[0] = 0; // Restore the real cached image after an explicit synthetic UI test.
        candidate[0] = 0;
        esp_http_client_config_t cfg = {.url = url,
                                        .timeout_ms = 8000,
                                        .disable_auto_redirect = true,
                                        .event_handler = event,
                                        .buffer_size = 2048};
        esp_http_client_handle_t client = esp_http_client_init(&cfg);
        bool accepted = false;
        int status = 0;
        uint8_t *body = NULL;
        uint16_t *frame = NULL;
        const char *phase = "connect";
        int64_t expected = -2;
        size_t received = 0;
        bool complete = false;
        esp_err_t decode_error = ESP_OK;
        if (!client)
            goto finish;
        int64_t deadline = esp_timer_get_time() + 20000000;
        esp_http_client_set_header(client, "X-UBox-Token", c.ubox_token);
        if (etag[0])
            esp_http_client_set_header(client, "If-None-Match", etag);
        if (esp_http_client_open(client, 0) != ESP_OK)
            goto cleanup;
        int64_t length = esp_http_client_fetch_headers(client);
        expected = length;
        if (esp_http_client_is_chunked_response(client))
            length = -1;
        status = esp_http_client_get_status_code(client);
        if (status == 304 && etag[0])
        {
            accepted = true;
            goto cleanup;
        }
        hb_transfer_t transfer;
        phase = "headers/size";
        if (!hb_transfer_begin(&transfer, status, length))
            goto cleanup;
        body = heap_caps_malloc(HB_JPEG_LIMIT, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        phase = "memory";
        if (!body)
            goto cleanup;
        while (transfer.received < HB_JPEG_LIMIT)
        {
            int64_t remaining = (deadline - esp_timer_get_time()) / 1000;
            if (remaining <= 0)
            {
                transfer.failed = true;
                break;
            }
            esp_http_client_set_timeout_ms(client, remaining < 8000 ? (int)remaining : 8000);
            int got = esp_http_client_read(client, (char *)body + transfer.received,
                                           HB_JPEG_LIMIT - transfer.received > 4096
                                               ? 4096
                                               : HB_JPEG_LIMIT - transfer.received);
            if (got < 0)
            {
                transfer.failed = true;
                break;
            }
            if (!got)
                break;
            if (!hb_transfer_append(&transfer, got))
                break;
        }
        received = transfer.received;
        complete = esp_http_client_is_complete_data_received(client);
        phase = "partial";
        if (!complete || !hb_transfer_complete(&transfer))
            goto cleanup;
        phase = "decode";
        decode_error = hb_image_decode(body, transfer.received, &frame);
        if (decode_error != ESP_OK)
            goto cleanup;
        phase = "queue";
        if (hb_ui_publish_image(frame))
        {
            frame = NULL;
            hb_etag_commit(etag, sizeof(etag), candidate, true, true);
            accepted = true;
        }
    cleanup:
        free(frame);
        free(body);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
    finish:
        failures = accepted ? 0 : failures + 1;
        hb_ui_diagnostic("UBox", accepted ? "OK"
                                          : (status == 401   ? "Błędny token"
                                             : status == 503 ? "Brak migawki"
                                                             : "Błąd pobierania"));
        ESP_LOGI("ubox",
                 "cached snapshot HTTP=%d accepted=%d retries=%u phase=%s "
                 "expected=%lld received=%u complete=%d decode=%s",
                 status, accepted, failures, phase, (long long)expected, (unsigned)received,
                 complete, esp_err_to_name(decode_error));
        vTaskDelay(pdMS_TO_TICKS(hb_retry_seconds(failures, c.poll_seconds) * 1000));
    }
}
esp_err_t hb_ubox_start(void)
{
    return xTaskCreate(worker, "snapshot", 8192, NULL, 2, NULL) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
