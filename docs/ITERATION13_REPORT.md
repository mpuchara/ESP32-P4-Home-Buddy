# Iteration 13: persistent identity stabilization

Date: 2026-10-26. Candidate: **0.11.3-dev**, branch `codex/local-face-recognition`.
Baseline: `6c095eb505f371c121211117f83280a46fb61c8b` (verified against origin).
Final source commit and binary SHA256 are recorded in the USB package metadata.

## Current physical state and authorization

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
