# Diagnostic face-based presence, 0.4.3-dev

Kamera lokalna -> Test twarzy shows Obecnosc: TAK / NIE / nieustalona / wylaczona
beside the existing raw detection boxes and model timing. The detector remains an
explicit diagnostic: page close/timeout, preview off and color pattern stop it.
No continuous background inference, identities, stored frames, HA events or
presence-triggered greetings are enabled. Face detection is only a visibility
proxy: a person facing away, outside the field of view or obscured can be missed.

## Evidence policy

* Start/re-enable: UNKNOWN. Two consecutive distinct positive analyses confirm
  PRESENT. A negative analysis resets the positive streak. Any face counts; no
  association with a household profile or a particular rectangle is implied.
* PRESENT survives isolated negative results and fresh-data gaps within 3 seconds.
  Four or more consecutive negative analyses, spanning at least 5 seconds from
  the first negative source timestamp, confirm ABSENT. Both conditions are needed.
* ABSENT needs two consecutive positives to return to PRESENT. It does not change
  on a single isolated positive.
* No accepted completed analysis for more than 3 seconds gives UNKNOWN, whether
  the last confirmed value was PRESENT or ABSENT. A subsequent observation after
  that gap resets old streaks before counting new evidence. No-data time alone
  never establishes absence.
* Stop/cancel/re-enable/error resets evidence. Disabled gives OFF; enabled but
  unhealthy/loading gives UNKNOWN. Nothing persists across reboot or sessions.

Only accepted analyses with capture<=completion<=now, source age<=2500ms and
completion age<=1200ms count. Duplicate sequence, non-increasing source/completion
and malformed/future/expired timestamps are rejected. Monotonic capture timestamps
allow a normal uint32 frame sequence wrap. No tick or UI read increments evidence.
Reads derive timeout state from the locked tracker without mutating it. A small
static tracker/counters use no image/history allocations or flash writes.

The UI retains TAK/NIE while confirming a possible change and displays the current
streak. Rectangles still obey the shorter face-result freshness policy and vanish
on a newer zero-face result. Thus a briefly missing box or AI: czekam does not
immediately change stable presence. The presence timeout is independent of that
caption; UNKNOWN means evidence is unavailable, not a negative analysis.

Publication checks the enable/disable epoch while holding the status lock, then
updates result and tracker together. A cancelled inference cannot seed the next
session. Model resources, score threshold, capture/exposure, brightness calibration,
UBox behavior and memory admission remain unchanged from 0.4.2.

## Verification and remaining work

Host temporal_presence tests cover entry/exit, isolated misses, isolated positives,
minimum elapsed negative evidence, duplicate/read behavior, chronology, sequence
wrap, exact gap boundary, stalled data, healthy/disabled state and reset. These
verify the temporal policy, not neural accuracy, physical presence or concurrency
stress. Physical UI/layout, step out/in behavior and repeat activation require
operator acceptance. Final build/flash/CI receipts are in DEVELOPMENT_LOG.md.

This module provides reusable temporal evidence for later integration. Before
actual Home Assistant automation, validate representative lighting/occlusion,
labelled false-positive/negative cases, availability and a deliberate event policy.
Household identification still requires alignment/embedding/enrollment; no identity
or occupancy accuracy is inferred from this diagnostic.


Final source7cc3c7d66092c78f7503c22aae301130e8dc271a: host10/10, Python3/3,
all local profiles/configurations and four CI jobs pass (run39926305385). Image
flashed/hash-verified on COM5; boot/camera/network are sampled with AI disabled.
Operator subsequently confirms TAK, a tolerated brief miss, NIE after leaving
for>5s, TAK on return and readable labels. These transitions/layout are physically
accepted; exact timing boundaries, repeated sessions and long soak remain open. See the final0.4.3
receipt in DEVELOPMENT_LOG.md; successful compilation is not physical acceptance.
