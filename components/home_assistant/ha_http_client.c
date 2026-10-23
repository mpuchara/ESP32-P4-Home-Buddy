// SPDX-License-Identifier: Apache-2.0
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hb_ha.h"
#include "hb_identity.h"
#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
static portMUX_TYPE status_lock = portMUX_INITIALIZER_UNLOCKED;
static hb_ha_status status;
static atomic_uint test_requests;
static bool started;
static uint64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}
void hb_ha_get_status(hb_ha_status *out)
{
    portENTER_CRITICAL(&status_lock);
    *out = status;
    portEXIT_CRITICAL(&status_lock);
}
void hb_ha_test(bool post)
{
    atomic_fetch_or(&test_requests, post ? 2U : 1U);
}
static void result(hb_ha_result r, int code, bool event)
{
    portENTER_CRITICAL(&status_lock);
    status.result = r;
    status.http_status = code;
    if (event && r == HB_HA_DELIVERED)
        status.sent++;
    if (r >= HB_HA_UNAVAILABLE)
        status.failed++;
    portEXIT_CRITICAL(&status_lock);
}
static hb_ha_result request(const hb_ha_config *cfg, bool health, const char *payload)
{
    char url[320], authorization[528];
    if (!hb_ha_url(cfg, health, url, sizeof(url)))
    {
        result(HB_HA_INVALID, 0, false);
        return HB_HA_INVALID;
    }
    esp_http_client_config_t options = {.url = url,
                                        .timeout_ms = 1500,
                                        .disable_auto_redirect = true,
                                        .buffer_size = 1024,
                                        .buffer_size_tx = 1024,
                                        .crt_bundle_attach =
                                            cfg->tls ? esp_crt_bundle_attach : NULL};
    esp_http_client_handle_t client = esp_http_client_init(&options);
    if (!client)
    {
        memset(url, 0, sizeof(url));
        result(HB_HA_UNAVAILABLE, 0, false);
        return HB_HA_UNAVAILABLE;
    }
    if (cfg->mode == HB_HA_REST)
    {
        snprintf(authorization, sizeof(authorization), "Bearer %s", cfg->secret);
        esp_http_client_set_header(client, "Authorization", authorization);
    }
    esp_http_client_set_method(client, health ? HTTP_METHOD_GET : HTTP_METHOD_POST);
    if (!health)
    {
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, payload, (int)strlen(payload));
    }
    esp_err_t e = esp_http_client_perform(client);
    int code = e == ESP_OK ? esp_http_client_get_status_code(client) : 0;
    bool timeout =
        e != ESP_OK && (e == ESP_ERR_TIMEOUT || esp_http_client_get_errno(client) == ETIMEDOUT);
    int tls_error = 0, tls_flags = 0;
    esp_http_client_get_and_clear_last_tls_error(client, &tls_error, &tls_flags);
    hb_ha_result r = hb_ha_classify(code, timeout, e != ESP_OK && (tls_error || tls_flags));
    esp_http_client_cleanup(client);
    memset(url, 0, sizeof(url));
    memset(authorization, 0, sizeof(authorization));
    if (health && r == HB_HA_DELIVERED)
        r = HB_HA_HOST_OK;
    result(r, code, !health);
    return r;
}
static void drain(void)
{
    hb_id_event e;
    while (hb_identity_event_read(HB_EVENTS_HA, &e))
    {
        portENTER_CRITICAL(&status_lock);
        status.expired++;
        portEXIT_CRITICAL(&status_lock);
    }
}
static void worker(void *unused)
{
    (void)unused;
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_EFUSE_FACTORY);
    char device[32];
    snprintf(device, sizeof(device), "hb_%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3],
             mac[4], mac[5]);
    hb_ha_config active = {0};
    hb_ha_cooldowns cooldown = {0};
    uint64_t retry_at = 0;
    bool reachable = false;
    bool manual_test = false;
    uint64_t diagnostic_boot = ((uint64_t)esp_random() << 32) | esp_random(),
             diagnostic_sequence = 0;
    for (;;)
    {
        hb_ha_config cfg;
        hb_ha_config_get(&cfg);
        if (memcmp(&cfg, &active, sizeof(cfg)))
        {
            active = cfg;
            reachable = false;
            retry_at = 0;
            drain();
            manual_test = false;
        }
        memset(&cfg, 0, sizeof(cfg));
        unsigned tests = atomic_exchange(&test_requests, 0);
        if (tests)
        {
            manual_test = true;
            if (tests & 1)
                request(&active, true, NULL);
            if (tests & 2)
            {
                char payload[256];
                snprintf(
                    payload, sizeof(payload),
                    "{\"schema\":1,\"device\":\"%s\",\"event_type\":\"connection_test\",\"event_"
                    "id\":\"test-%016llx-%llu\",\"method\":\"diagnostic\",\"status\":\"test\"}",
                    device, (unsigned long long)diagnostic_boot,
                    (unsigned long long)++diagnostic_sequence);
                request(&active, false, payload);
            }
        }
        if (!active.enabled)
        {
            drain();
            if (!manual_test)
                result(HB_HA_DISABLED, 0, false);
        }
        else if (!hb_ha_config_valid(&active, true))
        {
            drain();
            result(HB_HA_INVALID, 0, false);
        }
        else if (!reachable)
        {
            drain();
            if (now_ms() >= retry_at)
            {
                reachable = request(&active, true, NULL) == HB_HA_HOST_OK;
                retry_at = now_ms() + 5000;
                drain();
            }
        }
        else
        {
            hb_id_event e;
            while (hb_identity_event_read(HB_EVENTS_HA, &e))
            {
                uint64_t now = now_ms();
                char payload[1400];
                if (!hb_identity_event_current(&e) ||
                    !hb_ha_cooldown_ready(&cooldown, &e, now, active.cooldown_seconds) ||
                    !hb_ha_event_json(device, &e, now, active.send_names, payload, sizeof(payload)))
                    continue;
                // Revalidate the cancellation epoch after serialization, just before network I/O.
                if (!hb_identity_event_current(&e))
                    continue;
                hb_ha_result r = request(&active, false, payload);
                if (r == HB_HA_DELIVERED)
                    hb_ha_cooldown_commit(&cooldown, &e, now);
                else
                {
                    reachable = false;
                    retry_at = now_ms() + 5000;
                    drain();
                    break;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
esp_err_t hb_ha_start(void)
{
    if (started)
        return ESP_OK;
    extern esp_err_t hb_ha_config_load(void);
    esp_err_t e = hb_ha_config_load();
    if (e)
        result(HB_HA_INVALID, 0, false);
    // ESP HTTP debug traces can contain the webhook path. Do not log requests/secrets.
    esp_log_level_set("HTTP_CLIENT", ESP_LOG_NONE);
    if (xTaskCreate(worker, "ha_http", 8192, NULL, 1, NULL) != pdPASS)
        return ESP_ERR_NO_MEM;
    started = true;
    return ESP_OK;
}
