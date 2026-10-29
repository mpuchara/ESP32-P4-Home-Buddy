# 2026-10-24: background navigation intent fix prepared

## 2026-10-29: 0.11.11 installed, AUTO/26 restored, configured fast wait observed

08288bc app-only install passes verify and full 16 MiB comparison; other sectors
unchanged. Normal boot restores saved/clean 26, ready AUTO, models 1/0. UBox and HA
host recover. Five CI jobs pass (40893822504). Runtime logs show pending KNOWN/1
wait_ms=100, then UNKNOWN/0 wait_ms=1000. These are configured waits, not physical
speedup. New recognition/greeting/HA latency acceptance awaits the operator test.


## 2026-10-29: real greeting/HA accepted; pending-confirmation wait reduction

0.11.10 owner confirms display greeting and HA event, but slow acquisition. Current
profile 26 reaches actual 3/3; UI queue receives greeting 348 ms after confirmation.
Weak-score resets cause 34068 ms from first known candidate; final accepted series
spans 4911 ms. These are log intervals, not entry/LCD/phone/p95 measurements.
0.11.11 uses 100 ms wait only for one/two pending background known hits, returning
to 1000 ms otherwise. No thresholds, three fresh-frame gates or enrollment change.
Production-policy test removes 1800 ms configured waiting; physical gain unmeasured.


## 2026-10-29: 0.11.10 crop correction installed, profile 23 retained

585669d app-only update passes verify and complete 16 MiB comparison; all other
sectors including profiles unchanged. Normal boot restores B/23 saved/clean.
Source CI five jobs pass (40889248031), plus 32 CTest/24 Python/3 checks and full
sanitizers. The preceding foreground test may leave AUTO OFF; opt-in is preserved.
No recognition/landmark/greeting/HA/phone acceptance inferred from installation.


## 2026-10-29: MNP square clipped by rectangular ROI, 0.11.10 candidate

Sharp/stable operator preview still loses landmarks. Pinned MNP squares each
candidate; old independent ROI margins truncate portrait candidates. A host
regression reproduces that exact clipping. Map the native square and inclusive
edges into bounded context, preserve sensor boundaries and the 640x640 buffer.
500+ accepted-box contract checks plus edge/close cases cover it. No threshold,
model, profile schema or buffer-size change. Physical stability remains unaccepted.


## 2026-10-29: UBox encounter still unconfirmed, matching trace identifies gate

0.11.9 records UNKNOWN 0.544/0.457, one KNOWN 0.726 (1/3 confirmations), then UNKNOWN
0.515 (0/3). No admitted recognized/greeting receipt in this encounter; owner sees
no greeting. HA is enabled/reachable and HTTP POST 200 counters rise, which does
not prove recognized payload, automation or phone delivery. Retain all gates;
next isolate face/landmark/image stability in the existing foreground preview.


## 2026-10-29: 0.11.9 installed, profile 23 retained, HA host reachable

714bfd1 app-only install passes verify and full 16 MiB comparison; all other sectors
including profiles unchanged. Normal boot restores B/23 clean and ready AUTO.
HA enabled, host GET returns 200 after initial network retries. This does not prove
recognized-event/webhook/phone delivery. Runtime trace and owner encounter remain
pending. CI host passes; four target builds in progress at this check.


## 2026-10-29: generation 23/MBF ready, notification path needs runtime trace

Owner reports no display or phone notification despite saved new profile. Serial
confirms 23 clean, AUTO ready and full MBF runs, mixed with preprocessing failures.
Add numeric matching/confirmation/event/greeting/HA receipts in 0.11.9 without
changing any decision gate or sending diagnostic/recognized events synthetically.
Discard incoherent live JTAG reads; rely on production runtime receipts instead.


## 2026-10-28: new-light attempt blocked before inference by cold-start RAM

0.11.8 earlier verifies eight samples and commits 18. Owner subsequently deletes
and recreates the profile; models release and cold admission fails at 43008 B largest
internal block (65536 B guard), despite 236079 B free. Current profile is enabled
but has zero samples at generation 20. Profile readback and controlled restart
preserve 20 and recover a 180224 B block. Restart is an explicit recovery, not a
permanent fragmentation fix or artificial-light acceptance. See current report.


## 2026-10-28: 0.11.8 feedback candidate installed, B/17 ready AUTO restored

8528881 app-only update follows STOP of the preceding RAM session and a fresh full
backup. Verify plus 16 MiB readback pass, all other sectors unchanged. Normal boot
restores saved/clean generation 17 and ready AUTO with camera frames. Five CI jobs
pass (40718883733), 32 CTest/24 Python/3 checks and existing IRAM/UI guards pass.
Actual new-light enrollment/feedback acceptance remains the next operator test.


## 2026-10-28: enrollment verification feedback candidate 0.11.8

Eight samples/visible landmarks are not a verification receipt. The running session
has verified=false; saved generation 17 remains. Guided UI hid the actual worker
reason. Show it, separate verification mismatch from other-profile conflict and
add bounded numeric progress logs. No threshold, sampling or commit-guard change.
Installation deferred while the operator's unsaved enrollment session is active.


## 2026-10-28: fresh face samples saved to existing identity

Owner completed guided eight-sample re-enrollment and verification. Generation 17
commit/readback passes; exactly one face record changed, ID/name/enabled preserved,
voice data unchanged. Debugger-free restart restores 17, saved/clean, ready AUTO.
Actual post-update recognized greeting/HA event remains pending. See current report.


## 2026-10-28: touchscreen toggle, no flicker and background AUTO observed

Owner confirms successful disable/enable on installed 0.11.7 and no screen flicker.
Passive serial confirms generations 15/16, clean saves and models 1/0. Controlled
restart restores 16 and ready AUTO. Background detection continues alongside UBox:
faces are sometimes detected but logged attempts fail ROI/five-landmark quality
before matching. No full identity/greeting/HA acceptance inferred. Owner reports
poor enrollment images. See ITERATION13_REPORT.md for measured scope and limits.


## 2026-10-26: final 0.11.7 installed, final-source persistence verified

098ede3 app-only installation passes verify_flash and full 16 MiB comparison; every
other sector unchanged. Exact-source automated worker cycle commits 13/14, stable
boot/identity/templates, model loads/releases 1/0. After debugger shutdown, valid
14/13 readback proves canonical payload byte-identical to pre-test 4. Normal restart
restores 14, saved/clean and ready AUTO, CSI ~30 FPS. No recovery or re-enrollment.
CI 40411959469: five successes; local 32 CTest/24 Python/3 checks plus seventeen
IRAM symbols and 2736 B UI guard pass. See ITERATION13_REPORT.md for actual memory,
source/binary hash, public package and physical visual/greeting/HA/voice limits.


## 2026-10-26: six target commits and exact payload retention; final save-wait UI

0.11.6 completed three automated disable/enable worker cycles, generations 7-12,
without a boot-ID change or model reload/release (1/0 throughout). Names/IDs/counts
were stable. After debugger shutdown, full profile-partition readback validates
12/11 and proves generation 12 canonical payload byte-identical to pre-test 4.
Normal restart restores 12 and ready AUTO. CSI continues ~30 FPS. All five firmware
CI jobs pass (40386086581). UI/visual/watchdog acceptance during JTAG is not claimed.
Measured saves of 5.03/6.04 s motivate 0.11.7's create timeout guard: keep waiting
while an actual durable save is active; retain the old command timeout otherwise.


## 2026-10-26: JTAG captures the Flash-time CSI fault, 0.11.6-dev

Owner reproduced disable restart. Live serial: HP watchdog reset, previous valid
profile generation 4 restored. Exact ELF/JTAG saved exception: mepc 0x402dd6be,
ra 0x4ff01c7a, mcause 0x19/hart 0. The pinned SDK omitted
`dw_gdma_channel_config_transfer` from its noflash mappings, although CSI calls it
from its cache-safe ISR. Add a project-local conditional linker fragment and an
expanded seventeen-function ELF guard. Existing 0.11.5 ELF now fails this precise
guard. Debugger-injected command reproduction uses the real worker/transaction;
it is not touchscreen or normal-watchdog acceptance. Owner explained dark camera
metrics by darkness and device orientation; no image-path change is needed.


## 2026-10-26: 0.11.5-dev installed, CSI and automatic restore observed

311dbe1 app-only update verified, plus full Flash readback against a fresh private
backup: all other sectors unchanged, profiles A generation 4/B generation 3 retained.
CSI now registers and completes frames at 29.6-29.7 FPS. Actual boot restores A/4
in 164230 us, durable clean state, LVGL and ready AUTO with models 1/0. No callback
registration/panic/watchdog/underrun marker in first 55 s. Image luminance currently
below range; operator lighting/lens context requested. Exact disable/enable/flicker,
post-change restart, greeting/HA and voice acceptance remain pending. All five
firmware-source CI jobs pass (40374472427); see ITERATION13_REPORT.md.


## 2026-10-26: shared DMA correction, 0.11.5-dev

The 0.11.4-dev post-install boot check caught DW-GDMA rejecting the CSI controller's
external user context. No operator acceptance was claimed. DSI's IRAM-safe selection
also affects CSI. Enable CSI cache-safe allocation and DW-GDMA IRAM control helpers;
place application callbacks and frame-pool ISR callees in IRAM. Add a target-linked
ELF placement guard and configuration invariant tests. Require real camera frames
in the next boot check, alongside the preserved generation 4 and ready AUTO.


## 2026-10-26: 0.11.3 profile toggle defect, 0.11.4-dev follow-up

