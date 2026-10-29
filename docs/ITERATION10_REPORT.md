# Iteration 10 candidate report, 2026-10-25

Version 0.9.0-dev, continued from PR #2 head 53dbd23 on codex/local-face-recognition.
Source provenance and measured image sizes/hashes are recorded in the USB package
usb-package.json and SHA256SUMS. Current CI status is linked from PR #2.

| Required outcome | Actual status |
|---|---|
| Microphone changes | Single-owner L/R consumer, both-slot DC/clip/repetition, delivery stamps, loss invalidation, resource/stack/processing telemetry, early Start/Stop race fixed. |
| Microphone hardware | Not installed or physically accepted; no real silence/speech evidence added. |
| VAD | Bounded local preliminary activity gate implemented/tested. No validated speech/overlap detector; ESP-SR AFE/VADNet investigated without importing its full stack. |
| Speaker model | Compiled checked adapter for pinned 0.3.1/ESP-DL 3.3.14, exact tensors/filterbanks/half-even quantization, dequantization+normalization, cancellable stages. One-shot advanced diagnostic only. |
| Package | 2,844,624 B, 301,104 B headroom in unchanged 3 MiB sv_model_3s. Whole/raw hashes and optional isolated USB mode. |
| Inference timing | Not measured on hardware. Instrumentation implemented; no invented latency. |
| Peak RAM/PSRAM | Not measured. 96,480 B PCM +96,000 B feature scratch plus FFT/window/mel/model; admission and sampled telemetry implemented. File sizes are not a runtime peak. |
| Enrollment | Preparatory versioned RAM core tested with synthetic vectors, multiple templates and stable IDs. Real user journey deliberately unavailable pending microphone/model/threshold acceptance. |
| Fusion | Freshness/session/conflict policy host-tested; no runtime identity/HA connection. |
| Face regression | Existing host identity/AUTO/navigation/visual/pipeline checks retained; current hardware regression pending. Historical physical evidence remains version-scoped. |
| HA regression | Existing FACE-only payload/configuration/dedup/outage policy tests retained. No new physical network acceptance and no voice events. |
| Persistence | Existing transaction store/security gate unchanged. Separate voice schema/model namespace prepared; migration sizing below. No plaintext or eFuse activation. |
| Automated tests | Final receipts below. Fault-injected audio owner and speaker adapter exercise production control flow with mocked drivers/FFT/model; not model accuracy or real preprocessing equivalence. |
| GitHub Actions | Source 9be6e0e CI 40194481185 host passed; later adapter alias-safety correction has its own final-head CI linked from PR #2. Compilation is separate from hardware acceptance. |
| Hardware acceptance | One concise combined session below; no automatic flash/reset. |
| Next milestone | Accept actual slot/silence/speech on installed candidate, then pinned-model numerical/resource benchmark, speech-selective VAD and voice-specific enrollment/unknown calibration. |

## One essential operator session after an intentional USB installation

Restart loses installed RAM-only profiles. Preserve the currently running unit until
that consequence is accepted. No flash was performed by this iteration.

1. Open the four-category menu and navigate back to the UBox screensaver.
2. Informacje > Diagnostyka zaawansowana > Mikrofon: Start, brief silence,
   normal/louder speech and a pause, Stop. Retain the numeric log; both slots must
   show which actually carries useful PCM without excessive DC/clipping/repeats,
   errors/loss, while touch/UBox/brightness remain responsive. This is the sole
   microphone acceptance check, not a multi-person test matrix.
3. Confirm one enrolled face still greets and emits its eligible FACE HA event.
   Only if necessary after the agreed reboot, re-enroll that single household profile.

The new model L/R diagnostic is optional until capture is accepted and the optional
speaker partition intentionally provisioned. A valid embedding there is still not
speaker accuracy acceptance. No long manual observation or 72-hour result is claimed.

## Persistent-profile migration readiness

Current schema-1 face payload is 246,640 B, with a 128 B header in each 256 KiB
bank; two banks occupy only 512 KiB of the existing 1 MiB profiles partition.
Raw voice vectors for 8x4x512 float32 add 65,536 B plus metadata. Combining those
with face data exceeds the present 256 KiB bank; never append/reshape schema 1.
A future schema-2 could use two 512 KiB banks within the unchanged partition, but
must validate authenticated lengths/namespaces/stable-ID references, stage the
complete migration, retain a valid old bank until a verified new header is committed,
and prove interruption/rollback/scrub using synthetic data first. This is a sizing
and migration design, not an implemented migrator or permission to write profiles.

Existing security/recovery assessment in SECURITY_PROVISIONING.md still governs:
RAM-only default, no same-Flash embedded-key workaround, no plaintext fallback,
no irreversible provisioning. Secure Boot/encryption activation needs a proved
signed USB recovery route on matching spare silicon and a concrete key-custody
procedure. The current USB helper deliberately rejects secured devices/packages.

## Validation receipts

Local validation: 26/26 C/LVGL tests, 12/12 Python tests, 3/3 project checks,
formatting and whitespace checks passed. ASan+UBSan passed identity/enrollment,
PCM, voice core/fusion, fault-injected speaker adapter and audio-owner lifecycle.
The deterministic menu/settings/diagnostic render hashes remain unchanged; this
is shared-menu geometry coverage, not a full peripheral/voice-page simulator.

Recognition firmware compiled with ESP-IDF 5.5.1 and unchanged dependencies.lock;
effective recognition config and enforced Flash budget passed. Final application size, factory margin, exact source commit and all file hashes
are recorded by package_firmware.py in usb-package.json and SHA256SUMS. The
optional speaker image is independently validated and never part of app/full writes.
An additional greedy-arena alias test ensures output is not cleared before run:
output and input buffers can share storage, so pre-clearing output corrupts input.
The adapter correction is included in the final candidate.
No physical inference, peak RAM, silence/speech or uninterrupted soak was measured.
