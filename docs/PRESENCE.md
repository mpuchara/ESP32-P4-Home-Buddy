# Diagnostic face-based presence, 0.4.4-dev

Kamera lokalna -> Test twarzy shows Obecnosc: TAK / NIE / nieustalona / wylaczona
beside the existing raw detection boxes and model timing. The foreground test
remains page-scoped. An independent, explicit Obecnosc w tle diagnostic lease can
continue on the UBox screensaver for up to10 minutes. It is OFF after every reboot.
No identities, stored frames, HA events or presence-triggered greetings are enabled.
Face detection is only a visibility proxy: a person facing away, outside the field
of view or obscured can be missed.

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
* A transition from no active requester to active, final stop, or error resets evidence. Closing the page while a live monitor lease remains preserves the ongoing stream.
  Disabled gives OFF; enabled but
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


## Bounded background diagnostic, 0.4.4

Open Obecnosc w tle and press Uruchom10minut. The owner is a RAM-only monotonic
600000ms lease; Zatrzymaj cancels it and a new explicit start renews it. UI reads,
menu activity, foreground test toggles and configuration saves never renew it.
The worker expires the lease without a UI task and checks it under the publication
lock before accepting a result. The getter reports expired demand immediately;
an already running inference can finish, then resources are released once no
requester remains. No new result may publish after lease expiry.

One worker/model/input serves two owners: foreground diagnostic and background
lease. Closing a page cancels only foreground demand. Lease expiry/stop does not
cancel an independently active foreground test. A change of owners with no gap
preserves temporal evidence. Starting after expired demand first processes expiry,
then resets the session before accepting new evidence. Model load/admission failure
cancels both requests with the existing error; no retry storm or silent restart.

Foreground target period remains500ms start-to-start (actual model can take longer).
Background-only target period is1500ms to leave CPU time for JPEG/network work;
slow work never causes overlapping inference. Existing750-800ms timings are prior
observations, not a guaranteed background latency. Raw storage and all memory
admission/load-delta guards remain. Test kolorow explicitly stops both owners before
changing sensor pattern. Preview off only releases foreground demand.

The monitor page displays countdown, availability/presence, last inference duration
and error. The default screensaver content stays unchanged and never shows local
camera images or a new identity greeting. Logs carry only scalar owner/timing/heap
metadata. No network service, HA entity/event or new persistent record is added.

Host face_demand tests cover default OFF, both ownership directions, exact lease
boundary, cancellation, explicit renewal only, read/close behavior, overflow and
clock anomaly rejection. They test policy, not RTOS concurrency or a physical
10-minute expiry. Background-on-screensaver behavior, natural JPEG coexistence,
manual stop/release, repeat starts and actual10-minute expiry need hardware receipts.


0.4.4 source299f7dd021efdce7eb557dbf55cf1bec9ee9a4db passes host11/11, all three
local builds/configuration checks and four CI jobs39937908256. Final Wi-Fi image
is flashed/hash-verified on COM5; boot,30fps capture and natural full-size UBox
are sampled with monitoring inactive. Background behavior/stop/10-minute expiry
are explicitly pending physical acceptance. Final artifact receipt is in
DEVELOPMENT_LOG.md, including the superseding diagnostics-final package.