Owner observed Flash-associated display flicker and MBF admission failure after
last-member disable/enable. Actual storage remained durable, clean at generation 4.
Passive capture: free internal 249939 B, largest 59392 B; AUTO selected but not ready,
models loaded/released 1/1. Retain the same model set only during an AUTO member
pause, with no inference from disabled members and no guard reduction. Explicit
Stop/cancel/delete still release. Enable and enforce DSI/DW-GDMA IRAM-safe interrupt
handling during Flash; correct firmware labels. See ITERATION13_REPORT.md for
causality, software checks and pending exact-path physical retest.


## 2026-10-26: iteration 13 authorized installation and boot

Owner authorized installation and necessary physical testing. Installed the
8499ffc / 0.11.3-dev app and pinned speaker model on COM5 after a private full
16 MiB backup and security/table checks. Application/model verify and full readback
pass; profiles and all other sectors outside the selected ranges are byte-identical.
No profile recovery was needed. Boot restores A generation 2 in 170369 us, retains
durable clean status, starts LVGL and resumes ready AUTO without manual activation.
Operator touch changes/restart/greeting/HA and PCM/model inference are still pending;
provisioned model weights do not constitute audio/model-execution acceptance.
See ITERATION13_REPORT.md for measurements and their limits.


## 2026-10-26: iteration 13, 0.11.3-dev

Continue from 6c095eb, preserving its later physical retention/AUTO evidence.
Commit empty fallback only on fully blank storage; preserve unknown/torn bytes.
Bound Flash operations, yield between them, retain commit readback and compute
admission. Add production-engine cut tests, remove household raw save codes,
isolate recovery in loader, and publish a numeric voice readiness report.
See ITERATION13_REPORT.md for validation and pending target acceptance.


Saver navigation previously consulted the worker's delayed mode snapshot. It
could cancel an accepted but unprocessed background request, or preserve an old
background snapshot after a newer Test request. Track accepted mode-changing UI
commands instead, clear ownership on page close, and preserve normal cancellation
for ordinary exits. Add a short acceptance acknowledgement and explicit worker
background deadline/expiry logs. Host regression covers both delayed-mode
directions, queue rejection and Stop; all 18 C tests, 7 Python tests and 3 project
checks pass. Firmware build/package results are recorded in IDENTITY_VALIDATION.
No device flash/reset: physical acceptance of this fix is pending and the
operator's enrolled RAM profile is preserved.

# 2026-10-24: unattended background completion observed

The first expiry attempt cancelled during saver navigation after about twenty
seconds. A retry with a twenty-second wait before saver entry remained active.
Passive USB retry-01..10 captures show current inference from uptime 26,386,061 ms
until clean model release at 26,925,411..26,925,433 ms; heap_ok=1 and PSRAM returns
to 26,327,860 bytes. Camera/UBox continue without reset. This accepts unattended
completion consistent with the ten-minute lease, not exact deadline timing:
the start command predates the new capture. One full 2304x1296 UBox decode succeeds
while inference is active. Preserve the RAM profile and continue to physical
outage/negative checks. No firmware/configuration/security change was made.

# 2026-10-24: operator accepts Stop/navigation notification outcome

Operator reports Stop followed by no new notifications in the requested trial.
HA's retained trace list still ends at the previous 13:54:06 run. USB shows a clean
model release and restored PSRAM in identity-stop-acceptance-05.log, but no matching
button timestamp, so that release is not conclusively attributed to Stop versus
lease expiry. This accepts the operator-reported Stop/navigation path, not an
independently timed isolated command test or separately observed greeting absence.
Started passive capture for a fresh timed ten-minute background-expiry test and
requested immediate operator acknowledgement of activation. No firmware write,
reset or profile loss. Offline/negative/multi-person/security acceptance remains open.

# 2026-10-24: controlled encounter deduplication and return accepted

Operator reports one notification during the 90-second stay, then one after the
15-second leave/30-second return. Retained HA traces confirm two new successful
recognized/face/confirmed runs at 13:52:34 and 13:54:06 Europe/Warsaw, runtime about
0.28 seconds each. Event sequences 54/57 differ, session 123 is the same, encounters
22/23 differ, source ages are 1356/1539 ms. This accepts that one-person scenario;
it is not broader accuracy or an uninterrupted event-stream capture. Renewed the
HA listener (it had disconnected) and passive USB capture for explicit Stop.
No firmware change, reset or security provisioning. Expiry/offline/negative and
multi-person acceptance remain pending.

# 2026-10-24: live recognized-only notification acceptance

After operator sign-in, the live HA listener received a real recognized/face/
confirmed event at 2026-10-24T11:34:44.764184Z (session 106, encounter 18, event
sequence 47, source age 1576 ms). The User detection trace matches the 13:34:44
Europe/Warsaw trigger, mobile notification execution and successful completion
at 13:34:45, runtime 0.30 seconds. Operator confirms the expected message arrived.
No automation YAML, firmware, thresholds or device reset were required. This
accepts the real recognized-only harmless action; it does not establish the
cause of the earlier missed attempt. Continued to continuous-stay deduplication
and a real subsequent encounter, with browser/USB observation and the RAM profile
preserved. Stop/expiry, offline and broader accuracy acceptance remain open.

# 2026-10-24: unexpected notification during covered-camera trial

Live HA follow-up after operator sign-in: the intended User detection automation
is enabled and its stored YAML correctly matches recognized/face/confirmed within
home_buddy_identity. No conditions; the existing notification action/template is
correct. No available traces were shown. No HA configuration was changed. Started
a read-only event listener and passive USB capture for a fresh explicit background
leave/return trial; the operator only needs to perform that physical operation.

Operator correction: the notification anomaly was caused by an incorrectly
configured HA automation, according to the operator. Do not count the earlier
report as a proven fresh covered-camera false acceptance. The intended automation
still fails to run for recognition while a test event works. Inspect its live
trigger/trace and background-versus-foreground mode; preserve the existing RAM
profile. HA is reachable in the browser, but the agent tab requires login; the
operator elected to sign in directly. No credential collection or firmware write.

Operator reports three notifications in the deduplication trial, one while the
camera was covered. Notification/deduplication acceptance fails; the cause is
unassigned until actual triggering events/timestamps and notification text are
correlated. Check fresh inference versus previously generated event/delayed phone
delivery and older/duplicate automation. Passive camera ownership/freshness review
retains leased frame snapshots, completion sequence/timestamp and five-second
event freshness gates. No firmware/threshold change or reset is justified yet.
Requested a renewed background lease, opaque local-lens cover and matching HA
events while USB capture runs. Prior real recognized delivery remains evidence of
the transport path, not proof that this negative-camera scenario is correct.

# 2026-10-24: real confirmed identity reaches Home Assistant

The operator supplied a real home_buddy_identity event with recognized/face/
confirmed, an enrolled RAM profile name, similarity 0.762862741947174 and source
age 1299 ms. Source timestamp 2026-10-24T07:29:31Z; HA receipt 07:29:32.000870Z.
person_id_persistent=false and omitted person_id are expected for RAM-only mode.
Local inference, confirmation, personalized greeting and real REST identity-event
delivery are now accepted for the one-person scenario. Similarity is not an
accuracy estimate, and coarse wall timestamps do not measure network latency.
Next: recognized-only harmless automation, encounter deduplication/re-entry,
stop/expiry and offline checks. No reset/flash while the RAM profile is in use.

# 2026-10-24: operator accepts one-person local recognition scenario

Background follow-up: operator confirms the expected personalized greeting on
the saver, but the HA listener shows no event. Local greeting is accepted; actual
recognized HA delivery remains open. Diagnose publishing enablement and repeat
the existing harmless diagnostic POST before changing firmware/resetting the RAM
profile. Independent greeting and HA event consumers are retained.
Subsequently the operator supplied a real HA presence_leave event (method face,
2026-10-24T07:28:44Z, source_age_ms=780). Automatic presence delivery is accepted;
the next check explicitly looks through new events for recognized/face/confirmed
with the listener already active. The leave event does not establish identity
delivery. RAM-only nonpersistent-ID signaling is consistent with this build.

The operator confirms all steps of the supplied scenario work, including name
entry, eight-sample RAM enrollment, fresh-frame verification/save, confirmed
KNOWN, leaving the camera frame and recognition on return. This accepts the
household keyboard fix and that local scenario only; no quantified accuracy or
impostor acceptance is inferred. Continued automatically to real background HA
recognized-event delivery and the local saver greeting, preserving the live RAM
profile without resetting/flashing. Secure persistent profiles remain gated.

# 2026-10-24: household name keyboard usability

Enrollment was blocked by household buttons/status drawing over keyboard keys.
The shared focus handler now raises an opaque keyboard above later form children;
ready/cancel hides it without clearing typed text, and tapping the field reopens
it. Household actions use two rows above the keyboard, the title/name labels are
shorter, profile-list contrast is explicit, and name editing no longer idle-closes.
Actual household/form functions were rendered at 800x480 before/after using LVGL.
The host UI harness also entered a key, checked foreground stacking, closed on
ready, reopened on tap and verified the text survived. No biometric samples were
enrolled as part of that UI check. On-device input was subsequently accepted in
the operator's complete local-recognition scenario above.
Recognition build, effective-config/budget and formatting checks pass. App-only
COM5 installation and verify_flash pass; application is 4,386,720 bytes, SHA-256
4f6cb3b2ac58ea8bfae7e1faed6e6883b53efc23ffc022fc25aee9e612e799bb.
NVS, models, partition table and profile areas were not written.

# 2026-10-24: captured Flash validation watchdog and mmap candidate

