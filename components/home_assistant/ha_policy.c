// SPDX-License-Identifier: Apache-2.0
#include "hb_ha_core.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
const char *hb_ha_config_error(const hb_ha_config *c, bool require_secret)
{
    if (!c || c->schema != 1 || !c->port || (unsigned)c->mode > HB_HA_REST ||
        c->cooldown_seconds > 3600 || !memchr(c->host, 0, sizeof(c->host)) ||
        !memchr(c->secret, 0, sizeof(c->secret)))
        return "Nieprawidłowa konfiguracja HA (port, tryb lub długość pól).";
    if (!c->host[0])
        return !require_secret && !c->enabled && !c->secret[0]
                   ? NULL
                   : "Host HA jest pusty. Wpisz adres IP lub nazwę hosta.";
    // Deliberately accept only an explicit host, never userinfo/path/query/fragment.
    size_t n = strlen(c->host);
    bool ipv6 = c->host[0] == '[' && n > 2 && c->host[n - 1] == ']';
    for (size_t i = 0; i < n; i++)
    {
        unsigned char ch = (unsigned char)c->host[i];
        if (ch >= 128)
            return "Host HA: użyj adresu IP lub nazwy bez polskich znaków.";
        if (ipv6)
        {
            if (i && i < n - 1 && !isxdigit(ch) && ch != ':' && ch != '.')
                return "Host IPv6: użyj adresu w nawiasach [adres], bez portu.";
        }
        else if (!isalnum(ch) && ch != '-' && ch != '.')
            return "Host HA: sam adres IP lub nazwa, bez http://, portu i ścieżki.";
    }
    if (!require_secret && !c->enabled && !c->secret[0])
        return NULL;
    n = strlen(c->secret);
    if (c->mode == HB_HA_WEBHOOK && (n < 1 || n > 128))
        return "Webhook ID: wymagane 1-128 znaków. Wpisz sam ID, nie URL.";
    if (c->mode == HB_HA_REST && (n < 16 || n > 512))
        return "Token REST: wymagane 16-512 znaków, osobny token HA.";
    for (size_t i = 0; i < n; i++)
    {
        unsigned char ch = (unsigned char)c->secret[i];
        if (c->mode == HB_HA_WEBHOOK)
        {
            if (!isalnum(ch) && ch != '_' && ch != '-')
                return "Webhook ID: dozwolone litery, cyfry, podkreślenie i myślnik.";
        }
        else if (ch < 33 || ch > 126)
            return "Token REST zawiera spację lub niedozwolony znak.";
    }
    return NULL;
}
bool hb_ha_config_valid(const hb_ha_config *c, bool require_secret)
{
    return hb_ha_config_error(c, require_secret) == NULL;
}
bool hb_ha_url(const hb_ha_config *c, bool health, char *out, size_t n)
{
    if (!out || !hb_ha_config_valid(c, true))
        return false;
    const char *path =
        health ? (c->mode == HB_HA_REST ? "/api/" : "/")
               : (c->mode == HB_HA_REST ? "/api/events/home_buddy_identity" : "/api/webhook/");
    int written = snprintf(out, n, "%s://%s:%u%s%s", c->tls ? "https" : "http", c->host, c->port,
                           path, !health && c->mode == HB_HA_WEBHOOK ? c->secret : "");
    return written >= 0 && (size_t)written < n;
}
hb_ha_result hb_ha_classify(int status, bool timeout, bool tls)
{
    if (tls)
        return HB_HA_TLS;
    if (timeout)
        return HB_HA_TIMEOUT;
    if (status == 401)
        return HB_HA_AUTH;
    if (status == 403)
        return HB_HA_FORBIDDEN;
    if (status >= 200 && status < 300)
        return HB_HA_DELIVERED;
    return status > 0 ? HB_HA_REJECTED : HB_HA_UNAVAILABLE;
}
const char *hb_ha_result_text(hb_ha_result r)
{
    static const char *text[] = {"Wyłączone",
                                 "Host/API OK; publikacja niezweryfikowana",
                                 "HTTP POST 2xx; sprawdź ślad automatyzacji HA",
                                 "Host niedostępny",
                                 "Błąd uwierzytelnienia (401)",
                                 "Brak uprawnień (403)",
                                 "Nieprawidłowy host / sekret / konfiguracja",
                                 "Błąd TLS / certyfikatu",
                                 "Przekroczony czas połączenia",
                                 "Żądanie odrzucone przez HA"};
    return (unsigned)r < sizeof(text) / sizeof(text[0]) ? text[r] : "Błąd";
}
static unsigned slot(const hb_ha_cooldowns *c, const hb_id_event *e)
{
    unsigned oldest = 0;
    for (unsigned i = 0; i < HB_ID_PEOPLE + 4; i++)
    {
        if (c->slots[i].used && c->slots[i].person == e->person_id && c->slots[i].kind == e->kind)
            return i;
        if (!c->slots[i].used)
            oldest = i;
        else if (c->slots[oldest].used && c->slots[i].at < c->slots[oldest].at)
            oldest = i;
    }
    return oldest;
}
bool hb_ha_cooldown_ready(const hb_ha_cooldowns *c, const hb_id_event *e, uint64_t now,
                          uint32_t seconds)
{
    if (!c || !e || !e->frame_ms || e->frame_ms > now || now - e->frame_ms > 5000)
        return false;
    unsigned i = slot(c, e);
    return !c->slots[i].used || c->slots[i].person != e->person_id || c->slots[i].kind != e->kind ||
           (now >= c->slots[i].at && now - c->slots[i].at >= (uint64_t)seconds * 1000);
}
void hb_ha_cooldown_commit(hb_ha_cooldowns *c, const hb_id_event *e, uint64_t now)
{
    unsigned i = slot(c, e);
    c->slots[i].person = e->person_id;
    c->slots[i].at = now;
    c->slots[i].kind = e->kind;
    c->slots[i].used = true;
}
