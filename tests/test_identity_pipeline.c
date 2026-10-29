// SPDX-License-Identifier: Apache-2.0
#include "hb_ha_core.h"
#include "hb_identity_lifecycle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "line %d: %s\n", __LINE__, #x);                                        \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
int main(void)
{
    hb_id_database *db = calloc(1, sizeof(*db));
    CHECK(db);
    hb_id_database_init(db, 123);
    CHECK(!hb_id_auto_ready(true, 0)); // Startup after a RAM-only reboot.
    uint64_t id = hb_id_create(db, "Synthetic person");
    hb_id_person *person = hb_id_find(db, id);
    hb_id_embedding embedding = {{1}};
    for (unsigned i = 0; i < HB_ID_MIN_SAMPLES; ++i)
        person->templates[i] = embedding;
    person->count = HB_ID_MIN_SAMPLES;
    CHECK(hb_id_auto_ready(true, 1));
    hb_id_policy policy = {0.7f, 0.12f, 0.6f, 3};
    hb_id_tracker tracker = {0};
    hb_id_tracker_reset(&tracker, 1);
    hb_id_match match = {0};
    for (unsigned sequence = 1; sequence <= 3; ++sequence)
    {
        match = hb_id_match_embedding(db, &embedding, &policy);
        CHECK(match.state == HB_ID_KNOWN && match.person_id == id);
        bool confirmed = hb_id_confirm(&tracker, match, 1, sequence, sequence * 1000,
                                       sequence * 1000 + 100, sequence * 1000 + 100, &policy);
        CHECK(confirmed == (sequence == 3));
    }
    hb_event_bus bus;
    hb_event_bus_init(&bus);
    hb_id_event event = {.session = 1,
                         .sequence = 3,
                         .frame_ms = 3000,
                         .timestamp_ms = 3100,
                         .person_id = id,
                         .confirmed = true,
                         .state = match.state,
                         .similarity = match.best,
                         .second = match.second,
                         .kind = HB_ID_EVENT_RECOGNIZED,
                         .source = HB_ID_SOURCE_FACE};
    snprintf(event.person_name, sizeof(event.person_name), "%s", person->name);
    CHECK(hb_event_push(&bus, event));
    CHECK(!hb_event_push(&bus, event)); // Stationary person cannot flood either consumer.
    hb_id_event greeting, http;
    CHECK(hb_event_pop(&bus, HB_EVENTS_GREETING, 1, 3200, &greeting));
    CHECK(hb_event_pop(&bus, HB_EVENTS_HA, 1, 3200, &http));
    CHECK(greeting.event_id == http.event_id && greeting.person_id == id);
    hb_ha_cooldowns cooldown = {0};
    CHECK(hb_ha_cooldown_ready(&cooldown, &http, 3200, 60));
    char payload[1400];
    CHECK(hb_ha_event_json("synthetic-device", &http, 3200, false, payload, sizeof(payload)));
    CHECK(!strstr(payload, "Synthetic person") && !strstr(payload, "embedding"));
    // Wi-Fi and HA outages exercise the actual classification/serialization policy.
    CHECK(hb_ha_classify(0, false, false) == HB_HA_UNAVAILABLE);
    CHECK(hb_ha_classify(0, true, false) == HB_HA_TIMEOUT);
    CHECK(hb_ha_classify(503, false, false) == HB_HA_REJECTED);
    CHECK(hb_ha_cooldown_ready(&cooldown, &http, 3200, 60)); // Failure did not commit delivery.
    CHECK(hb_ha_classify(204, false, false) == HB_HA_DELIVERED);
    hb_ha_cooldown_commit(&cooldown, &http, 3200);
    CHECK(!hb_ha_cooldown_ready(&cooldown, &http, 3300, 60));
    CHECK(!hb_ha_event_json("synthetic-device", &http, 9000, false, payload, sizeof(payload)));
    event.kind = HB_ID_EVENT_ENTERED;
    CHECK(hb_event_push(&bus, event));
    event.kind = HB_ID_EVENT_LEFT;
    event.sequence = 10;
    event.frame_ms = 63000;
    event.timestamp_ms = 63100;
    CHECK(hb_event_push(&bus, event));
    event.kind = HB_ID_EVENT_RECOGNIZED;
    event.sequence = 13;
    event.frame_ms = 66000;
    event.timestamp_ms = 66100;
    CHECK(hb_event_push(&bus, event)); // A genuine new encounter can notify again.
    CHECK(hb_ha_cooldown_ready(&cooldown, &event, 66200, 60));
    CHECK(hb_id_detection_failure(true, 2) == HB_ID_AMBIGUOUS);
    hb_id_tracker_reset(&tracker, 2); // OFF/cancel changes the epoch.
    CHECK(!hb_event_pop(&bus, HB_EVENTS_GREETING, 2, 66200, &greeting));
    CHECK(!hb_event_pop(&bus, HB_EVENTS_HA, 2, 66200, &http));
    CHECK(hb_id_delete(db, id));
    CHECK(hb_id_match_embedding(db, &embedding, &policy).state != HB_ID_KNOWN);
    free(db);
    puts("Synthetic identity pipeline: matching, confirmation, independent greeting/HA, privacy, "
         "outages, cooldown, new encounter and OFF passed");
    return 0;
}
