# Iteration 13: persistent identity stabilization

Date: 2026-10-26. Installed candidate: **0.11.8-dev**; final-source transactions and restart verified; 0.11.4/0.11.5 withdrawn, branch `codex/local-face-recognition`.
Baseline: `6c095eb505f371c121211117f83280a46fb61c8b` (verified against origin).
Final source commit and binary SHA256 are recorded in the USB package metadata.

## 2026-10-28 feedback firmware 0.11.8 installed and boot verified

Firmware source 8528881b7d9e64247013e3419991d5e773da30e1; all five CI jobs pass:
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/40718883733.
The preceding unsaved session was already STOP; durable generation 17 remained.
After a fresh private full 16 MiB backup, installed only the app at 0x20000:
4,505,632 B, headroom 2,834,400 B, SHA256
`d85dd9e99f4341382093a8798a234c2d283a38bb87d04b98c35c23e385c917a3`.
verify_flash and full 16 MiB readback pass. Every other sector, including both
profile banks, NVS, table and models, is unchanged. Normal boot restores B/17,
saved/clean, ready AUTO, models 1/0 and continuing camera frames. No recovery,
profile edit, C6/eFuse action or new enrollment was performed during installation.

Local 32/32 CTest, 24/24 Python, 3/3 checks, formatting/whitespace, target build,
2736 B UI frame subtotal and seventeen linked IRAM functions pass. Package:
`dist/0.11.8-dev-enrollment-feedback`, with source metadata and validation receipt.
The guided reason display and a successful new-light verification/save still
require the next operator attempt; boot/CI do not constitute that acceptance.

## 2026-10-28 eight-sample save remains gated; feedback correction candidate

Owner reports eight samples and visible landmarks but cannot confirm a subsequent
re-enrollment under changed lighting. A running-target metadata read found ENROLL,
samples=8, verified=false, with the saved generation still 17. Concurrent metadata
reads can overlap publication and are not atomic snapshots; transient zeroed reads
are not treated as device state. No halt/queue injection/profile write was requested.
The current passive log reports ENROLL/ready and does not record verification
success/failure, so it cannot establish the exact earlier rejection cause.

Source review confirms a UI feedback defect: the guided page overwrites the actual
worker reason with generic waiting text. Candidate 0.11.8 shows the reason and
separates fresh-frame verification mismatch from another-profile conflict, with
bounded numeric ENROLL_VERIFY/ENROLL_STATUS logging and no names or embeddings.
All quality thresholds, three matching samples, freshness/conflict checks and
explicit verified-only Save remain unchanged. Local validation passes: 32/32 CTest,
24/24 Python, 3/3 project checks, formatting/whitespace and target build, including
seventeen linked IRAM functions. No target installation or physical acceptance
is claimed for this candidate. Visible landmarks/eight samples
alone do not prove verification. Candidate validation and installation must remain
separate; do not replace the active unsaved RAM session during this diagnosis.

## 2026-10-28 existing profile re-enrollment and restart verified

Owner completed guided fresh re-enrollment, eight samples, verification and save
using the existing profile. Actual commit took 6,687,649 us, generation 17. Private
full profile-partition readback validates header/payload SHA256 in both 17/16 banks.
Comparison finds exactly one changed face record, eight new face samples, preserved
ID/name/enabled state and byte-identical voice database. No profile deletion or
new identity. A controlled debugger-free restart restores bank B/17, saved/clean
and automatically ready AUTO, models 1/0. No firmware change. This confirms new
sample persistence, not yet an actual post-update identity/greeting/HA encounter.

## 2026-10-28 operator touch acceptance and passive background observation

Owner confirmed installed 0.11.7, successful touchscreen disable/enable and no
visible flicker. Passive log confirms two clean commits (15/16), 5,955,644 and
6,026,045 us, model loads/releases 1/0. A controlled debugger-free USB restart
restored generation 16, saved/clean and ready AUTO without manual activation.
UBox initially retried its network connection and later accepted an HTTP 304;
subsequent full JPEG retry also recovered. No new firmware/profile edits or JTAG.

