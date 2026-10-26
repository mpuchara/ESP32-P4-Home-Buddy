# Iteration 13: persistent identity stabilization

Date: 2026-10-26. Current correction candidate: **0.11.6-dev**; installed 0.11.5-dev is withdrawn after a captured Flash-time CSI fault, branch `codex/local-face-recognition`.
Baseline: `6c095eb505f371c121211117f83280a46fb61c8b` (verified against origin).
Final source commit and binary SHA256 are recorded in the USB package metadata.

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

## Latest 0.11.5-dev installation and boot

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
