# Iteration 5 validation receipts (development)

Status: experimental implementation; hardware acceptance incomplete.
No production release, merge, flash, erase, partition migration, C6 write or HA bridge change.

## Completed checks

- Baseline Wi-Fi rebuild: 4,111,328 bytes, ESP-IDF 5.5.1.
- Initial experimental recognition firmware compiled with MBF backend, MNP and
  native ROI. Final code matrix is being collected before final delivery.
- Full host suite: 13/13 passed (original 12 retained plus identity core).
- Identity core passed GCC AddressSanitizer and UndefinedBehaviorSanitizer.
- Python project checks: 3/3; formatting and whitespace checks passed.
- Pinned component resolver kept every existing dependency version unchanged;
  added human_face_recognition 0.3.3 only. MBF file size/digest checked.
- Generic-only SD model staging tool executed successfully.

## Not accepted on physical hardware

This change has not been flashed or executed on the physical P4. COM5 enumeration
only is not a hardware test. No new recognition latency, memory peak, landmark
quality, neural accuracy, real enrollment, personalized greeting, JPEG/Wi-Fi/SD
coexistence, thermal/power, watchdog, fragmentation or soak result is claimed.

The existing baseline's operator results remain in DEVELOPMENT_LOG.md and do not
validate this new subsystem. Upstream reference timing/accuracy is not board data.

## Explicit remaining criteria

1. Provision generic MBF on local FAT microSD; verify slot 0 with C6 slot 1 and
   demonstrate missing/corrupt/removed-card behavior without Wi-Fi disruption.
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