Passive background window: eleven HEALTH samples all report AUTO mode 4, ready 1,
models 1/0, with continuing camera frames at 30 FPS and UBox activity. Nineteen
logged rejected inference attempts include nine face detections: four outside ROI
and five without five valid landmarks. The landmark stage ran in some attempts.
Logged failed attempts have zero MBF time; successful inference is not logged by
this diagnostic, so these observations do not prove full embedding/matching or
successful recognition. Owner reports poor enrollment photos. Current observed
ROI/landmark failures occur before template comparison and must be distinguished
from enrollment quality. Greeting/HA identity acceptance remains open. Voice gates
and all image-quality/model thresholds remain unchanged.

## Final installed 0.11.7 receipt

Firmware source: `098ede32577a914bff544fe968cdd9bd418e2cb2`. Application 4,505,248 B,
partition headroom 2,834,784 B, SHA256
`2b66003749fd57daabbcd94fa32ebd3049429bfc27f0ff15e505a3a95bb559dc`.
Fresh private full backup, app-only write at 0x20000, verify_flash and full 16 MiB
readback pass. All other sectors, profiles, NVS, table and both models unchanged.
No recovery erase, re-enrollment, partition, C6 or irreversible provisioning change.

One final-source real worker disable/enable cycle committed generations 13/14,
with stable boot ID, names, IDs, face/voice sample counts and models 1 load/0 releases.
Ready is false while disabled and true after enabling. It uses checked queue
injection while both harts are briefly halted; OpenOCD watchdog and touchscreen
limitations described below still apply. After debugger shutdown, full profiles
readback validates 14/13; generation 14's entire 312,416-byte canonical payload is
byte-identical to pre-test generation 4. A debugger-free normal restart restores
14, saved/clean, and automatically reaches ready AUTO. CSI continues about 30 FPS.
This supersedes the earlier final-source installation/restore placeholders below.

Actual first boot: restore 162,768 us; storage-stage internal lifetime low 285,239 B,
PSRAM low 30,460,512 B. Uptime 3 s: internal free 202,967 B, largest 69,632 B,
lifetime minimum 171,263 B, PSRAM free 18,167,508 B; identity stack watermark 5,920 B.
These are sampled free space/lifetime lows, not isolated operation peaks. Checked
UI frame subtotal remains 2,736 B; persistence snapshots remain 624,832 B PSRAM.

Final firmware CI: all five jobs pass, including host ASan/UBSan and four firmware
configurations: https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/40411959469.
Local 32/32 CTest, 24/24 Python, 3/3 checks, formatting/whitespace, recognition build,
Flash budget and seventeen-function ELF IRAM placement pass. Public final package:
`dist/0.11.7-dev-iteration13-final`, with source metadata, VALIDATION.md, SHA256SUMS
and a separately hashed ZIP. Backups/raw logs remain private and outside Git.

No outstanding installation authorization: the owner authorized these operations.
Visual touchscreen/flicker acceptance, actual restored-member greeting/HA delivery,
and real microphone/speech/model/calibration acceptance remain physical checks.
Speaker weights are provisioned and numeric readiness/failure cleanup reporting is
implemented; this is not a measured successful speaker inference. Voice/BOTH events
remain gated. Next milestone: one combined debugger-free visual/greeting/HA and
bounded silence/speech readiness session before considering 0.12.0-dev. PR stays
draft/unmerged; this is a reproducible development candidate, not product release.

## Verified 0.11.6 hardware transactions and final 0.11.7 UI guard

0.11.6/b7f5da1 was installed app-only with verify and full Flash readback; all other
sectors matched the fresh private backup. All five CI jobs passed (40386086581).
A captured disable/enable pair committed generations 5/6, parked/resumed the same
model set and survived a normal restart restoring generation 6 with ready AUTO.

