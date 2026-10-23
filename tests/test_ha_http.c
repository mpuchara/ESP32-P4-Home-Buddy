// SPDX-License-Identifier: Apache-2.0
#include "cJSON.h"
#include "hb_ha_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    hb_ha_config c = {.schema = 1, .port = 8123, .enabled = true, .cooldown_seconds = 60};
    strcpy(c.host, "homeassistant.local");
    strcpy(c.secret, "12345678901234567890123456789012");
    char url[320], payload[1400];
    assert(hb_ha_config_valid(&c, true));
    assert(hb_ha_url(&c, false, url, sizeof(url)));
    assert(!strcmp(url,
                   "http://homeassistant.local:8123/api/webhook/12345678901234567890123456789012"));
    assert(hb_ha_url(&c, true, url, sizeof(url)) && !strstr(url, c.secret));
    strcpy(c.host, "user:pass@host");
    assert(!hb_ha_config_valid(&c, true));
    strcpy(c.host, "https://ha.example/api");
    assert(!hb_ha_config_valid(&c, true));
    strcpy(c.host, "ha.example");
    strcpy(c.secret, "short");
    assert(hb_ha_config_valid(&c, true));          // HA accepts nonempty IDs shorter than 32 chars.
    strcpy(c.secret, "-AbCdE_fGhIjKlMnO-pQrS-tU"); // synthetic, never a real credential
    assert(hb_ha_config_valid(&c, true));
    assert(hb_ha_url(&c, false, url, sizeof(url)));
    assert(strstr(url, "/api/webhook/-AbCdE_fGhIjKlMnO-pQrS-tU"));
    c.secret[0] = 0;
    assert(!hb_ha_config_valid(&c, true));
    assert(strstr(hb_ha_config_error(&c, true), "Webhook ID"));
    memset(c.secret, 'a', 128);
    c.secret[128] = 0;
    assert(hb_ha_config_valid(&c, true));
    c.secret[128] = 'a';
    c.secret[129] = 0;
    assert(!hb_ha_config_valid(&c, true));
    strcpy(c.secret, "1234567890123456789012345678901/");
    assert(!hb_ha_config_valid(&c, true));
    c.mode = HB_HA_REST;
    c.tls = true;
    strcpy(c.secret, "LONG.LIVED_TOKEN-not-ubox");
    assert(hb_ha_url(&c, false, url, sizeof(url)) &&
           !strcmp(url, "https://ha.example:8123/api/events/home_buddy_identity"));
    assert(!strstr(url, c.secret));
    strcat(c.secret, "\r\nInjected: header");
    assert(!hb_ha_config_valid(&c, true));
    assert(hb_ha_classify(401, false, false) == HB_HA_AUTH);
    assert(hb_ha_classify(403, false, false) == HB_HA_FORBIDDEN);
    assert(hb_ha_classify(0, false, true) == HB_HA_TLS);
    assert(hb_ha_classify(0, true, false) == HB_HA_TIMEOUT);
    assert(hb_ha_classify(0, false, false) == HB_HA_UNAVAILABLE);
    assert(hb_ha_classify(302, false, false) == HB_HA_REJECTED);
    assert(hb_ha_classify(500, false, false) == HB_HA_REJECTED);
    assert(hb_ha_classify(200, false, false) == HB_HA_DELIVERED);
    hb_id_event e = {.event_id = 1,
                     .session = 1,
                     .sequence = 1,
                     .frame_ms = 100,
                     .timestamp_ms = 200,
                     .person_id = 42,
                     .persistent_id = true,
                     .kind = HB_ID_EVENT_RECOGNIZED,
                     .state = HB_ID_KNOWN,
                     .confirmed = true};
    strcpy(e.person_name, "Maciej \"ł\"");
    assert(hb_ha_event_json("hb_test", &e, 300, false, payload, sizeof(payload)));
    assert(!strstr(payload, "person_name"));
    assert(hb_ha_event_json("hb_test", &e, 300, true, payload, sizeof(payload)));
    cJSON *j = cJSON_Parse(payload);
    assert(j);
    assert(!strcmp(cJSON_GetStringValue(cJSON_GetObjectItem(j, "person_name")), e.person_name));
    assert(cJSON_GetObjectItem(j, "person_id"));
    assert(!cJSON_GetObjectItem(j, "embedding"));
    cJSON_Delete(j);
    e.persistent_id = false;
    assert(hb_ha_event_json("hb_test", &e, 300, false, payload, sizeof(payload)));
    j = cJSON_Parse(payload);
    assert(!cJSON_GetObjectItem(j, "person_id"));
    cJSON_Delete(j);
    e.person_id = 0;
    assert(!hb_ha_event_json("hb_test", &e, 300, false, payload, sizeof(payload)));
    e.person_id = 42;
    assert(!hb_ha_event_json("hb_test", &e, 5101, true, payload, sizeof(payload)));
    assert(!hb_ha_event_json("hb_test", &e, 99, true, payload, sizeof(payload)));
    hb_ha_cooldowns cd = {0};
    assert(hb_ha_cooldown_ready(&cd, &e, 300, 60));
    hb_ha_cooldown_commit(&cd, &e, 300);
    assert(!hb_ha_cooldown_ready(&cd, &e, 1000, 60));
    e.encounter = 2;
    e.frame_ms = 61000;
    e.timestamp_ms = 61100;
    assert(hb_ha_cooldown_ready(&cd, &e, 61300, 60)); // same person, genuine later encounter
    e.kind = HB_ID_EVENT_UNKNOWN;
    e.state = HB_ID_UNKNOWN;
    e.person_id = 0;
    e.confirmed = false;
    assert(hb_ha_event_json("hb_test", &e, 61300, true, payload, sizeof(payload)));
    assert(!hb_ha_event_json("hb_test", &e, 61300, true, payload, 20));
    puts("HA HTTP URL/secret/privacy/status/expiry/cooldown checks passed");
    return 0;
}
