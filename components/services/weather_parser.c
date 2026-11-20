// SPDX-License-Identifier: Apache-2.0
#include "cJSON.h"
#include "hb_weather.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
bool hb_url_encode(const char *input, char *out, size_t cap)
{
    if (!input || !out || !cap)
        return false;
    size_t pos = 0;
    static const char hex[] = "0123456789ABCDEF";
    for (const unsigned char *p = (const unsigned char *)input; *p; ++p)
    {
        bool safe = (*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
                    (*p >= '0' && *p <= '9') || strchr("-_.~", *p);
        size_t need = safe ? 1 : 3;
        if (need >= cap - pos)
        {
            out[0] = 0;
            return false;
        }
        if (safe)
            out[pos++] = (char)*p;
        else
        {
            out[pos++] = '%';
            out[pos++] = hex[*p >> 4];
            out[pos++] = hex[*p & 15];
        }
    }
    out[pos] = 0;
    return true;
}
static cJSON *parse(const char *json, size_t n)
{
    if (!json || !n || n > 8192 || memchr(json, 0, n))
        return NULL;
    bool quoted = false, escaped = false;
    unsigned depth = 0;
    for (size_t i = 0; i < n; ++i)
    {
        char ch = json[i];
        if (quoted)
        {
            if (escaped)
                escaped = false;
            else if (ch == '\\')
                escaped = true;
            else if (ch == '"')
                quoted = false;
        }
        else if (ch == '"')
            quoted = true;
        else if (ch == '[' || ch == '{')
        {
            if (++depth > 8)
                return NULL;
        }
        else if (ch == ']' || ch == '}')
        {
            if (!depth)
                return NULL;
            --depth;
        }
    }
    if (depth || quoted)
        return NULL;
    const char *end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(json, n, &end, false);
    if (!root || !cJSON_IsObject(root))
    {
        cJSON_Delete(root);
        return NULL;
    }
    while (end < json + n && (*end == ' ' || *end == '\r' || *end == '\n' || *end == '\t'))
        ++end;
    if (end != json + n)
    {
        cJSON_Delete(root);
        return NULL;
    }
    return root;
}
static cJSON *item(cJSON *j, const char *key)
{
    return cJSON_GetObjectItemCaseSensitive(j, key);
}
static bool coordinate(cJSON *j, const char *key, double limit, double *out)
{
    cJSON *v = item(j, key);
    if (!cJSON_IsNumber(v) || !isfinite(v->valuedouble) || fabs(v->valuedouble) > limit)
        return false;
    *out = v->valuedouble;
    return true;
}
bool hb_places_parse(const char *json, size_t n, hb_places_t *out)
{
    if (!out)
        return false;
    cJSON *root = parse(json, n);
    if (!root)
        return false;
    out->count = 0;
    cJSON *list = item(root, "results");
    bool valid = !list || cJSON_IsArray(list);
    if (item(root, "error"))
        valid = false;
    if (valid)
        for (cJSON *j = list ? list->child : NULL; j && out->count < 5; j = j->next)
        {
            hb_place_t p;
            cJSON *name = item(j, "name"), *country = item(j, "country_code"),
                  *admin = item(j, "admin1");
            if (!cJSON_IsString(name) || !name->valuestring[0] ||
                !coordinate(j, "latitude", 90, &p.latitude) ||
                !coordinate(j, "longitude", 180, &p.longitude))
                continue;
            int len = snprintf(p.label, sizeof(p.label), "%s, %s%s%s", name->valuestring,
                               cJSON_IsString(country) ? country->valuestring : "?",
                               cJSON_IsString(admin) ? ", " : "",
                               cJSON_IsString(admin) ? admin->valuestring : "");
            if (len < 0 || len >= (int)sizeof(p.label) || strpbrk(p.label, "\r\n"))
                continue;
            out->place[out->count++] = p;
        }
    cJSON_Delete(root);
    return valid;
}
bool hb_weather_parse(const char *json, size_t n, int64_t now, hb_temperature_t *out)
{
    if (!out)
        return false;
    cJSON *root = parse(json, n);
    if (!root)
        return false;
    cJSON *current = item(root, "current"), *units = item(root, "current_units");
    cJSON *value = item(current, "temperature_2m"), *stamp = item(current, "time");
    cJSON *unit = item(units, "temperature_2m");
    bool valid = cJSON_IsNumber(value) && isfinite(value->valuedouble) &&
                 value->valuedouble >= -80 && value->valuedouble <= 80 && cJSON_IsNumber(stamp) &&
                 isfinite(stamp->valuedouble) && stamp->valuedouble >= 1577836800 &&
                 stamp->valuedouble < 4102444800.0 &&
                 floor(stamp->valuedouble) == stamp->valuedouble && cJSON_IsString(unit) &&
                 !strcmp(unit->valuestring, "\xC2\xB0"
                                            "C");
    if (valid)
        valid = hb_fresh((int64_t)stamp->valuedouble, now, 3600);
    if (valid)
        *out = (hb_temperature_t){true, value->valuedouble, (int64_t)stamp->valuedouble};
    cJSON_Delete(root);
    return valid;
}
