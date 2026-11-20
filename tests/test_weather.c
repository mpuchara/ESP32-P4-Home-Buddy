// SPDX-License-Identifier: Apache-2.0
#include "hb_weather.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(e)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(e))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "line %d: %s\n", __LINE__, #e);                                        \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static char *read(const char *path, size_t *n)
{
    FILE *f = fopen(path, "rb");
    CHECK(f);
    char *body = malloc(8193);
    CHECK(body);
    *n = fread(body, 1, 8193, f);
    fclose(f);
    CHECK(*n <= 8192);
    return body;
}
int main(int argc, char **argv)
{
    char encoded[128];
    CHECK(hb_url_encode("Nowa Wies, PL", encoded, sizeof(encoded)));
    CHECK(!strcmp(encoded, "Nowa%20Wies%2C%20PL"));
    CHECK(hb_url_encode("\xC5\x81\xC3\xB3"
                        "dz",
                        encoded, sizeof(encoded)));
    CHECK(!strcmp(encoded, "%C5%81%C3%B3dz"));
    CHECK(!hb_url_encode("long", encoded, 3) && !encoded[0]);
    hb_places_t places = {0};
    const char *geo = "{\"results\":[{\"name\":\"Test\",\"country_code\":\"PL\",\"admin1\":"
                      "\"Region\",\"latitude\":52.2,\"longitude\":21.0}]}";
    CHECK(hb_places_parse(geo, strlen(geo), &places) && places.count == 1);
    CHECK(!strcmp(places.place[0].label, "Test, PL, Region") && places.place[0].latitude == 52.2);
    CHECK(hb_places_parse("{}", 2, &places) && places.count == 0);
    const char *err = "{\"error\":true}";
    CHECK(!hb_places_parse(err, strlen(err), &places));
    CHECK(!hb_places_parse("{}", 8193, &places));
    const char *invalid_geo = "{\"results\":[{\"name\":\"bad\",\"latitude\":100,\"longitude\":1}]}";
    CHECK(hb_places_parse(invalid_geo, strlen(invalid_geo), &places) && places.count == 0);
    hb_temperature_t value;
    const char *weather = "{\"current\":{\"time\":1792663200,\"temperature_2m\":12.5},\"current_"
                          "units\":{\"temperature_2m\":\"\\u00b0C\"}}";
    CHECK(hb_weather_parse(weather, strlen(weather), 1792663200, &value) && value.value == 12.5);
    CHECK(hb_weather_parse(weather, strlen(weather), 1792666800, &value));
    CHECK(!hb_weather_parse(weather, strlen(weather), 1792666801, &value));
    CHECK(!hb_weather_parse(weather, strlen(weather), 1792663199, &value));
    CHECK(!hb_weather_parse(weather, strlen(weather), 0, &value));
    CHECK(!hb_weather_parse("{}", 2, 1792663200, &value));
    const char *invalid[] = {"{\"current\":{\"time\":1792663200,\"temperature_2m\":null}}",
                             "{\"current\":{\"time\":1792663200,\"temperature_2m\":\"12.5\"}}",
                             "{\"current\":{\"time\":1792663200,\"temperature_2m\":999}}", "{}junk",
                             "[[[[[[[[[[0]]]]]]]]]]"};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        CHECK(!hb_weather_parse(invalid[i], strlen(invalid[i]), 1792663200, &value));
    if (argc == 3)
    {
        size_t n;
        char *body = read(argv[1], &n);
        CHECK(hb_places_parse(body, n, &places) && places.count);
        free(body);
        body = read(argv[2], &n);
        const char *current = strstr(body, "\"current\":");
        CHECK(current);
        const char *p = strstr(current, "\"time\":");
        CHECK(p);
        int64_t stamp = (int64_t)strtoll(p + 7, NULL, 10);
        CHECK(hb_weather_parse(body, n, stamp, &value));
        free(body);
        puts("live public API fixtures: actual geocoding/current-response parser passed");
    }
    puts("weather: URL UTF-8/geocoding/current Celsius/missing/stale/future/depth tests passed");
    return 0;
}