Application-only candidate installation and Flash verification passed. The
operator completed five re-entry cycles with landmarks/successful-frame status
and no restart. USB confirms six validated loads, passing release heap checks
and real 512-dimensional MBF results (total pipeline approximately 1.48..1.59 s).
The reproduced restart scenario is accepted; soak/multi-person accuracy and
recognized HA delivery remain pending. Proceeded to one consenting RAM-only
enrollment and fresh-frame verification; no security provisioning or reboot.

Captured CPU1 interrupt watchdog during model validation, with CPU0 waiting in
the Flash/cache IPC handshake. Matching ELF decoding and evidence are recorded
in IDENTITY_VALIDATION.md. Replaced thousands of raw Flash reads with one cache
mapping, retaining header validation and full pinned SHA-256 on every load.
Added host fault/cleanup/full-coverage/reload regression: C suite 17/17, Python
7/7, project checks 3/3 pass. Hardware acceptance of this candidate remains open;
the watchdog timeout and recognition thresholds are unchanged.

# 2026-10-24: MBF hardware re-entry investigation

Operator sees MBF ready / one face / UNKNOWN, landmarks and the successful-frame
message. Re-entry from the saver after a completed face frame is reported to
restart P4; this is an open acceptance blocker, not yet a reproduced/fixed cause.
Added bounded lifecycle/reset/heap/stack diagnostics, incremental reconnecting USB
capture and two disconnect regressions. UI shows absent comparison scores as
"brak", with solid yellow landmark dots. Host suite 16/16 and Python 7/7 pass.
See IDENTITY_VALIDATION.md for current observations and unresolved JPEG coexistence.

# 2026-10-23: 0.6.1-dev continuation

REST transport acceptance: the operator configured the LAN endpoint and supplied
an actual HA event showing event_type home_buddy_identity, payload type
connection_test, method diagnostic and status test (2026-10-23 20:26:07 UTC).
The operator then confirmed that the event automation/notification works. This
accepts authenticated device-to-HA diagnostic transport and the harmless action,
not biometric recognition. Credentials reside only in private device NVS and
private recovery artifacts; none is committed or embedded in public firmware.
Firmware f764f3212696c061e92c33a6425f87fb295c4267 passed host plus all four firmware
CI jobs in run 40008194920. Current local app is 4,383,248 bytes, SHA256
db2827836af2913d9ef48d14d9ca8e995a915b7641708bc13c32be1d64e7b5f4.
Next active acceptance: real onboard MBF execution, then consenting on-device
enrollment and a real confirmed recognized event. Voice remains unimplemented.

HA save correction: removed the erroneous 32-character minimum for a nonempty
URL-safe Webhook ID (maximum 128 unchanged). No real credential is stored in tests
or logs. Validation now identifies the offending field; saving hides the keyboard,
confirms NVS commit plus byte-exact readback, and keeps the form/values on success
or error. HA settings no longer idle-close while editing. Host suite 16/16 passes,
including short synthetic IDs, save/load and commit/readback failure injection.

HA form readability correction: the new panel inherited a white background while
its labels were white. Matched the existing settings dark panel, set checkbox and
transport colors explicitly, shortened labels to fit the left column and bounded
the status label width. Manual LVGL 800x480 renders reproduce the invisible labels
before and show legible fields/options/test actions after. Recognition firmware,
font/partition checks and package integrity checks pass. Updated app is 4,382,192
bytes (SHA256 8b45c8004f67c56600cda54963fddf2ee7d1b2c25bc03359f26730053b66f2c5).
USB application-only installation matched the single-factory table and completed
with exit 0, including verify_flash. Only application bytes at 0x20000 were written.

Supersedes the iteration-6 dual-OTA and MQTT deployment decisions. One factory
application, onboard MBF, protected profile gate and future voice capacity; native
HA Webhook/REST and masked touchscreen configuration. See ITERATION7_REPORT.md
for current code, automated receipts and separate physical/security acceptance.
Earlier entries below are historical and retain their original version context.

# Development log

## 2026-10-22: initial foundation

Repository began at 0c361ab with LICENSE only. Work is on
codex/initial-hardware-foundation. HA_UBOX_bridge was read through GitHub only;
no checkout, file mutation or commit was made in that repository.

### Research and implementation

Inspected both requested ultramcu repositories, their source APIs and licenses,
the existing UBox snapshot handler, official ESP-DL speaker example and the
esp_cam_sensor supported sensor list. Selected IDF 5.5.1, LVGL 8.3.11 and exact
managed dependency pins. Created architecture, partition/PSRAM budget, roadmap,
hardware/API/privacy/build/recovery documentation, CI and draft-release workflows.

Implemented serial diagnostics, non-destructive NVS init, an experimental DSI/PPA
LVGL port, GT911 inverse coordinate mapping, Polish font, touch-open menu, manual
brightness and inactivity return, plus optional version-gated C6 scan diagnostics.
The menu explicitly labels unfinished product features. No fabricated recognition,
time, weather, network connection or camera image is presented.

### Confirmed physical evidence

* esptool 4.12.0, COM5: ESP32-P4 rev. 1.0, 40 MHz crystal, 16 MB flash.
* Full original P4 flash backup completed, 16,777,216 bytes, SHA256
  d951fe69eeb2e297c75e76ad970bf5caeafd4b7a71ab88cdf6167f1bd6c47c15.
  Private backup remains in ignored local/original-p4-flash.bin; never upload it.
* Initial diagnostics image compiled, flashed and hash-verified. Serial boot:
  IDF v5.5.1; 32 MB PSRAM detected; SPI SRAM memory test OK; app_main reached.
* Initial default image initialized ESP-Hosted bookkeeping via its constructor.
  Final configuration disables the component in diagnostics/display profiles;
  C6 is enabled only in the explicit Wi-Fi profile. No C6 firmware was flashed.

### Verification completed so far

* CMake/MSVC host test: cover/crop aspect ratios, overflow, coverage over many
  dimensions, invalid inputs and touch corners passed (CTest 1/1).
* Python configuration suite: dependency lock parity, flash partitions and all
  Polish glyphs passed (3/3).
* clang-format 19.1.7 application checks and actionlint 1.7.12 passed.
* Diagnostic and first experimental display firmware builds passed. Final
  display profile with ESP-Hosted disabled also compiled and flashed successfully
  (0x8b070-byte app). Effective configuration checker passed.
* Display-profile boot: DSI panel initialized, two 480×800 native buffers allocated,
  PPA rotation registered, GPIO23 LEDC enabled, GT911 chip ID 911 read at 0x5D,
  and LVGL 800×480 task started. Initial free memory: internal 429123 bytes,
  PSRAM 31245164 bytes. Twelve-second capture contains no reset or display error.
  This is initialization evidence; visual/touch acceptance remains separate.
* GT911 driver warns "Unable to initialize the I2C address" with no INT line,
  then reads chip ID/configuration successfully. Functional touch remains to test.
* Local Git push had no noninteractive credentials. Published the same file tree
  through the authorized GitHub connector, then reconciled the local branch to it;
  no credentials were extracted. Draft PR: https://github.com/mpuchara/ESP32-P4-Home-Buddy/pull/1.
* Wi-Fi profile built locally (0xf1330 bytes), effective GPIO/profile check passed,
  and all three profiles packaged with public-only outputs and license notices.