Three further automated hardware worker cycles produced six verified commits:
generations 7,8,9,10,11,12. Both harts were briefly halted only to inject each
command into the checked empty/unlocked existing queue and to inspect results;
execution used the real identity worker, production transaction engine and active
CSI/DSI. Boot ID stayed constant; names/IDs/sample counts stayed identical and
model loads/releases remained 1/0. This proves the backend risky path without a
new manual reproduction, not touchscreen/visible flicker or normal-watchdog proof
while the debugger was attached.

After debugger shutdown, a private read of the entire profiles partition verifies
valid generations 12 and 11. The latest 312,416-byte canonical payload is byte-for-
byte identical to generation 4 before the tests, proving names, IDs and both
modalities' templates preserved, beyond just matching counts. A debugger-free
normal restart then restores generation 12 and ready AUTO with continuing CSI
frames. No recovery erase or re-enrollment.

Measured transaction times include 5,030,632 and 6,041,538 us. The final UI guard
in 0.11.7-dev does not report a create timeout while storage_saving is true, even
past 5 seconds; actual creation receipts are still handled first. Waiting without
an active save retains the old timeout. Tests cover 5,001/5,031/6,042/30,000 ms
and prevent false failure during a legitimate durable commit. The worker/storage/
IRQ implementation remains that of the verified 0.11.6 fix. Final-source install
and target receipts are recorded when obtained, without inferring acceptance.

## 0.11.5 failure and measured 0.11.6-dev correction

The owner confirmed the dark frame metrics were caused by darkness/device
orientation and that the camera itself operates. However, disabling a profile
reproduced a restart. A live capture confirms HP_SYS_HP_WDT_RESET, with previous
committed generation 4 restored. This supersedes any acceptance implication from
the earlier idle boot/window. Do not run additional manual tests on 0.11.5.

USB JTAG/OpenOCD and the exact installed ELF captured the original exception.
With both harts halted and the existing polling command queue empty/unlocked,
a synthetic disable command plus the normal epoch change was injected into that
queue. It exercises the real worker/Flash/CSI transaction, not the touchscreen.
At panic entry, the saved frame has mepc=0x402dd6be, ra=0x4ff01c7a, mcause=0x19,
hart=0. Address resolution identifies `dw_gdma_channel_config_transfer` called by
`csi_dma_trans_done_callback` at the native CSI transfer restart. SDK 5.5.1 leaves
this function in Flash despite its other IRAM control options. The secondary
watchdog occurs while the cache error is being handled. No model threshold,
partition or stored templates are implicated by this frame.

0.11.6-dev adds a project-local linker fragment for only that missing noflash
mapping, without editing the installed SDK. The ELF guard now checks seventeen
entrypoints/callees, including DW-GDMA control/list helpers, cache synchronization
and the timer. The 0.11.5 binary is rejected at the exact Flash-resident culprit.
Hardware transaction tests under the debugger are diagnostic evidence; OpenOCD
changes watchdog behavior while halted, so final normal-mode/touchscreen/flicker
acceptance still requires a debugger-free session. Source/CI/package/install and
post-fix results are recorded below once actually obtained.

## Historical 0.11.5-dev installation and boot (subsequently withdrawn)

Following the existing owner authorization, installed only the app at 0x20000:
4,505,248 B, SHA256 ccd13d196db6410d4617f0c1d16e951fd36358149cb1aab9c79e0051d984f8e8.
A fresh private 16 MiB backup preceded the update. App verification and full Flash
readback pass; every other sector is unchanged, including all profiles, NVS,
partition table and both models. No recovery or new enrollment was performed.
Public package: dist/0.11.5-dev-iteration13-csi-cache, 54 verified hashes and ZIP.
All five CI jobs pass for firmware source 311dbe1:
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/40374472427.
Local: 32/32 CTest, 24/24 Python, 3/3 checks, sanitizers, target configuration,
eight-function linked ISR placement, Flash and unchanged 2,736 B UI frame subtotal.

Actual 0.11.5 boot confirms version, LVGL, profile bank A generation 4, durable clean
status and automatic ready AUTO (models 1/0). Restore took 164,230 us. CSI controller
and application callbacks register successfully and real frame completion counters
advance at 29.6-29.7 FPS. The first 55-second window has no panic, watchdog, callback
registration error or display underrun. Heap samples at uptime 3 s: internal free
203,423 B, internal lifetime minimum 171,895 B, PSRAM free 18,167,508 B. These are
sampled free space/lifetime minima, not isolated operation peaks.

