# MQTT integration design (not implemented)

No MQTT client, Discovery payload or Home Assistant recognition event is active
in this firmware. The following is the version-1 contract to implement in phase 7.

Use hardware-derived stable device IDs, independent stable person IDs and Polish
display names. Default event topic home_buddy/identity/events is configurable;
use a per-device namespace for multi-device installations. Retain discovery,
state and availability; never retain recognition events or replay an offline backlog.
Use LWT offline, online after connection, bounded reconnect and configurable QoS.
Application event IDs combine a boot identifier and monotonic sequence; consumers
can deduplicate QoS delivery. Timestamp is null until wall-clock synchronization.

Event types: recognized, unknown, conflict, presence_enter, presence_leave.
Include person_id/name only when supported, method, status and modality similarities.
Similarity is not probability. Conflicts contain both modality results and no
fabricated winner. Individual presence entities mean detection at this device,
not occupancy of the whole home.

Plan native MQTT Event discovery with event_types so repeated recognition of the
same person triggers HA automations independently of unchanged sensor states.
Publish identity, modality and status sensors; face/speech detection, local light,
backlight, memory, uptime and camera availability. Availability gates stale state.
Delete per-person retained discovery/state on authenticated profile deletion.

## Temperature input design

Preferred input is a configurable MQTT topic, with payload carrying a numeric
Celsius temperature or explicit unavailable. Future docs must include a tested HA
automation publishing entity changes, HA startup and availability transitions.
A retained reading alone has no reliable age: include the measurement timestamp
and reject stale retained payloads after reboot. Manual source selection belongs
in PIN-protected settings. No unrestricted HA REST token is required by MQTT.
TLS must verify the broker certificate; no insecure certificate bypass.
