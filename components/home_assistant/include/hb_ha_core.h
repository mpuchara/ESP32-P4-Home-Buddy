// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "hb_identity_events.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum
    {
        HB_HA_WEBHOOK,
        HB_HA_REST
    } hb_ha_mode;
    typedef struct
    {
        uint32_t schema;
        char host[128], secret[513];
        uint16_t port;
        bool enabled, send_names, tls;
        hb_ha_mode mode;
        uint32_t cooldown_seconds;
    } hb_ha_config;
    typedef enum
    {
        HB_HA_DISABLED,
        HB_HA_HOST_OK,
        HB_HA_DELIVERED,
        HB_HA_UNAVAILABLE,
        HB_HA_AUTH,
        HB_HA_FORBIDDEN,
        HB_HA_INVALID,
        HB_HA_TLS,
        HB_HA_TIMEOUT,
        HB_HA_REJECTED
    } hb_ha_result;
    bool hb_ha_config_valid(const hb_ha_config *cfg, bool require_secret);
    bool hb_ha_url(const hb_ha_config *cfg, bool health_check, char *url, size_t size);
    hb_ha_result hb_ha_classify(int http_status, bool timeout, bool tls_failure);
    const char *hb_ha_result_text(hb_ha_result result);
    const char *hb_ha_event_type(hb_id_event_kind kind);
    bool hb_ha_event_json(const char *device, const hb_id_event *event, uint64_t now, bool names,
                          char *output, size_t capacity);
    typedef struct
    {
        struct
        {
            uint64_t person, at;
            hb_id_event_kind kind;
            bool used;
        } slots[HB_ID_PEOPLE + 4];
    } hb_ha_cooldowns;
    bool hb_ha_cooldown_ready(const hb_ha_cooldowns *, const hb_id_event *, uint64_t, uint32_t);
    void hb_ha_cooldown_commit(hb_ha_cooldowns *, const hb_id_event *, uint64_t);
#ifdef __cplusplus
}
#endif