Frames currently measure below-range luminance 0.0/rejected fraction 1.00; this is
not proof of usable scene pixels. Operator lighting/lens context or a preview is
needed before camera-image/recognition acceptance. Actual disable/enable after
this fix, absence of visible flicker/MBF error, post-change restart, greeting/HA
and microphone/model inference remain pending operator interaction. Do not infer
these from frame rate or ready status. 0.11.4-dev was withdrawn after its CSI boot
failure; its earlier all-green CI is not hardware acceptance.

## Earlier authorized 0.11.3-dev installation

The owner subsequently authorized installation and hardware testing. The exact
0.11.3-dev candidate (8499ffc) and pinned speaker model were installed on COM5.
A private 16 MiB backup was captured outside the repository before any write.
Disabled Secure Boot/Flash Encryption and the unchanged installed table were
verified. The backup contained valid schema-2 banks A generation 2 and B generation 1;
no recovery erase was necessary or performed.

Writes were limited to application 0x20000 (4,503,568 B) and speaker model 0xc20000
(2,844,624 B). esptool verification and full 16 MiB readback passed. Every sector
outside the two selected write/erase ranges was byte-identical to the private
backup, including the entire profiles partition, NVS, face model and partition table.
Application SHA256: 86eabffc2384fbc571b9eaf9eb28c1713bc280ef54c91eac3b858e6e442d05c7.
Speaker SHA256: b503ff049a0cd5c62b4c1ac337cf20b7de1549857a4d5329a1821cfb55e9b3a8.

Actual boot log confirms Home Buddy 0.11.3-dev / IDF v5.5.1, LVGL ready,
PROFILE_BOOT result=0 bank=0 generation=2, PROFILE_STATUS ready=1 saved=1 dirty=0,
and ready AUTO (HEALTH mode=4 ready=1) without manual activation. Restoration took
170,369 us. The first 55-second boot capture has no panic/watchdog/brownout marker.
This proves this boot's restoration/AUTO readiness, not recognition/greeting/HA or
new-transaction acceptance. Touch changes, a post-change restart and microphone/
real model inference remain pending operator interaction. Prior owner-confirmed
retention/AUTO on 0.11.2-dev remains the baseline, superseded only where tested here.

## Profile toggle hardware defect and 0.11.4-dev correction

The operator's first disable/enable test on 0.11.3-dev produced display flicker
and a failed MBF restart. Passive capture confirms both profile commits succeeded:
PROFILE_STATUS result=0 ready=1 saved=1 dirty=0 bank=0 generation=4. AUTO remained
selected but not ready (models=1/1), so persistence acceptance must be separated
from failed recognition availability. No post-change restart or voice test was
claimed; the failing runtime was captured before any further device reset.

The observed internal free heap was 249,939 B but its largest block only 59,392 B,
below the unchanged 65,536-byte cold-start admission. Disabling the last enabled
member had unnecessarily destroyed the already loaded model set. The new policy
parks that same bounded set only for a member-disable pause with AUTO still chosen.
Disabled people cannot produce inference, confirmation, greeting or HA identity.
Re-enabling reuses the existing set; no additional model allocations or relaxed
admission thresholds. Explicit Stop/cancel, deletion and foreground mode changes
clear the administrative pause and retain normal resource release. A genuine cold
start can still fail safely under fragmented memory; this is not a general heap
compactor or a promise that Stop/reload succeeds under arbitrary memory pressure.

The pinned IDF 5.5.1 DSI callback must restart frame DMA on each completion. With
LCD_DSI_ISR_IRAM_SAFE disabled, Flash operations defer that handler. Enable the
supported setting (which selects DW_GDMA_ISR_IRAM_SAFE) and verify effective
configuration in every build profile. The board refresh callback was already IRAM
and uses an internal semaphore. This targets the Flash-associated flicker; physical
confirmation on the same toggle remains required, not inferred from configuration.
Menu/system firmware labels now use the real application descriptor instead of
stale hardcoded versions. A 100-cycle policy regression covers park/resume without
reallocation, explicit release, disabled cold state and foreground enrollment.

