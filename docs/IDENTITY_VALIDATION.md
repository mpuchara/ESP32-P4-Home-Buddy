## Active hardware investigation, 2026-10-24

Captured failure: `local/identity-household-crash-11.log` contains an interrupt
watchdog on CPU1 immediately after MODEL_LOAD begin and before detector creation.
Matching diagnostic ELF SHA prefix is 2e574acd4. CPU1 MEPC 0x4ff00c5e resolves to
shared_intr_isr (intr_alloc.c:481); CPU0 MEPC 0x4ff02530 resolves to the flash IPC
wait in spi_flash_op_block_func (cache_utils.c:109). reset_reason=5 follows.
This identifies a flash/cache-handshake stall during model validation, not a
biometric match failure. The exact interrupt-lock interleaving is not proven.

The candidate replaces 3,442 raw esp_partition_read calls per validation with
one read-only cache mapping. Header and complete pinned-package SHA-256 remain
mandatory on every load; hashing yields per 64 KiB and unmaps on every exit.
The host regression covers 20 reloads, full-byte coverage, corrupt header/tail,
mapping admission/failure and all SHA error cleanup paths using an injected SHA
double. It does not claim hardware concurrency coverage. Host C suite 17/17,
Python 7/7 and project checks 3/3 pass. Physical re-entry acceptance remains open.

USB deployment of the mmap candidate succeeded in application-only mode on P4
COM5 and verify_flash passed. Application: 4,386,480 bytes, SHA-256
72fa4c48ab8410f4d48fd696910f30ff08d7e643df58e62102f8906402cbf9b3.
Recognition build and effective-config/budget checks pass. NVS, partition table,
model and profile areas were not written. A bounded 18-minute passive capture is
running for operator re-entry/landmark checks; no acceptance is claimed yet.

The operator reports MBF ready, one face, UNKNOWN, visible landmarks and the
successful-inference message "Kandydat z nowej klatki". This is operator evidence
of an initial pipeline execution, not a measured accuracy/enrollment acceptance.
The operator also reports a device restart when returning from the saver to the
household recognition test after landmarks appeared. This remains an OPEN blocker.
Do not proceed to enrollment/recognized-event acceptance until it is resolved.

The operator clarified that opening the household page alone works; the restart
occurs when entering recognition. A later capture shows a successful first screen
open/model load, a safe release (heap integrity passes before and after), and a
truncated second recognition-screen-open line followed by a new uptime. The panic
and reset reason were missed during the capture gap, so the cause remains unknown.
UI minimum free stack recorded before the second open was 1,520 bytes; identity
worker minimum free stack at release was 6,448 bytes. These are diagnostic
observations, not proof of stack exhaustion. CI run 40047726427 passes the host job
and all four firmware profiles for diagnostic commit 909a1f5; hardware restart
acceptance remains open.

Initial bounded passive captures show model loads (PSRAM delta 8,538,056 bytes,
internal deltas around 55 KiB), continued ~30 fps camera capture, and no captured
panic for the specific after-inference re-entry. Some full-resolution UBox JPEG
decodes fail with ESP_ERR_NO_MEM while models are resident; a later smaller JPEG
is accepted. This is a separate observed coexistence issue, not proof of the
restart cause. Existing admission checks and rejection behavior remain enabled.

The diagnostic candidate adds reset reason, screen-transition/minimum-stack logs,
model construction/release stages, heap integrity at release, and rate-limited
inference-wait reasons. No images, landmarks, names, credentials or embeddings are
logged. Recognition panic reboot waits 3 seconds to allow the dump to drain.
Missing score sentinels display as "brak"; landmark dots have no white border.
The USB capture helper writes incrementally and optionally reconnects passively
after USB reset. Two injected-disconnect regressions pass; Python suite 7/7 and
host C suite 16/16 pass. These changes aid diagnosis; they are not a restart fix.

---

> Historical iteration 5 receipts. Superseded storage deployment and outstanding
> criteria are documented in ITERATION7_REPORT.md. No microSD deployment is supported.
> Current single-factory firmware is installed and its REST diagnostic automation
> is operator-confirmed. MBF/enrollment/recognized-event hardware acceptance is
> the active next step; the old unflashed status below is historical only.

# Iteration 5 validation receipts (development)

Status: experimental implementation; hardware acceptance incomplete.
No production release, merge, flash, erase, partition migration, C6 write or HA bridge change.

## Completed checks

Validated code commit: `a59523557e79533b43eac7a42ed788ff287abc5e`.
[CI run 56](https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39967264653)
completed **successfully**: host and all four firmware jobs. Subsequent receipt-only
documentation commits do not change that validated code.

| Local firmware profile | Application bytes | Build + effective config |
|---|---:|---|
| recognition | 4,383,856 | PASS |
| diagnostics | 227,536 | PASS |
| bringup | 3,820,432 | PASS |
| wifi | 4,121,472 | PASS |

- Baseline Wi-Fi rebuild: 4,111,328 bytes, ESP-IDF 5.5.1.
- Full host suite: 13/13 passed (original 12 retained plus identity core), also in CI.
- Identity core passed GCC AddressSanitizer and UndefinedBehaviorSanitizer,
  including staging/append/cancel, model namespace rejection, matching and geometry.
- Python project checks: 3/3; formatting and whitespace checks passed.
- Pinned resolver preserved every existing dependency version, adding recognition
  0.3.3 only. Generic MBF file size/digest and generic-only SD staging tool passed.
- Firmware packaging passed with attribution notices; no personal biometric files.
- Partition table and existing camera/face/screensaver/services/connectivity code
  have no changes. New integration uses the original camera and compute APIs.
- Cancellation has an out-of-band stop latch so a full command queue cannot reject
  Stop; a generation change clears candidate/confirmation before any republication.
  Concurrent hardware stress acceptance is still pending.

## Not accepted on physical hardware

This change has not been flashed or executed on the physical P4. COM5 enumeration
only is not a hardware test. No new recognition latency, memory peak, landmark
quality, neural accuracy, real enrollment, personalized greeting, JPEG/Wi-Fi/SD
coexistence, thermal/power, watchdog, fragmentation or soak result is claimed.

The existing baseline's operator results remain in DEVELOPMENT_LOG.md and do not
validate this new subsystem. Upstream reference timing/accuracy is not board data.

## Explicit remaining criteria

1. Use the internal-Flash installation and missing/corrupt-model acceptance in FLASH_MIGRATION.md.
2. Execute MBF on native camera ROIs, inspect landmark overlays and alignment;
   compare ESPDet candidates with official MSR candidates under representative scenes.
3. Measure per-stage inference/preprocessing, model deltas, peak/internal/PSRAM and
   contiguous blocks, including JPEG traffic. Combined MBF timing includes alignment.
4. Consenting local enrollment, separate verification and unknown/similar-person
   trials; record counts, ambiguity/confusion, latency and threshold distributions.
5. Repeated model unload/reload, cancellation/navigation and background sessions;
   confirm no leak, camera ownership violation, stale greeting, watchdog or UI freeze.
6. Persistent opt-in and persistent biometric storage remain deferred. There is no
   claim of encrypted profiles, schema migration, reboot recovery or failed-write tests.
7. CI results must be taken from the actual workflow runs for the code commit;
   updating its matrix is not itself evidence that CI succeeded.

See FACE_RECOGNITION.md, IDENTITY_FLASH_LAYOUT.md and ACCURACY_CALIBRATION.md.
