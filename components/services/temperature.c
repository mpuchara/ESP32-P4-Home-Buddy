// SPDX-License-Identifier: Apache-2.0
#include "cJSON.h"
#include "hb_dashboard.h"
#include <math.h>
#include <string.h>
bool hb_temperature_parse(const char *data, size_t n, int64_t now, uint32_t age,
                          hb_temperature_t *out)
{
    if (!data || !out || !n || n >= 768 || memchr(data, 0, n))
        return false;
    // The wire schema is flat. Reject nesting before invoking a recursive parser.
    bool quoted = false, escaped = false;
    unsigned depth = 0;
    for (size_t i = 0; i < n; ++i)
    {
        char ch = data[i];
        if (quoted)
        {
            if (escaped)
                escaped = false;
            else if (ch == 92)
                escaped = true;
            else if (ch == '"')
                quoted = false;
            continue;
        }
        if (ch == '"')
            quoted = true;
        else if (ch == '[' || ch == ']')
            return false;
        else if (ch == '{')
        {
            if (++depth > 1)
                return false;
        }
        else if (ch == '}')
        {
            if (!depth)
                return false;
            --depth;
        }
    }
    if (quoted || depth)
        return false;
    char text[768];
    memcpy(text, data, n);
    text[n] = 0;
    const char *end = NULL;
    cJSON *j = cJSON_ParseWithOpts(text, &end, true);
    if (!j)
        return false;
    if (!cJSON_IsObject(j))
    {
        cJSON_Delete(j);
        return false;
    }
    for (cJSON *a = j->child; a; a = a->next)
        for (cJSON *b = a->next; b; b = b->next)
            if (!strcmp(a->string, b->string))
            {
                cJSON_Delete(j);
                return false;
            }
    cJSON *a = cJSON_GetObjectItemCaseSensitive(j, "available");
    cJSON *v = cJSON_GetObjectItemCaseSensitive(j, "temperature");
    cJSON *u = cJSON_GetObjectItemCaseSensitive(j, "unit");
    cJSON *t = cJSON_GetObjectItemCaseSensitive(j, "timestamp");
    bool ok = false;
    if (cJSON_IsBool(a) && cJSON_IsFalse(a))
    {
        *out = (hb_temperature_t){0};
        ok = true;
    }
    else if (cJSON_IsTrue(a) && cJSON_IsNumber(v) && isfinite(v->valuedouble) &&
             v->valuedouble >= -80 && v->valuedouble <= 80 && cJSON_IsString(u) &&
             !strcmp(u->valuestring, "C") && cJSON_IsString(t))
    {
        int64_t epoch;
        if (hb_timestamp(t->valuestring, &epoch) && hb_fresh(epoch, now, age))
        {
            *out = (hb_temperature_t){true, v->valuedouble, epoch};
            ok = true;
        }
    }
    cJSON_Delete(j);
    return ok;
}
