# Home Assistant over HTTP, 0.6.1-dev

No broker or custom integration is needed. Recognition remains local. Enable the
recognition worker explicitly in the household/test UI. Publishing is independently
disabled by default. The fullscreen dashboard and local greeting do not depend on HA.

## Webhook setup in the graphical editor

1. HA: Settings -> Automations & scenes -> Create automation -> Create new.
2. Add a Webhook trigger. Generate a random secret ID, 32-128 characters using
   letters, digits, underscore or hyphen. Allow POST; retain local-network-only.
3. Home Buddy: Settings -> Home Assistant. Enter the bare HA host/IP (no scheme
   or path), port (normally 8123), Webhook mode and that ID in the masked field.
   Select HTTPS only for a verified TLS endpoint. Save, then reopen this page.
4. Test connection performs GET `/`. It verifies host reachability only; it cannot
   verify a webhook ID. Send diagnostic test performs the actual POST with
   `event_type: connection_test`, `method: diagnostic`. Inspect HA automation traces.
   HTTP 2xx alone does not prove the webhook exists or that an action ran.
5. In the automation add a Choose action with template condition
   `{{ trigger.json.event_type == 'recognized' }}`. Keep diagnostic tests in a
   separate branch, for example a harmless persistent notification. Never treat a
   diagnostic as biometric recognition.
6. Enable event publishing on Home Buddy. Enable sending names only if wanted.
   Perform actual enrollment/verification, leave and return. Confirm the real
   `recognized` payload and harmless action in HA traces.

One webhook can dispatch all people and event types. Use `trigger.json.person_name`
(optional), `trigger.json.person_id` (only persistent IDs), `trigger.json.event_id`
and `trigger.json.event_type`. Guard optional fields using `.get('person_name', '')`.
Supported observed types: recognized, unknown, ambiguous, presence_enter,
presence_leave. No voice events are fabricated. RAM-only profiles explicitly set
`person_id_persistent: false` and omit `person_id`. Timestamps include boot uptime;
wall time is supplied only when available. Event IDs use a random boot namespace.

## Last recognized person helper

HA: Settings -> Devices & services -> Helpers -> Create helper -> Text. Name it
Ostatnio rozpoznany domownik. Add it to a dashboard. In the recognized Choose
branch add action `input_text.set_value`, target that helper, value
`{{ trigger.json.get('person_name', 'Domownik') }}`. Enable name publishing if names
are needed. The helper is user-created, not an automatically registered sensor.
Events still trigger a new encounter even when the helper text is unchanged.

## REST alternative

Create a Home Assistant Long-Lived Access Token in the user's profile security
settings. It can grant broad account permissions. It is distinct from the UBox
Bridge X-UBox-Token; do not copy that credential. Select REST API on Home Buddy,
enter the same bare host/port and paste the token in the masked field. Save.
The client sends `Authorization: Bearer <token>` in the header, never in the URL.
GET `/api/` tests API access; diagnostic POST tests the actual operation separately.
Events POST to `/api/events/home_buddy_identity`. In HA use an Event trigger with
event type `home_buddy_identity`; replace `trigger.json` in the examples above
with `trigger.event.data`. Check real event/automation traces separately.

## Delivery and diagnostics

The independent HTTP task uses bounded buffers and a 1.5s request timeout. Status
distinguishes disabled, host/API reachable, POST 2xx, unavailable, 401, 403, invalid
configuration, TLS error, timeout and rejected HTTP response. Redirects are refused.
Connection recovery retries after 5s and drains offline events. There is no durable
network outbox, retained message or stale replay. The shared 16-entry event ring
reports overflow; session cancellation/freshness (5s) is checked before POST.
HA cooldown is separate from the 300s greeting cooldown. A genuine new encounter
may publish again after the configured HA cooldown; inference does not spam events.
One request already in flight cannot be recalled after session cancellation.

HTTP on a LAN has no transport encryption. HTTPS uses the trusted CA bundle and
hostname verification; there is no insecure bypass. A private/self-signed CA needs
explicit future trust provisioning and currently fails verification. Credentials
are masked in this form and omitted from logs/exports. Development NVS is plaintext
at rest; restrict physical access. No image, audio or embedding enters the payload.
Do not expose the webhook to the internet. Face recognition/webhook alone must not
control high-risk access such as door unlocking.

## Sensor architecture decision

The helper is the initial display solution. A local read-only HTTP status server
was evaluated and deferred: it adds an inbound endpoint, access control and polling
work without helping event delivery. `/api/status` is NOT implemented and no RESTful
Sensor example is advertised as working. No ephemeral `/api/states` entity is used.
This client does not automatically register HA devices/entities.

References: [Webhook trigger](https://www.home-assistant.io/docs/automation/trigger/#webhook-trigger),
[REST API](https://developers.home-assistant.io/docs/api/rest/).