## Shared DW-GDMA follow-up, 0.11.5-dev

0.11.4-dev (e9a2f6c) passed all five CI jobs and was installed application-only with
verified readback; all other sectors, including profiles generation 4, were unchanged.
The automatic boot check then caught a CSI regression before an operator retest:
DW-GDMA rejected the camera controller's external-RAM user context. DSI's global
IRAM-safe DMA selection applies to CSI as well. Model-ready AUTO without camera
frames is NOT recognition acceptance. No successful toggle/flicker result is claimed
for 0.11.4-dev; its runtime is superseded by the following correction.

Enable CAM_CTLR_MIPI_CSI_ISR_CACHE_SAFE with DSI IRAM safety, selecting the pinned
DW-GDMA ISR/control/getter/setter IRAM modes and the CSI controller's internal-RAM
allocation. Move the two application callbacks and their bounded frame-pool
reserve/complete/abort/publish callees into IRAM. ISR state stays in static internal
RAM; callbacks never read biometric/PSRAM payloads, allocate, log or call LVGL.
Time division uses the existing ROM libgcc helpers. A new post-link ELF guard
checks the actual eight camera/display entrypoints and frame-pool callees; missing
or Flash-resident functions fail the build. Effective configuration checks cover
the shared DMA contract. Host frame-pool lifetime/lease behavior is unchanged.
The target boot must confirm actual CSI frames as well as profile/AUTO restoration.

## Storage initialization and first save

The production boot engine verifies both entire banks. Only a completely erased
1 MiB partition receives an empty schema-2 generation in upper bank B. Enrollment
then saves to A, retaining that committed empty fallback. No format, partition,
ABI, namespace, household capacity or persistence-policy change is introduced.
A valid pre-existing database restores without this initialization write.
Legacy migration retains both schema-1 banks until its schema-2 commit verifies.

Bootstrap is itself a transaction. If power fails during this empty generation,
there may still be no committed bank. Unknown/torn bytes remain preserved and
writes are blocked; this tradeoff cannot be eliminated by pretending an erased
header proves a blank bank. A failed first user save restores either the empty
fallback or the new valid generation, depending on whether its header committed.
Failed runtime saves retain usable RAM and a retry; durable UI success still waits
for header/payload readback. A commit whose final read fails can be found valid
on reboot; the failed attempt is never presented as confirmed durable in advance.

Flash erase and payload programming use bounded 4 KiB operations, with a scheduler
yield between device operations. Readback and final commit ordering stay unchanged.
The identity worker owns the full transaction and shared compute admission covers
boot, fingerprinting and saves. LVGL callbacks do not perform database writes.
Yields reduce continuous monopolization; they do not prove watchdog or physical
power-failure acceptance. No cancellation can delete the owning worker mid-save.
Canonical fingerprints still exclude recognition timestamps/counters.

## Transaction evidence

Tests exercise production `profile_store_v2.c`, `profile_store.c` and the real Flash
adapter with synthetic records, never household biometrics. Empty bootstrap injects
four cut lengths at all 206 mutation boundaries plus blank scan, payload readback
and final verification failures (836 cases). First user save injects four cuts at
all 209 I/O operations (836 cases) and proves the empty fallback remains unchanged.
Existing database faults cover all 209 I/O operations with eleven cut lengths
(2,299 cases), asserting the active bank is byte-identical and restored names and
IDs match the selected generation. Migration, full capacity, both modalities,
rename/disable, corrupt/unknown banks, locked policy, deletion and scrub remain
covered. The historical 14,368-byte headerless payload failure remains reproduced.
Tests use a deterministic protocol checksum; target firmware uses mbedTLS SHA256.

## Recovery and USB