* Physical C6 diagnostic: slot1 / 4-bit / 40 MHz SDIO initialized, ESP32-C6 and WLAN
  identified. Version mismatch reported; firmware-version RPC timed out and the
  app reported ESP_FAIL without rebooting the UI during the 30 s capture. Installed
  version remains UNKNOWN (do not interpret the transport's 0.0.0 as a real version).
  No scans/association or C6 writes occurred. Display-only P4 image restored.
* Initial GitHub run compiled all three images and passed host checks, then failed
  in post-build Git repository discovery. Replaced that lock guard with a direct
  before/after byte comparison and explicit workspace paths. Full rerun passed:
  https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39752014808
  (host checks and all three firmware build/configuration/package jobs).
  Verified implementation commit: 1b967073fceb8db5b143348d839a81fac8e5c84d.
* That documentation-only evidence update used [skip ci]; firmware/workflows were
  unchanged at that point. No production release or merge was made.

### Acceptance still open

Visible display, four-corner touch, backlight behavior, cold-boot/repeated-menu
stability remain pending physical observations. C6 transport is confirmed, but
firmware identification/compatibility must be resolved before scan/association.
Wi-Fi provisioning/association/DHCP are not implemented yet. Camera, microphone,
face/speaker inference, profiles/PIN, UBox transport/JPEG display, NTP, temperature,
MQTT, automatic brightness and OTA are not implemented.
72-hour stability: NOT VERIFIED ON HARDWARE. This is not a completed product.

Next gate: visible 800×480 LVGL and peripheral verification, then fullscreen UBox
snapshot transport and rendering. Follow docs/DEVELOPMENT_PLAN.md.

## 2026-10-22: missing text reported during physical display test

The operator observed a dark saver and, after touch, menu rectangles and a slider
without any labels. This confirms a visible display path and touch navigation,
but does not confirm slider brightness or complete four-corner calibration.

Root cause: hb_font_20 used compressed bitmap_format=1, while the resolved LVGL
configuration has LV_USE_FONT_COMPRESSED=0. LVGL therefore returns NULL for glyph
bitmaps even though their descriptors/codepoint maps exist. The earlier static
glyph-coverage test did not exercise decoding and did not detect this mismatch.

Regenerated the same Noto Sans subset with --no-compress (bitmap_format=0), keeping
the current LVGL configuration. Added a host regression using the real pinned
LVGL 8.3.11 font decoder with compression disabled. It rejects the old compressed
font at bitmap != NULL and passes the corrected font for ASCII and all Polish
letters. Added target startup probes for ASCII/Polish glyph descriptors and bitmap
pointers so this failure is reported through serial rather than silently accepted.

Corrected display firmware built (0x8b9e0 bytes), flashed/hash-verified on COM5,
and booted. Target log: "Font bitmaps ready: ASCII and Polish", then LVGL ready.
Host CTest: 2/2 passed; Python checks 3/3, formatting and actionlint passed.
The operator subsequently confirmed that saver text and menu labels are visible.
That follow-up exposed cramped text in the four menu tiles. Increased tile height
from 70 to 112 pixels, provided a 52-pixel title area and a separate bottom-aligned
status, reduced padding to fit the full status and moved the controls below.
The actual font advances give "W przygotowaniu" a 162-pixel width, fitting the
168-pixel content width. The operator then confirmed "Tak, kafle są czytelne":
the corrected saver/menu text and separated tile labels are verified on the
physical device. This does not establish slider operation or four-corner touch.
The font fix passed all jobs in CI run 39763215780. The final layout implementation
18d6dab752873a8236fa3ea2e34f7a9bf591f1c2 passed host tests plus all three firmware
build/configuration/package jobs in CI run 39763775980:
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39763775980.
The subsequent documentation-only receipt uses [skip ci]. The corrected display
profile remains installed on COM5; no C6 changes were needed for these UI fixes.


## Iteration 2: 0.2.0-dev, 2026-10-22

Inspected existing open/unmerged PR #1 and preserved its branch and working BSP,
font and coordinate transform. No change to HA_UBOX_bridge or C6 flash. Pinned
ESP-Hosted documentation distinguishes legacy station RPCs from newer metadata.
A P4-only probe got station start and scan ESP_OK despite the version RPC timeout;
first probe found 2 networks. Installed C6 version remains UNKNOWN.

Implemented PIN/NVS configuration workers, Wi-Fi selection/keyboard/password/DHCP/
reconnect/RSSI, original cached UBox HTTP/ETag/backoff, bounded hardware JPEG to
800x480 cover rendering, independent Warsaw SNTP clock, timestamped MQTT
Celsius input with stale/unavailable rules and an asynchronous test greeting.
Added offline synthetic JPEG and coordinate diagnostics, protected individual
settings and per-service system diagnostics. NVS is not encrypted at rest; secure
production storage remains incomplete. No biometric or simulated HA events.

Local Windows host CTest 3/3 passed (geometry, actual LVGL font decoder, dashboard
transfer/error/ETag/JPEG bounds/cover/time/actual cJSON temperature parser).
Python project checks 3/3 passed, clang-format check and actionlint passed.
Windows skips POSIX TZ transition assertions; Linux CI is required for those.
These tests do not emulate ESP-IDF sockets, hardware JPEG, UI buffer lifetime,
NVS power loss or a real broker/bridge. Those acceptance checks remain open.

Wifi firmware built without application warnings, size 0x17c8c0 bytes at first
iteration-2 flash, within the 6 MiB slot. Esptool 4.12 wrote P4 via COM5 and verified
hashes. Boot log reports LVGL ready, then scan ESP_OK / 8 records; no startup panic.
Internal free/largest/min 328135/245760/328135 and PSRAM free 31244760 at uptime 3 s.
The physical operator has been asked to verify JPEG/greeting/corners/brightness
and configure Wi-Fi on-device. Live DHCP/SNTP, UBox/ETag and MQTT acceptance are
pending. No production release or merge was performed. CI receipt follows when
iteration-2 jobs finish; baseline CI is not evidence for these new changes.


### Operator feedback and navigation/font correction

The operator tested the requested actions and explicitly reported missing keyboard/
text glyphs plus a restart after initial PIN creation. No separate JPEG/corners/
brightness success claim is inferred from that general response. Passive serial
log captured Load access fault followed by reboot; NVS retained the newly saved
PIN. Root cause: deleting the active LVGL page sets act_scr=NULL, then LVGL 8.3.11
screen loading sends its unload event through that old NULL screen. Switch to the
persistent saver before deleting a page. Added a regression using actual LVGL
screen/textarea/keyboard/theme code: 100 PIN/form/return cycles and stable pool
usage after the first cache allocation. It verifies the active-screen invariant.

Expanded the existing uncompressed Noto subset with typographic dashes, bullet,
ellipsis and minus. Use the already-enabled uncompressed Montserrat14 fallback
for LVGL keyboard arrows/backspace/OK/newline. Font tests resolve the actual fallback
font before checking its bitmap, matching LVGL's rendering behavior. CTest now
passes 4/4 locally. This addresses missing glyphs without enabling compression.

The operator explicitly changed the Wi-Fi requirement: direct scan/SSID/password,
no administrator PIN. Implemented a narrowly scoped Wi-Fi-only save preserving
UBox/MQTT settings and PIN. Protected UBox/MQTT remain as specified; Wi-Fi-before-
PIN saves use zero hash/salt and do not create a hidden default PIN on restart.
Wi-Fi dropdown now has a selection prompt, so the first real SSID can be selected,
and refreshes in place after scans. Form arrival resets idle time. Numeric and
UTF-8 byte limits reject truncation. Suppress the pinned upstream rpc_utils SSID
warning. System diagnostics now preserve separate service states/IP/RSSI and
accepted-image age; the synthetic frame has an explicit test marker. Serial capture
sets DTR/RTS before opening so passive reads do not request a reset.


### Physical acceptance and final operator override

The operator confirmed: Test JPEG OK, touch OK, greeting OK, brightness OK; Wi-Fi
connected and obtained an IP, and the clock showed correct local time. A passive
serial sample records JPEG1280x720 decode+cover156ms, internal free316471/largest
237568, PSRAM free28575716/largest28311552, aligned input51743, raw1843200 and
final768000 bytes. Subsequent idle PSRAM was30474708. These are synthetic-test
measurements, not UBox-download peak memory or a leak soak. The navigation
regression also fails against the former unsafe deletion (active-screen assertion)
and passes after restoring the correction.

CI run39770586133 passed host checks and all three firmware build/config/package
jobs at0754f36d7141b7ae66e5d1e349b62575d107b227, including Linux DST boundaries.
The operator then explicitly clarified that UBox/MQTT must have NO administrator
PIN either. This overrides the initial specification's PIN workflow. Removed every
PIN gate/hash/lockout and kept all credential fields masked. Existing NVS settings
stay compatible; the obsolete48-byte hash/salt area is ignored and zeroed on next
save. There is no default or hidden PIN. UI navigation regression remains relevant
to all transient settings screens. Production storage encryption is still open.

Added a20-second total HTTP transfer deadline in addition to8-second individual
calls, and reset the request validator while a synthetic frame is shown so the
next poll restores the real image. Set station-connected false immediately on
credential change rather than waiting for the remote disconnect event.
The final no-PIN build is0x17cbf0 bytes; its fresh CI/flash receipt follows.


##0.2.1-dev: operator-requested Open-Meteo, camera colors and visible token

The operator confirmed a live UBox image but reported severe wrong colors; asked
for Open-Meteo/manual locality instead of MQTT, and a visible long-token field.
Pinned IDF5.5.1 jpeg_decode.c configures RGB565 endian scrambling under rgb_order.
The previous RGB setting emitted big-endian565 while LVGL expects native little
endian. Use BGR for native565 and explicit BT.601. Flashed P4-only and the operator
confirmed corrected colors and visible token. General logs still omit secrets.
A passive log recorded UBox304 accepted=1, preserving the existing decoded frame.

Implemented Pogoda: typed locality -> up to5geocoded country/region matches ->
explicit selection -> persistent independent location record. Open-Meteo current
Celsius over certificate/hostname-verified HTTPS,10-minute polling,1-hour timestamp
freshness, bounded retry/response/depth/deadline and off-LVGL processing. No default
city or invented temperature. Attribution/model-data status appears in settings.
Removed MQTT client/dependency/UI/subscription and legacy payload parser entirely.
Old transport bytes remain opaque layout-compatible padding and are cleared on
save. Compile-time assertions preserve852-byte settings/900-byte old record.

Host CTest5/5passed, including new real-parser geocoding/weather/UTF8/unit/time/
depth cases; project checks3/3passed. Public Warsaw API probe downloaded1087-byte
geocoding and319-byte current responses, and the actual C parsers passed against
them. This public contract probe is not a configured default or hardware reading.
The weather-capable P4 firmware built at0x175130bytes and was flashed/hash-verified.
Device locality search/temperature acceptance was requested and remains pending.
Source cleanup after that flash removes the unused MQTT archive/dependency too.

Earlier no-PIN/static-font implementation210862dc749ce50fb45b45858b5138a7a798b28b
passed all CI jobs in run39771837517. It is not a CI receipt for0.2.1 weather work;
a fresh run is required. PR#1 remains draft/unmerged, C6 unchanged, no release.


### Device weather acceptance and SDIO mitigation

The operator explicitly confirmed locality search, correct selection and a
current temperature shown on the saver. No MQTT setup was needed. Camera colors
and the visible-token field were separately confirmed. Weather functionality is
now device-verified; stale/outage/reconnect/soak cases remain pending.

Later passive logs at40MHz captured sdmmc_send_cmd0x109/INVALID_CRC and a rejected
snapshotHTTP200, leaving the previous image. Selected20MHz in the Wi-Fi profile,
with an effective-config CI assertion; this is an attempted host-side signal/
transport mitigation, not a proven root cause or a C6 firmware update. Fresh
build/flash/boot evidence will follow. Private location and credentials are not
copied to these public logs/docs. No merge or production release.


###20MHz restart verification and JPEG layout regression

The final Wi-Fi build0x1751a0bytes passed effective-config validation and was
flashed/hash-verified on COM5. Esptool HardReset with USB handling captured a full
boot: actual SDIO20MHz, station retries then DHCP, certificate validated, Open-Meteo
HTTP200/complete and accepted=1/same_location=1. Saved weather settings restored
without re-entering locality. A subsequent sample decoded a real640x360 bridge
JPEG in125ms and accepted HTTP200 with expected/received101730 and complete=1.
Internal free273339/largest196608 and PSRAM free27800540/largest27787264 during
that decode; raw471040, aligned input101730, output768000 bytes. This is a short
restart sample, not long-term transport/reconnect/soak proof. Earlier transient
200rejections did not advance ETag or publish partial frames.

Added bounded per-stage HTTP diagnostics(expected/received/completion/decode error)
without URLs/tokens/image contents, and rejected-header dimension/envelope details.
Pinned jpeg_parse_sof_marker.c aligns process_h/process_v to the first component's
sampling factors:8x8 for444,16x8 for422,16x16 for420. The earlier unconditional16
padding could reject a valid non-16-aligned decoded size or use a wrong stride.
Use an actual SOF/MCU layout helper; regressions cover1920x1080 all three variants
and invalid sampling. Host tests5/5still pass. This fixes a verified source-code
layout bug; it does not assert that all earlier network rejections had that cause.

CI run39804200456 passed all jobs(host plus diagnostics/bringup/wifi) at17f7206.
The following layout/diagnostic change requires its own fresh CI receipt.


### Original-resolution image bound

A later short sample established a separate precise cause for repeated decode
rejection: the original cached bridge image changed from640x360 to2304x1296.
All679649bytes were received with HTTP200/complete=1 and a valid JPEG envelope;
the old per-dimension2048guard rejected it before hardware decode. This roughly
3MP source is within the existing4MP decoded-pixel budget. Increase only the
per-dimension bound to4096, retaining the2MiB JPEG and4MP pixel limits. Add a
2304x1296 layout regression and a4096x2048 rejection proving the pixel budget is
still enforced. Host tests5/5passed, firmware built0x1754d0bytes. Device acceptance
and final CI for this bound adjustment follow separately. No low-resolution UBox
endpoint or capture call was introduced.


## Final iteration-2 receipt:0.2.1-dev

Implementation dd3d4443d3f3df3f7a5b1b09172ad261f976ccde passed every job in
CI run39807653198: host tests/checks and diagnostics, bringup and wifi firmware
build/configuration/package jobs. This includes five CTest suites, three Python
project checks, formatting, whitespace, dependency lockfile and effective SDIO
20MHz assertions; Linux executes the Warsaw DST boundaries. The local five CTest
suites also passed with final sampling/full-source dimension regressions.
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39807653198

Final Wi-Fi firmware0x1754d0bytes was flashed/hash-verified on COM5. It booted at
actual SDIO20MHz, retried station association and obtained DHCP, restored saved
locality, validated Open-Meteo TLS and accepted HTTP200/current data. It accepted
complete original640x360 UBox frames(expected/received104438 then later103160bytes),
with approximately126ms decode+cover and repeated conditional304 accepted. Idle
PSRAM samples remained30474708bytes between these operations. This is a short
sample, not a leak/fragmentation/stability soak. No CRC was logged in that final
short sample; absence there does not prove the mitigation eliminates the issue.

The2304x1296 source caused the earlier reject and is now covered by the expanded
side bound, unchanged4MP/2MiB budgets and host layout regressions. The final short
post-fix hardware sample happened to contain640x360 frames; a fresh2304x1296
hardware frame remains a separate acceptance item. Polling still uses only the
original cached snapshot route and never initiates an LTE capture to force a test.

Public-only local package:dist/iteration2-final, with SHA256SUMS; app SHA256
9e75f39c429fc24531fb9eae3e71aa4d36f28f4b9b8666d187995eed226ac002.
It contains app/boot/partition/OTA-initial/debug/license outputs, not NVS, private
backups, user photos or credentials. A documentation-only receipt follows this
verified implementation with[skip ci]. PR#1 stays draft and unmerged. No release,
C6 write, HA_UBOX_bridge modification, biometric inference or active MQTT.

Next acceptance work: a naturally available full-resolution frame, sustained
connection/error recovery and stale-weather tests, repeated replacement heap/
fragmentation checks, cold power-on and the planned72-hour physical soak. Secure
production storage/provisioning remains a later task; do not infer it from masked
Wi-Fi fields or direct setup. The operator-requested UBox token edit is visible.

## Iteration 3 in progress, 2026-10-22

Baseline/PR inspected: clean06f8a11, PR1 open/draft/unmerged. Original private P4
backup SHA256 rechecked unchanged. No C6 write or bridge modification.
Read-only SCCB identified0x5602 at0x36. GUITION schematic/source/per-file Apache
license audit recorded in camera/vendor/SOURCE.md; no ESPHome runtime imported.
Native CSI one400Mbps lane /1288x728 RAW10 -> ISP RGB565 measured29.9..30.1fps.
Three leased slots + discard coexist with retained UBox, weather TLS and DHCP.
Host six tests and Python three checks passed at the first UI implementation.
Operator reports green/pale and~1fps preview, so image quality is explicitly NOT
accepted. Correcting pinned-driver cache length, preview pace and preview-only
white balance/display gamma. Brightness controls and schema1 separate persistence
implemented; actual day/night/override/reboot acceptance pending. See LOCAL_CAMERA
and BRIGHTNESS for algorithms, relative-metric limitations and face-model research.

## Iteration 3 RAW8 and upright preview receipt, 2026-10-22

The initial RGB565 image path was rejected by the operator. Native RAW10->RAW8
GBRG, black-target subtraction and thumbnail-only white balance/gamma were then
operator-confirmed for usable colors and detail. Head initially pointed right;
CCW90 portrait preview now has operator confirmation of upright orientation with
the device in landscape. Synthetic four-quadrant regression verifies the mapping
and unchanged source Bayer storage. Frame API declares rotation_ccw=90 for future
AI preprocessing; no actual inference or recognition accuracy is claimed.

Pinned CSI source has two build-local DMA/cache corrections with exact source
hash admission. Native RAW10->RAW8 requires output-bit transfer count to avoid an
overrun. ISP constructor/uncertain cleanup failures quarantine bounded allocations
until reboot rather than repeatedly claiming/leaking driver resources.

Final local host checks: six CTest suites, three project checks, formatting and
whitespace passed. Raw linear light uses unquantized channels and rejects clipping
or near-total black; black-level-only data remains invalid. The connected short
sample still reported luma0/invalid at nominal32ms and8x gain. Thus actual local
illumination measurement, day/night calibration and AUTO behavior are NOT accepted
from the preview confirmation. Manual operation remains available; no false anchor
is written. Real calibration, NVS reboot restore, five-minute temporary expiry,
changing-light stability and fault injection remain on-device acceptance work.

Upright firmware built0x17db60bytes and was written/hash-verified on COM5. Capture
measured about30fps. At158..178seconds PSRAM stayed26,723,652bytes, largest22,544,384;
internal DMA265,431 then265,219bytes. These are short samples, not a leak/72-hour
soak. Earlier RGB preview measured7fps; the final portrait rate still needs a fresh
active-preview sample and is not equated to30fps capture. Boot temporarily logged
network host-unreachable; no uninterrupted connectivity claim is made.

No C6 firmware write, bridge modification, forced LTE snapshot, model download,
biometric enrollment, production release or PR merge occurred. Naturally available
2304x1296 UBox acceptance, cold-power boot and planned72-hour soak remain pending.

Operator follow-up: local light now shows OK with a numeric value that changes
when more light reaches the camera. Thus illumination responsiveness is physically
confirmed; the earlier black short samples do not establish a camera/meter fault.
This confirmation does not replace real day/night anchors or AUTO/persistence tests.

Final local package: dist/iteration3-upright-final, public build outputs and
SHA256SUMS only. App SHA256:
c6e376dd90fefe575e982be7e2e7838de8543a865a42a5dd568c0d2b49a82631.
All three local firmware profiles build and pass effective configuration checks.
Six host suites pass on Windows; the camera/brightness suite additionally passes
GCC -Wall -Wextra -Werror under Ubuntu. Linux CI host also passes after signed
PWM test intermediates avoid platform-specific type-limit/absolute-value warnings.

Final implementation d4af598bded38d8eda0ce220565255fe09bb6ed4 passed all four
jobs in CI39839957879: host suites/checks and diagnostics/bringup/wifi clean
firmware builds, effective configurations and public-only packages. This includes
the project-local CSI adapter against the pinned Linux IDF source. CI receipt:
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39839957879
The following documentation-only acceptance receipt uses [skip ci]. PR1 remains
open/draft/unmerged. Original private backup hash was rechecked unchanged.

## 0.3.1-dev: explicit temporary override and persistence regression

Continued clean ed7fa2e on the existing draft PR1. Found that an open brightness
form retained OVERRIDE after the worker returned to AUTO; editing an option or
saving that stale snapshot could start another five-minute interval. Only explicit
temporary-mode selection now starts/restarts a deadline. Live edits/saves preserve
an active deadline and normalize expired overrides to AUTO. The form follows
expiry, and controls ignore edits while the configuration worker commits a save.
Existing calibration/schema, GPIO/PWM, camera orientation and dashboard remain.

Added real brightness-service host integration against deterministic NVS/clock/
OS boundaries: defaults/restart, both anchors, absent/short/oversized/invalid
records preserved without writes, NVS errors without applying RAM settings,
expiry/stale save, explicit restart and no per-tick flash writes. This does not
claim physical NVS transaction rollback or real day/night calibration acceptance.

0.3.1 local verification: CTest7/7 and project checks3/3 passed; clang-format,
actionlint and diagnostics/bringup/wifi effective configuration checks passed.
The new real-service suite also passed GCC -Wall -Wextra -Werror under Ubuntu.
All three local profiles built: diagnostics0x378b0, bringup0x134330,
wifi0x17dcb0bytes. Wi-Fi image was flashed/hash-verified on COM5 and booted as
0.3.1-dev. Private originals/NVS were not erased; no C6 or bridge operation.

Short post-flash sample: camera around30fps alongside accepted UBox304 responses.
At73..103seconds PSRAM26,723,652bytes and largest23,592,960 stayed constant;
DMA265,395 then265,183bytes. No CRC/panic appeared in this short sample; it is not
an extended stability/leak result. Manual five-minute expiry/options behavior is
awaiting operator confirmation; no anchors were manufactured or changed for tests.

Public-only package: dist/iteration3-override-031 with SHA256SUMS. App SHA256:
053e9d723d3f69c28bd4ef226012c530396d55449bef168d0dba35ffdf51fd5c.

Final implementation48661d1ef313f62fb835db070410f12eb05c9eea passed all four jobs
in CI39842183267: Linux host7suites and all three clean firmware/configuration/
package jobs. https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39842183267
Package hashes were rechecked. Documentation-only receipt follows with [skip ci].
PR1 remains draft/unmerged; real two-anchor calibration, physical reboot persistence,
cold-power and extended stability/72-hour acceptance are still pending.

## Operator acceptance, 2026-10-23

Operator confirmed that0.3.1-dev temporary brightness returned to AUTO after the
five-minute interval on the physical unit. The reply did not separately confirm
editing Pomiar/Przejscie after expiry; retain that physical check as pending.
Real day/night anchors, reboot persistence and extended stability remain pending.

## 0.3.2-dev camera recovery hardening

On successful driver stop, invalidate the published latest frame and abort pending
writers while retaining external reader leases. A restart cannot hand new readers
a pre-recovery frame. Existing consumers may release safely; their storage stays
ineligible for DMA while held. Failed-length/duplicate completion does not count
as a published frame. The CSI adapter remains mandatory; the pinned SDK reports
configured completion size, so these checks alone do not detect every corrupt
hardware/MIPI payload.

Extract the owner's3s progress watchdog and5..300s retry policy for deterministic
host fault testing. Thirty seconds of continued frame progress restores5s retry.
A quarantined driver is visibly marked RESTART; planned retries are not claimed
when driver lifetime is uncertain. Original capture buffers, raw orientation,
local light, backlight anchors and UBox/Open-Meteo remain independent.

0.3.2 verification: CTest8/8 and Python project checks3/3 passed locally, as did
formatting, whitespace, actionlint and all three effective-profile checks. The
recovery suite also passed GCC -Wall -Wextra -Werror under Ubuntu. Local firmware
sizes: diagnostics0x378b0, bringup0x134470, wifi0x17dde0bytes. P4 COM5 image was
flashed/hash-verified and boot log confirms0.3.2-dev. No C6 firmware write, bridge
change, fabricated light anchors or private image publication.

A naturally available original UBox2304x1296 image was received after initial
network retries: HTTP200, expected=received746465bytes, complete=1, decodeESP_OK.
Hardware JPEG decode plus cover took218ms; aligned decoded buffer5,971,968bytes,
output768,000bytes. At the logged allocation point: internal free290,651bytes,
largest180,224; PSRAM free17,897,292, largest17,825,792. The local camera continued
around30fps, with free PSRAM26,723,652 after decode cleanup. This verifies complete
full-resolution download/decode/cover on the P4; operator visual acceptance,
repeated full-size replacement and long-run fragmentation remain separate checks.
No forced LTE capture or new bridge endpoint was used.

Final implementation19e88f082fd80f7b662d70c135844c217d1b0c9d passed all four CI
jobs (host8suites plus diagnostics/bringup/wifi clean builds/config/packages):
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39898488143
Public-only package dist/iteration3-recovery-032 has SHA256SUMS; app SHA256:
e1eaeaec9965180cc8d357b7a8e8de9051acc7356e76a68c9419629b7f3eda4c.
The following acceptance receipt is documentation-only [skip ci]. PR1 stays draft.

A second naturally updated2304x1296 image followed at about137s: complete739,606
bytes, HTTP200 accepted,295ms decode+cover. During its allocation log, PSRAM
17,127,240/largest11,796,480, internal289,555/largest180,224. After replacement,
free PSRAM returned to26,723,652 and largest23,592,960. This establishes two full
transfers and one natural replacement with sampled memory return; it is not a
long repeated-swap leak/fragmentation soak. Capture stayed around30.1fps, and one
local light reading was valid. Operator visual/fullscreen/flicker acceptance is
still pending. No CRC/panic appeared in the collected short samples.

## Operator follow-up and0.3.3-dev night calibration fix, 2026-10-23

Operator confirmed the natural full-resolution UBox image fills the screen with
correct colors and no visible bars/flicker. This adds visual acceptance to the two
complete2304x1296 transfers and one sampled natural replacement.

Operator reported that valid flashlight readings were negative, whereas ordinary
darkness showed0.000 and refused Kalibruj noc. Near-black rejection was conflating
genuine low illumination with no usable sample. Introduce a flagged dark reference
only at settled configured AE ceilings with valid metadata; preserve clipping,
staleness, missing-data guards and safe PWM. The finite reference is the5-DN dark
band boundary, not measured lux or invented anchors. Preview/metric scale, saved
anchors/schema and UBox processing remain unchanged. UI shows unavailable text
without0.000 and distinguishes the dark threshold. Actual night save is pending.

0.3.3 local checks: CTest8/8, Python3/3, formatting/whitespace and effective
configurations passed. All three local profiles built: diagnostics0x378b0,
bringup0x134740, wifi0x17e0b0bytes. Updated meter/brightness regressions passed
GCC -Wall -Wextra -Werror under Ubuntu. Final image was flashed/hash-verified on
COM5; boot confirms0.3.3-dev. In ordinary dark samples at the configured limits,
light_valid=1, below_range=1, metric=-15.649, actual luminance0, exposure~32078us,
analog8x/digital1x. Capture continued about30fps. Thus fresh dark-reference
admission is observed on P4; UI night-save acceptance is still pending operator.

Public-only package dist/iteration3-dark-033 hashes were verified; app SHA256:
d20a494759674289212a347f7c87dd757a4adf490398e98ed08d86f5f4c085c4.
No C6 write, bridge change, NVS schema change or manufactured anchor was performed.

Final implementation28bd6c26ec6223b2d40c818321cd1ef8d9712f9a passed all four
CI jobs: Linux host8suites and diagnostics/bringup/wifi clean build/config/package.
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39899761414
Documentation-only verification receipt follows with [skip ci]. PR1 stays draft
and unmerged. Real day/night save/reboot/AUTO transitions remain acceptance items;
full-size UBox visual acceptance is now operator-confirmed.

## 0.3.3 operator calibration follow-up, 2026-10-23

Operator confirmed Kalibruj noc saved a comfortable night level without error.
The form showed MANUAL, as designed for live slider selection. A second day/test
anchor was saved using a flashlight; after selecting AUTO, the display visibly
dimmed in the still-dark room. This physically accepts night-save,
two-anchor save workflow and AUTO dimming for those test anchors. The flashlight
point is provisional, not representative daytime calibration. Actual daytime
anchor, explicitly persisted AUTO mode and physical restart restore remain to be
verified. No firmware/configuration write was performed by the agent for this
receipt; operator-selected values remain unchanged. Documentation-only [skip ci].

##0.4.0-dev experimental face diagnostic, 2026-10-23

Continued clean6e8e231 on draft PR1. Audited pinned Espressif model/runtime files
and licenses. Added explicit local detection toggle, bounded4boxes, coordinate
clipping/expiry/cancellation, correct portrait->4:3 input and lazy model resources.
The model runs outside LVGL with raw leases released first. Added static compute
mutex for JPEG/model work, conservative admission and cleanup. No identities,
profiles, templates, HA events, cloud inference or runtime model download.

Nine local host suites and static project checks pass; the face policy additionally
passes GCC -Wall -Wextra -Werror under Ubuntu. They test admission/geometry/expiry,
not neural accuracy. Wi-Fi app initially builds0x38d8b0bytes. Hardware model load
and detection acceptance are pending explicit Test twarzy activation.


## 0.4.0-dev build and boot receipt, 2026-10-23

Implementation 84a0cb0c68cc1f9dfc26c8f47084d977de9cb24e passed all four CI jobs:
Linux host nine suites and diagnostics/bringup/wifi clean build/config/package.
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39902790684
All three local builds and effective configurations pass, with dependencies.lock
unchanged: diagnostics 0x378d0, bringup 0x344090, wifi 0x38d8b0 bytes. These retain
the existing 6 MiB application slots. Public-only packages are in
 dist/iteration4-face-040, dist/iteration4-face-040-bringup and
 dist/iteration4-face-040-diagnostics; their 60 file hashes were verified.
Wi-Fi app SHA256: 4431b01e355250c27a2cc43199391f657687ed20a1fdc055dab488aec5d10b0f.

The final Wi-Fi image was flashed/hash-verified on COM5. Boot confirms 0.4.0-dev;
passive samples show camera about 30 fps and a naturally cached 2304x1296 UBox
JPEG (857,018 bytes) accepted, decoded/covered in 224 ms. After image cleanup,
sampled free PSRAM was 26,723,652 with largest block 23,592,960 bytes. Detection
was disabled: there is no captured model load/inference/cleanup evidence yet.
The operator has been asked to activate Test twarzy in good light and report the
box/count/time and return to screensaver. Face correctness, initialization memory,
inference latency and coexistence under load remain explicitly unverified.

No calibration/NVS schema change, C6 flash, bridge change, private-image upload,
merge or production release. Existing night calibration acceptance remains valid;
representative daytime anchor and physical persistence test are still pending.
Documentation-only verification receipt uses [skip ci].


## 0.4.1 accuracy-oriented face diagnostic, 2026-10-23

Operator confirms 0.4.0 detects their face below 40 ms, with correct camera image,
but frequent misses in a dim-looking image with glasses/beard/dark clothes and
background. This is operator timing, not an agent-captured inference trace. Return
to screensaver and model cleanup are not explicitly confirmed by this reply.

Replace fixed MSR160x120 (portrait about68x120) with ESPDet416 and a 234x416
portrait input, preserving aspect via SDK letterboxing. Keep score >=0.7, freshness,
explicit activation, frame leases and resource admission; no synthetic presence
hold or lower threshold. Host regressions cover mapping/extreme coordinates.
Sensor AE, exposure limits and calibrated light/brightness path remain unchanged.
Improvement in this room is pending a physical comparison, not claimed from the
upstream benchmark. Dependencies/source pins remain unchanged; selected weights
and their checked hash are updated. No enrollment or automation events added.


## 0.4.2 completion-aware diagnostic freshness fix, 2026-10-23

Actual ESPDet416 load delta: PSRAM1,862,120/internal29,328 bytes including194,688
input. Inference samples757-803 ms; the network usually returns one face in the
sample, while operator reports only AI: czekam. Cause: the old750ms source age
expired before inference completed. Add completion timestamp and require capture
<=completion<=now, completed age<=1200ms and absolute source age<=2500ms. A newer
zero result clears boxes immediately; this is not identity/presence hysteresis.
Host tests reproduce800ms processing, completion expiry, absolute expiry and
invalid/future completion. User-visible improvement/cleanup remain pending.


Operator accepts 0.4.2 visible rectangles and substantially improved consistency,
usually700-800ms, with occasional short waiting captions. A second attempt shows
AI: brak pamieci. The log proves model/input cleanup at59573ms, free PSRAM returned
to26,723,652. Internal idle free281,495/largest102,400 bytes. At110873ms policy
refused before load because the old largest-block guard required128KiB, despite
measured load delta~30KiB. Tune only loading largest-block admission to96KiB;
retain192KiB free,64KiB observed load cap and unchanged PSRAM/inference guards.
No memory allocation exception or panic was observed. Test boundary96KiB/96KiB-1.
Repeated open/close acceptance is pending; no model arena cache is introduced.


## Final 0.4.2 verification receipt, 2026-10-23

Implementation791f8d9a252ec1edd5f5c9d93df62780c538f6ff passes all four CI jobs
(host9suites; diagnostics/bringup/wifi clean builds/config/package):
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39913963248
All three local profiles and effective configurations pass: diagnostics0x378d0,
bringup0x3a0d30, wifi0x3ea540 bytes. Dependency lock unchanged. Python3/3,
format/whitespace and GCC -Wall -Wextra -Werror face regressions pass. Public-only
packages dist/iteration4-face-042-reload{,-bringup,-diagnostics} contain retained
licenses and60 verified hashes. Final Wi-Fi application SHA256:
90a89790b4491a1a09c33a7e1193a0f72805555016ae182fe97d7853a8aca5ff.
This image was flashed/hash-verified and booted on COM5; no C6/bridge/NVS change.

Operator confirms substantially improved face detection, typically700-800ms,
and after the reload fix confirms detection works again on reentry from the
screensaver. This accepts at least one reopen, not an explicitly counted three-
cycle test or sustained soak. Occasional waiting captions persist for about a
second: only fresh completed results are shown. They are not negative detections.
No automation/identity/presence semantics are inferred from these diagnostics.

Captured latest-image model load delta is1,862,120bytes PSRAM/internal29,340bytes.
A short session runs about763-810ms per model call; cleanup explicitly logs released
allocations and free PSRAM26,723,652 at80402ms. A subsequent natural UBox640x360
JPEG decoded/covered in142ms. Full-size UBox simultaneous model allocation,
long fragmentation tests and labelled accuracy/false-positive evaluation remain
unverified. No biometric image/template or private log is committed. PR1 remains
draft/unmerged. Documentation-only receipt [skip ci].


## 0.4.3 diagnostic temporal presence, 2026-10-23

Continue clean464b71c with a scalar, fixed-memory presence tracker on accepted
ESPDet416 results. Two distinct consecutive positives enter PRESENT; consecutive
negative results need both four analyses and5s since the first negative capture
to enter ABSENT. >3s without accepted completion gives UNKNOWN and clears old
streaks on recovery. Stop/re-enable/error reset; timestamp/duplicate/sequence-wrap
rules reject recycled/out-of-order evidence. Pure status reads cannot add votes.

Diagnostic LVGL page shows stable presence separately from fresh boxes. Compress
camera diagnostic text to reserve space for the new two-line status. Existing
30s idle timeout and page-scoped AI remain. Publication now checks epoch inside
the status critical section, preventing cancelled results entering reset state.
No persistent records, identities, greetings, background AI or HA events.

Initial local10/10 host suites, Python3/3 and format checks pass. Physical transition
behavior/layout and firmware build/CI are pending; no occupancy accuracy claim.


## 0.4.3 build/boot verification receipt, 2026-10-23

Implementation7cc3c7d66092c78f7503c22aae301130e8dc271a passes all four CI jobs:
Linux host10suites; diagnostics/bringup/wifi clean build/config/package.
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39926305385
All three local profiles and effective configurations pass: diagnostics0x378d0,
bringup0x3a11d0, wifi0x3ea9d0 bytes, unchanged6MiB slots. Python3/3, formatting,
whitespace and GCC -Wall -Wextra -Werror temporal policy tests pass. Dependency
lock unchanged. Public-only dist/iteration4-presence-043{,-bringup,-diagnostics}
packages retain licenses and60 verified hashes. Final Wi-Fi app SHA256:
70f8c1f16e54fe73464fddf0a91c39c8286a3f652b42747529d13cb545d397a0.

This image was flashed/hash-verified on COM5 and reset. Boot confirms0.4.3-dev,
Wi-Fi association and camera around30fps. Passive samples show cached UBox304
accepted and idle PSRAM26,723,652/largest23,592,960 bytes. No active face/presence
session was captured in these passive samples. After the build/boot checks, the
operator explicitly confirms the requested workflow: two detections -> TAK,
brief turn/miss tolerated, leaving the frame for>5s -> NIE, return -> TAK, and
readable new labels. This physically accepts those transitions and layout; exact
hardware boundary timing, longer inference/cleanup and repeated sessions on0.4.3
remain separate checks. The30s inactivity timeout remains.
Do not infer labelled occupancy accuracy from temporal host tests.

No continuous background detector, identity, enrollment, HA event or automatic
greeting; no C6/bridge/settings-schema/calibration change. PR1 remains draft and
unmerged. This is a documentation-only receipt [skip ci].


## 0.4.4 bounded background diagnostic, 2026-10-23

Continue clean6c7f12a. Iteration3 requires face processing to remain diagnostic,
so add an explicit10-minute RAM-only background test, not default-on production
presence or HA actions. A monotonic lease and independent page owner share one
worker/model; close cancels page only. Effective demand transitions reset epoch
and presence; uninterrupted ownership handoffs preserve them. Expiry is checked
by worker and inside publication lock; status reads never renew it. Start after
unprocessed expiry first clears old demand. Errors cancel both owners.

Add Obecnosc w tle page with explicit start/stop, countdown and status. Keep saver
unchanged. Background-only start period1500ms, foreground500ms; existing resource
admission, compute mutex, image limits and calibration remain. Color test cancels
both owners. Logs add scalar owner/countdown/period for physical verification.

Local11/11 host suites, Python3/3 and format pass. face_demand covers ownership,
exact expiry/renewal, default OFF, overflow and clock anomalies. Builds, CI,
physical background inference/JPEG coexistence and expiry remain pending.


## 0.4.4 build/boot verification receipt, 2026-10-23

Implementation299f7dd021efdce7eb557dbf55cf1bec9ee9a4db passes all four CI jobs:
Linux host11suites; diagnostics/bringup/wifi clean build/config/package.
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39937908256
All local builds/effective configs pass: diagnostics0x378d0, bringup0x3a1850,
wifi0x3eb060bytes, unchanged6MiB slots. Python3/3, format/whitespace and GCC
-Wall -Wextra -Werror demand tests pass. Dependency lock unchanged. Public-only
packages dist/iteration4-monitor-044, dist/iteration4-monitor-044-bringup and
 dist/iteration4-monitor-044-diagnostics-final have60 verified hashes and byte
parity with completed current build outputs. Final Wi-Fi application SHA256:
71a97dcfa8c607f843d180274ad326db3f27d3e58cf77b2f547f3ca3a9078f48.
An earlier diagnostics staging copy preceded final build completion; it is
superseded by the explicitly verified diagnostics-final directory.

Final image flashed/hash-verified on COM5; boot confirms0.4.4-dev, Wi-Fi and camera
about30fps. A passive sample accepts a natural2304x1296 UBox JPEG (1,034,334-byte
staging), decode/cover226ms. At allocation log free PSRAM16,832,312/largest16,252,928,
internal286,531/largest180,224. No monitor session appeared in these samples.
Thus monitor-on-screensaver, simultaneous model/JPEG, manual stop/release, handoff,
repeat starts and physical10-minute expiry remain unverified. The operator has
been asked to start, return to UBox for20s, inspect countdown/presence, then stop
and confirm readable layout. Host deadline tests do not prove RTOS/device timing.

Default monitoring OFF, no persistent config/NVS/calibration/C6/bridge change,
identity, HA event or automatic greeting. PR1 remains draft/unmerged. This is a
documentation-only receipt [skip ci].


## 0.4.4 operator background-trial follow-up, 2026-10-23

Operator confirms starting the countdown, returning to the UBox screensaver for
more than20s, reopening Obecnosc w tle with the countdown still running, then
Zatrzymaj switching the trial off. All labels are readable. This physically
accepts the bounded trial's survival across page/saver navigation,
ongoing countdown display, manual stop UI and layout. The reply does not explicitly
confirm presence accuracy during background operation, heap release or a new
full-size JPEG decode concurrent with inference; do not infer those observations.
Actual10-minute expiry, reboot default-OFF, repeat starts and longer combined
stress remain pending. No firmware write/configuration change for this receipt.
Documentation-only [skip ci]; existing code CI remains unchanged and green.


## 0.4.5 face-image quality diagnostic, 2026-10-23

Continue cleaneb620ec. Add per-box size, mean luma, contrast, neighbouring-luma
detail and near-limit fraction on the retained234x416 processed detector input.
Bounds/capacity checks and64-bit accumulators avoid invalid access/overflow;
exact variance numerator avoids float cancellation on constant maximum ROIs.
Up to four scalar results, no crop allocation/image persistence/new model.

Add Jakosc twarzy page with explicit start/stop and largest-face metrics. Hint
thresholds are provisional, not calibrated focus/readiness or identity confidence.
No filtering of accepted detection/presence and no exposure/brightness/storage
change. Model and metric timings are separate; completion follows quality work.
Existing foreground/background ownership, deadlines and memory guards remain.

Local12/12 host suites, Python3/3 and formatting pass. Host tests cover arithmetic,
smoothing response, clipping/capacity/extreme coordinates and accumulator limits.
Firmware/CI and physical metric/layout/overhead checks are pending. No alignment,
embeddings, enrollment, identification or HA event added in this diagnostic step.


## 0.4.5 build/boot and measured quality overhead, 2026-10-23

Implementationbb2107fcda897a5cfdb91aa15d1d91dd572758bf passes all four CI jobs:
Linux host12suites; diagnostics/bringup/wifi clean build/config/package.
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39943561382
All local profiles and effective configs pass: diagnostics0x378d0, bringup0x3a2400,
wifi0x3ebbe0bytes, unchanged6MiB slots. Python3/3, format/whitespace and GCC
-Wall -Wextra -Werror quality regressions pass. Dependency lock unchanged. Public
 dist/iteration4-quality-045{,-bringup,-diagnostics} has60 verified hashes and byte
parity with final build outputs. Wi-Fi application SHA256:
086d64c3ad2bfc4fb533267493d2cbcbf3b8791985b9d93e1d606e2ca60318de.

Final image flashed/hash-verified on COM5 and reset. Boot confirms0.4.5-dev,
Wi-Fi and about30fps capture. A natural640x360 UBox JPEG decoded/covered in139ms
before the sampled quality session. This is not a simultaneous full-size JPEG test.
Actual model load: PSRAM delta1,862,120/internal29,368bytes including input. A
66-iteration diagnostic sample contains63 nonzero-face results; this is unlabelled
telemetry, not an accuracy benchmark. On nonzero-face samples, quality postprocess
ranges5,789..29,946us, median17,323us. Model run ranges553,472..749,475us in that
sample, separately from quality. The quality page has no live preview, so this is
not a controlled speed comparison with the previous camera-preview UI. Camera
capture remained about29.9fps in the sample. No extra ROI allocation is introduced.

Metric arithmetic/bounds and actual processing are observed. Operator subsequently
reports the visible largest-face readout:150x190px, mean126/255, contrast66,
detail7.3, near-limit pixels4.3%, quality19000us and model578ms. These accept
displayed metric availability and agree with the sampled timing range; they do
not establish focus/identity suitability. Distance-response, explicit full-layout
acceptance, repeat sessions and long combined load remain pending. Existing presence/monitor calibration and resource gates are retained;
no image/template logging, enrollment, identity, HA event or exposure change.
PR1 stays draft/unmerged. Documentation-only receipt [skip ci].


## 2026-10-23: 0.5.0-dev experimental local household identity

Implemented ESPDet/native RAW8 ROI -> MNP -> upstream MBF alignment/features,
RAM-only stable-ID profiles, staged touchscreen enrollment, top-three different-
person matching, three-frame identity confirmation, bounded opt-in recognition,
exact-frame diagnostics and internal FACE/presence events. No partition edits or
biometric persistence. MBF generic file uses local microSD with digest verification.
Dependency resolver preserved ESP-DL3.3.14 and all existing pins, adding recognition0.3.3.
Host13/13 and sanitized identity-core tests passed. Final firmware/CI receipts and
all hardware/security acceptance limits are tracked in IDENTITY_VALIDATION.md.
No recognition execution/accuracy/latency or new physical UI result is claimed.


## 2026-10-23: Iteration 6, onboard-only development candidate

Code b4f8bc5 implements verified onboard MBF loading, gated two-bank profile
persistence, independent bounded identity events and MQTT 5 Home Assistant.
CI run 39983216326 passed host and all four firmware profiles for that exact SHA.
Local host 15/15, real-model Python validation, ASan/UBSan and four build/config
checks passed. Recognition image: 4,438,640 bytes; model PDL3: 3,522,944 bytes.
The security bootloader assessment requires table offset 0x10000, with private
NVS/PHY relocation. Full backup and relocation preparation were read-only.
No new device flash, migration, erase, eFuse/security provisioning, C6 update,
merge or production release. Hardware MBF/accuracy/memory, persistence security,
MQTT/TLS/HA and rollback acceptance remain incomplete. This supersedes historical
microSD deployment instructions above. Full receipts: ITERATION6_REPORT.md.


## Iteration 8 continuation

2026-10-24: iteration 8 continues 6dab65c on codex/local-face-recognition. Baseline CI 40118442005 succeeded. Installed 00272f7 remains untouched, preserving RAM enrollment. ITERATION8_REPORT.md separates candidate validation and physical gates.


## Iteration 10, 2026-10-25

Iteration 10 implements 0.9.0-dev foundations and fixes pre-dispatch audio Stop leaving busy latched. Added production-control-flow fault injection, optional isolated speaker provisioning and passive soak telemetry. No board writes or new physical acceptance. Final receipts: ITERATION10_REPORT.md.


## Iteration 11, 2026-10-25

0.10.0-dev continues from 15f7fd0 on codex/local-face-recognition. Added reference log-mel/CMVN/quantization regression, household voice enrollment/identification ownership, stale-session fusion rejection and a separate schema-2 engine with migration/torn-write/corruption/scrub tests. Evaluated pinned standalone libfvad with reproducible synthetic probes; not admitted into firmware. No board reset, Flash write, migration activation, security provisioning or hardware measurements. Exact validation is in ITERATION11_REPORT.md.

## 2026-10-25: iteration 12, 0.11.0-dev

Implemented explicit local unencrypted schema-2 runtime, automatic save/restore,
retained AUTO preference, privacy acknowledgment and retry UI. Preserved partitions,
USB boundaries and experimental voice gates. See ITERATION12_REPORT.md for validation.
No board flash, reboot, migration, eFuse or C6 operation was performed.

## 2026-10-26: captured enrollment reboot and 0.11.1-dev stack fix

Physical reproduction captured an LVGL Stack protection fault, matching ELF
576394fb6, at the automatic receipt-to-enrollment transition. Replaced nested
status snapshots with independent fixed page/refresh/preview buffers and added a
target GCC stack-usage build guard plus regression for captured old frames.
Python 16/16 and CTest 32/32 passed; target UI frame subtotal reduced from 7,360 B
to 2,736 B. Corrected physical acceptance is tracked separately; no acceptance is
inferred from host tests. See ITERATION12_STACK_FIX.md.

## 2026-10-26: 0.11.2-dev, interrupted first-write recovery

Owner completed enrollment after the stack fix, but persistent storage was blocked.
An owner-authorized read-only USB snapshot found a blank lower bank and a partial
upper payload with no commit header. No erase/repair was performed. Fixed the
eleven-item advanced menu and premature create receipt; added storage result
metadata and an offline-default, explicitly authorized, backup-bound recovery
helper. Evidence and test boundaries: ITERATION12_STORAGE_RECOVERY.md.

## 2026-10-26: physical persistence and recognition preference confirmed

Installed 0.11.2-dev (88cfe34), application-only COM5, readback verified. Following
separate explicit approval, the recovery helper compared the current partition
with the private backup, erased only the incomplete upper bank and verified both
banks blank. The owner then confirmed the newly enrolled member remained stored
after restart and recognition remained enabled. This confirms that physical
persistence/preference session; greeting/HA delivery, numeric-ID continuity,
accuracy and physical peak resources remain separately unconfirmed. The existing
USB candidate is unchanged; no further flashing or restart was performed to
record this documentation update.
