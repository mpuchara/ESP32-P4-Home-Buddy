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
