# Iteration 9: product UI and local audio diagnostic foundation

Date: 2026-10-25. Development candidate 0.8.0-dev, branch
`codex/local-face-recognition`, draft PR #2 based on PR #1. Audited remote/local
baseline: 4de115991e26d4daca707d350a2cf332122f64b1. The repository had no local changes.
Installed firmware remains the previously documented 00272f7; no live interrogation,
reset, flashing or fresh hardware acceptance was performed during this iteration.
Prior physical face/greeting/HA receipts remain baseline evidence, not acceptance
of this candidate or the prior 0.7.0-dev AUTO/audio candidate.

## Implemented

- Four fixed 800x480 cards: Domownicy, Rozpoznawanie, Ustawienia, Informacje.
  Photo return remains visible. Development controls moved to advanced diagnostics.
- Settings group Wi-Fi scan, UBox, weather, brightness and HA. Existing config
  schemas, visible UBox editing token, Open-Meteo and USB layout are preserved.
  HA connection tests have their own advanced page and retain the HTTP/action distinction.
- Stable-ID household list, person details, rename and confirmed delete. Name entry
  leads directly to automatic enrollment. No fake/prepopulated profiles or voice button.
- Guided enrollment runs in the existing identity worker after explicit BEGIN:
  existing detection/landmark/quality checks, single face, duplicate/diversity checks,
  one-second spacing, eight samples, fresh-frame verification and different-person
  conflict rejection. One final confirmation commits templates. Updated list needs no
  manual refresh. Repeat enrollment stages replacements without changing old templates
  before commit. Existing engineering/manual enrollment backend is retained.
- Ordinary AUTO on/off, Polish status, household count and last recognized person.
  No MBF inference without usable profiles outside enrollment. Normal navigation does
  not own the AUTO session. Closing foreground identity work first cancels its epoch
  and then restores prior AUTO consent; explicit Stop prevents restoration.
- Idle timeout protects keyboard/enrollment/delete confirmation/save and active audio.
  Save feedback stays on the edited form. Brightness calibration is shown in AUTO,
  manual slider in manual/override; low-level sampling controls have a separate page.
- RX uses named BSP DIN48, with unrelated pins unchanged. Audio reports both slots,
  RMS/peak/clipping, DC, sample count, suspicious repetition, drops/errors and bounded
  buffer sizes, with a local level meter. A busy guard protects Start/Stop cleanup.
  ES8311 shared I2C, identity-before-write, ADC-only, no PA and 60-second limit remain.
- RAM-only warning remains even for experimental secure-storage readiness: physical
  protected persistence has not been accepted. No security provisioning is activated.

## Automated validation

Existing host suites remain. New real LVGL menu renderer exercises shared firmware
layout code and the committed Polish font, 400 transitions, click dispatch, label/
touch bounds and allocation-pool recovery. It emits synthetic main/settings/diagnostics PPM artifacts with checked visual hashes.
Host timeout tests include tick wraparound and protected operations. Enrollment
uses the same extracted worker policy with synthetic embeddings, invalid/multiple
faces, frame spacing, fresh verification, explicit commit and replacement isolation.
A synthetic pipeline joins actual matching/confirmation, independent event consumers,
HA serialization/privacy, HTTP outage classification, cooldown and epoch cancellation.
Sanitizer checks for identity/enrollment and PCM run in CI. PCM tests cover bounds, signed extremes, DC, zero/full-scale suspicion, both slots
and nonperiodic data. Python checks guard actual top-level wiring and audio direction.

This is not a full hardware-free simulation of every ui.c callback, all peripheral
workers or all combinations of runtime allocation/HTTP failure. Existing HA payload/
policy, storage fault-injection, camera lease/recovery and AUTO consent tests remain
separate real-logic tests. No automated result establishes peripheral signal,
physical recognition accuracy or long simultaneous operation.

Validation receipts and package hashes are recorded in ITERATION9_VALIDATION.md.
CI builds diagnostics, bringup, wifi and recognition from clean configurations.
No dependency or license versions changed; existing source/model license inventory
is preserved in the USB package. No production release or merge was performed.

## Audio and voice evidence

See AUDIO_DIAGNOSTICS.md for the corrected direction and pinned driver audit.
The manufacturer specification identifies microphone position 1; sheets 01/05
show analog MIC1 through the ES8311. BSP declares P4 RX48/TX9. These are source
findings; exact component population, left/right slot alignment and a meaningful
silence/speech signal on this unit remain unaccepted. No microphone recording,
audio upload, voice embedding or speaker model execution was performed.

SPEAKER_FEASIBILITY.md retains asset sizes/tensors and the exact 48,240-sample
300-frame preprocessing calculation. Microphone acceptance still gates inference.
No new voice/fused events; existing FACE-only metadata serialization remains.

## Compact hardware acceptance after separately authorized installation

One combined UI smoke: photo/clock/weather, four cards/back, one guided enrollment,
rename/cancel-delete, AUTO continuing through navigation, brightness and saved config.
One microphone test: opening is passive; Start, silence/speech on both slots,
plausible RMS/DC/clipping, no permanent zero/full scale, Stop/leaving cleanup,
and simultaneous touch/camera/UBox. One recognition/HA regression: known greeting
and metadata event, multi-face/unknown rejection, encounter cooldown, then OFF.
HTTP success alone does not establish HA automation execution.

Installation restarts the device and loses current RAM enrollment; this iteration
has not performed it. No repeated flashing for small UI changes is needed. Longer
soak, quantitative biometric calibration and protected persistence remain separate
acceptance work. No OTA, MQTT, microSD, cloud inference, partition migration, eFuse,
C6 update, real biometric CI artifact or destructive Flash change was introduced.
