# MQTT temperature input, iteration 2

Configure a broker URI, separate username/password, an exact topic and freshness
limit in the local UI (without administrator PIN). There is no hardcoded broker/topic/secret.
A useful operator-chosen topic is home_buddy/temperature/outdoor. The client
subscribes QoS1, reconnects after 10 s, checks TLS CA trust and bounds its input to
767 bytes. Arrays/nested objects, duplicate keys and fragmented offset mismatches
are rejected. It contains no MQTT Discovery or recognition publisher yet.

```json
{"temperature":12.5,"unit":"C","timestamp":"2026-10-22T10:00:00Z","available":true}
```

This is an illustrative payload, not a measured temperature. Numeric Celsius only,
finite -80..80, strict ISO8601 seconds with Z or +/-HH:MM, and real synchronized
wall time are required. Fractions of seconds are not accepted. Explicit
`{"available":false}` clears the display. Missing/unknown/stale/future timestamps,
strings instead of numeric values and malformed JSON do not invent a reading.
The configured limit is 30..86400 seconds, default 900. Retained old data remains
rejected after reboot; valid data expires locally during outages. A retained
message received before SNTP is rejected, not buffered; a later publisher update
is needed after sync. Invalid messages preserve the previous valid reading only
until its original timestamp expires.

## Home Assistant automation example (not live verified)

Replace entity and topic locally. This sends changes, HA startup and a five-minute
refresh without inventing a new measurement timestamp. `last_updated` is retained
as the source timestamp; a sensor that does not update for longer than the device
freshness limit will be shown stale. Choose a limit appropriate for the sensor.
The example intentionally has no unrestricted HA REST token.

```yaml
alias: Home Buddy outdoor temperature
triggers:
  - trigger: state
    entity_id: sensor.outdoor_temperature
  - trigger: homeassistant
    event: start
  - trigger: time_pattern
    minutes: "/5"
actions:
  - action: mqtt.publish
    data:
      topic: home_buddy/temperature/outdoor
      qos: 1
      retain: true
      payload: >-
        {% set s = states.sensor.outdoor_temperature %}
        {% if s is defined and s is not none and is_number(s.state)
              and s.attributes.get('unit_of_measurement') == '°C' %}
          {{ {'temperature': s.state | float,
              'unit': 'C',
              'timestamp': s.last_updated.strftime('%Y-%m-%dT%H:%M:%SZ'),
              'available': true} | to_json }}
        {% else %}
          {{ {'available': false} | to_json }}
        {% endif %}
mode: restart
```

Host tests execute the actual cJSON-backed parser, freshness and unavailable
handling. Broker connection, retained-message delivery, TLS/reconnect and the HA
automation have not been physically verified. This module is a reusable transport
foundation; biometric events/discovery remain future work and are not simulated.
