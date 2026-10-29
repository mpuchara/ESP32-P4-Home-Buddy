// SPDX-License-Identifier: Apache-2.0
#include "cJSON.h"
#include "hb_ha_core.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
const char *hb_ha_event_type(hb_id_event_kind k)
{
    static const char *types[] = {"recognized", "unknown", "ambiguous", "presence_enter",
                                  "presence_leave"};
    return (unsigned)k < 5 ? types[k] : "invalid";
}
static bool serialize(cJSON *j, char *out, size_t n)
{
    bool ok = j && out && n <= INT32_MAX && cJSON_PrintPreallocated(j, out, (int)n, false);
    cJSON_Delete(j);
    return ok;
}
bool hb_ha_event_json(const char *device, const hb_id_event *e, uint64_t now, bool names, char *out,
                      size_t n)
{
    if (!device || !e || e->frame_ms > now || now - e->frame_ms > 5000 || (unsigned)e->kind >= 5 ||
        e->source != HB_ID_SOURCE_FACE ||
        (e->kind == HB_ID_EVENT_RECOGNIZED &&
         (!e->confirmed || !e->person_id || e->state != HB_ID_KNOWN)) ||
        !memchr(e->person_name, 0, HB_ID_NAME))
        return false;
    cJSON *j = cJSON_CreateObject();
    if (!j)
        return false;
    char id[100];
    snprintf(id, sizeof(id), "%s-%016" PRIx64 "-%" PRIu64, device, e->boot_id, e->event_id);
    cJSON_AddNumberToObject(j, "schema", 1);
    cJSON_AddStringToObject(j, "device", device);
    cJSON_AddBoolToObject(j, "person_id_persistent", e->persistent_id && e->person_id);
    cJSON_AddStringToObject(j, "event_type", hb_ha_event_type(e->kind));
    cJSON_AddStringToObject(j, "event_id", id);
    if (e->persistent_id && e->person_id)
    {
        snprintf(id, sizeof(id), "%016" PRIx64, e->person_id);
        cJSON_AddStringToObject(j, "person_id", id);
    }
    if (e->person_id && names)
        cJSON_AddStringToObject(j, "person_name", e->person_name);
    cJSON_AddStringToObject(j, "method", "face");
    cJSON_AddStringToObject(j, "status",
                            e->confirmed                  ? "confirmed"
                            : e->state == HB_ID_AMBIGUOUS ? "ambiguous"
                            : e->state == HB_ID_UNKNOWN   ? "unknown"
                                                          : "unavailable");
    // Epoch milliseconds are omitted until SNTP time is available.
    if (e->unix_ms > 0)
    {
        cJSON_AddNumberToObject(j, "timestamp_unix_ms", (double)e->unix_ms);
        time_t seconds = (time_t)(e->unix_ms / 1000);
        struct tm *utc = gmtime(&seconds);
        char timestamp[32];
        if (utc && strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", utc))
            cJSON_AddStringToObject(j, "timestamp", timestamp);
        else
            cJSON_AddNullToObject(j, "timestamp");
    }
    else
    {
        cJSON_AddNullToObject(j, "timestamp_unix_ms");
        cJSON_AddNullToObject(j, "timestamp");
    }
    cJSON_AddNumberToObject(j, "uptime_ms", (double)e->timestamp_ms);
    cJSON_AddNumberToObject(j, "source_age_ms", (double)(now - e->frame_ms));
    cJSON_AddNumberToObject(j, "session", e->session);
    cJSON_AddNumberToObject(j, "encounter", e->encounter);
    cJSON_AddNumberToObject(j, "similarity", e->similarity);
    cJSON_AddNumberToObject(j, "second_similarity", e->second);
    return serialize(j, out, n);
}