Recovery defaults to offline inspection and checks backup SHA256, package/table,
exact offsets, entire current partition, security state, approved scope and blank
readback. It rejects ambiguous/duplicate security fields and protected devices.
All USB commands now leave the board in the loader (`--after no_reset`), preventing
application writes between the comparison and erase/readback. Execution leaves the
loader active; a later application start/update is a separate authorized step.
Tests cover checksum/policy/layout/security failure, changed bytes, failed reads,
erase/readback failure and exact 0xba0000/0x80000 erase scope. The private backup
is never overwritten. This old interrupted-first-save recovery is inapplicable
to the currently valid household database.

Application-only USB plans still write only 0x20000, preserving NVS, profiles,
face/speaker models, settings and C6 firmware. Full recovery remains a separate
potentially destructive operation described in USB_FLASHING.md; never default to it.

## UI, restoration and voice

Four product sections, eleven advanced diagnostic entries, static UI status
workspace, touch geometry and target stack guard remain. Save failure no longer
shows a raw result number on the ordinary household screen. Numeric result stays
in advanced diagnostics/logs. Storage damage reports that profile memory needs
repair. Restore precedes AUTO preference processing, independently of Wi-Fi/HA;
model initialization retains its three bounded attempts and preserves templates.
No new GUI layout or extra ordinary navigation confirmation is added.

The existing explicit voice diagnostic now exposes one numeric readiness report:
selected I2S slot, observed rate, accepted sample count, RMS/peak/clipping/suspicious
signal flag, valid normalized 512D output, model load/preprocessing/inference times,
sampled heap lows, and callback/PCM cleanup. It logs no PCM or embedding values.
The old inferred model load/release counters were removed: RAII adapter cleanup
has host fault tests, not a fabricated physical resource-release measurement.
Report validity is tested with success, invalid output and error/cancel paths.
A successful embedding is not proof of speech selection or speaker accuracy.
Voice enrollment/identity calibration gates and FACE-only HA policy stay intact.

## Memory and validation receipt

Exact unchanged snapshot allocations: 2 x 312,416 = 624,832 bytes PSRAM. No extra
persistence buffer or snapshot-sized LVGL object. Voice adds a small fixed numeric
report; its 96,480-byte PCM window remains temporary and scrubbed before release.
Physical boot telemetry: storage-stage lifetime internal low watermark 287,143 B
and PSRAM low watermark 30,460,096 B; at uptime 3 s, internal free 204,667 B,
internal minimum 173,719 B, PSRAM free 18,166,888 B. Identity stack watermark 5,936 B.
These are sampled free space/global lifetime lows, not isolated operation peaks.
Initial restore took 170,369 us. Save/inference latency and longer watchdog behavior
remain pending; no measured microphone/model resource result is claimed yet.

Local validation: 32/32 CTest (including LVGL navigation and visual baselines),
21/21 Python regressions, 3/3 project checks, complete ASan/UBSan script, formatting,
whitespace, effective recognition configuration and enforced Flash budget pass.
Target ESP-IDF 5.5.1 recognition build passes; checked UI frame subtotal is 2,736 B.
Application size: 4,503,568 bytes; 2,836,464 bytes headroom in the 7 MiB partition.
The earlier Windows build invocation hit console encoding U+2265; rerunning with
PYTHONUTF8=1 succeeded. It was a host build-environment error, not a target fault.
Full firmware matrix and final source-bound package receipt are recorded in the
package VALIDATION.md after GitHub Actions completes. No CI success is inferred
from these local checks. Host tests cannot prove complete ESP-IDF boot under
network/model outages or every device-specific LVGL page and touch transition.

## Next milestone and one acceptance session

Application and speaker installation are authorized, completed and verified.
Next: one durable intentional change/restart/restore plus greeting and eligible HA event. Do not delete/re-enroll the current member or run obsolete recovery.
The pinned speaker model is now provisioned under the same approval. Collect
silence/speech/slot response and one readiness report in the same session. 0.12.0-dev remains conditional on target persistence
validation and measured microphone/model readiness, with calibration still required
before public speaker identification. No production release or PR merge.
