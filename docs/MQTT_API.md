# Native Home Assistant identity MQTT, 0.6.0-dev

Open Ustawienia -> Home Assistant. Configure broker hostname/IP, port, username,
password, enable MQTT, identity publishing, TLS and cooldown seconds. MQTT starts
disabled; identity publication also requires explicit enablement. Fields scroll
above the existing keyboard. No mandatory administrator PIN is introduced.
Settings persist in a separate hb_mqtt NVS namespace without changing existing
Wi-Fi/UBox/weather records. Development NVS is plaintext; the menu warns about
provisioning. Passwords are masked and are not logged. The existing Open-Meteo
temperature path is unchanged.

ESP-MQTT uses MQTT 5, authenticated CONNECT, optional TLS with the IDF CA bundle
and normal hostname verification. Never disable certificate verification. Use a
broker certificate chained to a bundled trusted CA and synchronized device clock.
A private/self-signed CA import UI, client certificates and MQTT 3 fallback are not
implemented. Configure a dedicated broker account with ACLs restricted to this
unit's state/events/availability and its Discovery config topics. Broker ACL and
TLS/authentication acceptance must be tested on actual hardware after migration.

## Topics and stable identifiers

Device ID is hb_<factory P4 MAC hex>, stable across reboots. Unique Discovery IDs
combine this device ID and entity key. The six default entity IDs requested via
current HA default_entity_id are:

- event.home_buddy_recognition
- sensor.home_buddy_last_person
- sensor.home_buddy_identity_status
- sensor.home_buddy_identity_method
- binary_sensor.home_buddy_face_detected
- binary_sensor.home_buddy_identity_available

Home Assistant may add a suffix if an entity ID is already in use; user-renamed IDs
are respected on rediscovery. No permanent per-person entities are generated.
RAM-only person IDs are omitted from outbound JSON. Persistent IDs are included
only when the secure profile store is active; names/status remain actual device
observations. No images, embeddings or biometric template records are serialized.

Topics:

- homeassistant/<domain>/<device_id>/<entity_key>/config: retained QoS 1 Discovery.
- home_buddy/<device_id>/availability: retained online / offline, with offline LWT.
- home_buddy/<device_id>/identity/state: non-retained current status/method,
  current face/identity availability and last successfully published person's name.
- home_buddy/<device_id>/identity/events: NON-RETAINED QoS 0 event JSON.

Event types: recognized, unknown, ambiguous, presence_enter, presence_leave.
Schema 1 includes event ID (MAC device namespace + random boot ID + monotonic
counter), persistent person ID when available, name when confirmed, method/status,
UTC timestamp and Unix milliseconds when synchronized (null otherwise), monotonic
uptime, source age, session/encounter and two similarity scalars. Similarities are
not probabilities. No fabricated wall-clock timestamp is emitted before SNTP.

The independent MQTT task performs every network operation. AI only pushes to a
16-entry bounded event ring under a short critical section; greeting/MQTT/legacy
consumers have independent cursors and overflow/stale diagnostics. Events older
than five seconds or from a cancelled session are rejected. An encounter emits
at most one confirmed event per person, and one unknown/ambiguous classification,
with presence edges tracked separately. MQTT adds configurable per-person/type
cooldown (default 60 s), independent of the 300 s greeting cooldown.

Before reconnect the MQTT consumer discards queued events; no offline enqueue is
used for identity data. QoS 0 avoids retransmission through ESP-MQTT's outbox;
MQTT 5 Message Expiry is five seconds. The last event's age/session is checked
again immediately before publishing. Loss during network failure or overflow is
intentional and diagnosed, not described as lossless delivery. Incoming default
HA birth message homeassistant/status=online reissues Discovery; connection also
reissues it. State is refreshed every second and expires after 15 seconds in HA.
No stale recognition event is replayed on HA or broker restart. Last-person sensor
state is historical data, not a new confirmation or event.

## Automation examples

In Home Assistant's visual editor create an automation with an MQTT trigger:
choose MQTT, topic home_buddy/<actual_device_id>/identity/events, and switch the
trigger to YAML only to add a value_template/payload if needed. Add actions in the
visual editor using your own actual light/notification entity. Never copy example
person IDs or device IDs as if they were measurements.

These are four independent trigger/condition/action examples, not installed
configurations:

1. Recognized household member: trigger payload recognized with value_template
   {{ value_json.event_type }}; template condition
   {{ trigger.payload_json.person_id | default('') == 'YOUR_PERSISTENT_ID' }};
   action persistent_notification.create with title Home Buddy and a welcome text.
2. Unknown person: same trigger with payload unknown; action
   persistent_notification.create, message An unrecognized face was observed.
   Unknown is not proof of an intruder. Ambiguous is a separate event type.
3. Personalized lighting: recognized trigger and persistent-ID condition as above;
   choose Light -> Turn on and the consenting person's preferred scene/brightness.
   Keep the similarity/confirmation decision on P4; HA performs no biometric work.
4. Presence notification: payload presence_enter; action
   persistent_notification.create, message Presence at Home Buddy. Use
   presence_leave in a separate automation for a departure notification.

Minimal editable YAML trigger (insert your actual topic):

```yaml
triggers:
  - trigger: mqtt
    topic: home_buddy/YOUR_DEVICE_ID/identity/events
    payload: recognized
    value_template: "{{ value_json.event_type }}"
conditions:
  - condition: template
    value_template: "{{ trigger.payload_json.status == 'confirmed' }}"
actions:
  - action: persistent_notification.create
    data:
      title: Home Buddy
      message: "Recognized: {{ trigger.payload_json.person_name }}"
mode: single
```

Treat event.home_buddy_recognition as an event entity: its state is an event
occurrence timestamp; do not write an automation that expects its state to equal
recognized. Use its event_type attribute or the MQTT JSON trigger above. Biometrics
alone must not unlock doors, disarm alarms or authorize security-sensitive actions.

Implemented host tests validate the six Discovery payloads, event types, escaping,
clock/freshness rules, bounded consumers and absence of templates. They do not
prove broker authentication, TLS handshakes, reconnect timing, HA restart or a
physical automation. Those acceptance checks are still pending.

Sources inspected 2026-10-23:
[HA MQTT](https://www.home-assistant.io/integrations/mqtt/),
[MQTT event entities](https://www.home-assistant.io/integrations/event.mqtt/),
[ESP-MQTT](https://docs.espressif.com/projects/esp-mqtt/en/stable/esp32p4/index.html).
