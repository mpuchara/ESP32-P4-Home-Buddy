# Local voice pipeline, iteration 11

## Authorized model provisioning, 2026-10-26

The pinned 2,844,624-byte speaker image is installed at 0xc20000 with esptool
verification and full readback. This changes provisioning status only: real PCM,
selected-slot/model inference, resource cleanup and accuracy remain unaccepted.
Voice identification and VOICE/BOTH gates stay closed. See ITERATION13_REPORT.md.

## Iteration 13 combined readiness

The existing explicit diagnostic publishes `VOICE_READINESS` and the numeric
`hb_voice_diagnostic_report` result after callback retirement and PCM scrubbing.
It joins selected slot, observed sample rate, accepted samples, signal metrics,
512D output validity, model timings and sampled resource metrics in one attempt.
Cleanup flags cover the callback and PCM; model cleanup remains RAII-tested.
No physical audio/model/accuracy acceptance is claimed. Keep calibration and
VOICE/BOTH HA gates closed. See ITERATION13_REPORT.md.


Status: implemented and host-tested foundations; device capture, numerical model
inference, memory peaks, speaker accuracy and 72-hour stability are NOT accepted.
The installed device has not been flashed or reset by this iteration.

## Capture and ownership

ES8311 ADC-only, shared I2C0, RX GPIO48, unused TX GPIO9, 16 kHz stereo PCM16.
One capture owner exposes a bounded synchronous mono consumer; it may only inspect
or copy the current <=320 samples, never block, allocate, infer or call LVGL.
Slot L/R is explicit; diagnostics measure the selected and opposite slot. No
automatic slot choice is made from a successful I2S read. The current frame stamp
is host-device monotonic delivery time, not a hardware ADC timestamp. DMA loss
invalidates the partial voice window. Four DMA buffers total 5,120 B; capture stack
arrays total 1,920 B. The consumer is retired under a short critical section before
its context is freed. Start/Stop before worker dispatch now releases busy state.
No extra I2C reset, output amplifier, storage or network transmission is added.

The one-shot voice diagnostic owns one 48,240-sample PSRAM window (96,480 B), with
atomic handoff to its worker and no full-window queue copy. It stops capture after
one candidate window or 60 seconds. Stop and page navigation cancel capture and
inference. It never enrolls a person and destroys the generated embedding.

## Activity gate, not validated speech VAD

The local preliminary gate uses DC-corrected energy, sample variation, clipping,
DC and short-period checks. It requires >=300 ms activity to enter ACTIVITY,
>=2 seconds activity in the exact 3.015-second window, tolerates shorter pauses,
rejects >=500 ms silence and delivery gaps >40 ms, and bounds history to one
window. Reset is cancellation; no history survives a diagnostic. Engineering
constants are not calibrated speech/identity thresholds. Music, TV and some noise
can pass; overlap is not detected. No inference result becomes an identity.

[ESP-SR VADNet](https://docs.espressif.com/projects/esp-sr/en/latest/esp32p4/vadnet/README.html)
is available through AFE. Its library/model footprint and the separate model
provisioning requirement do not justify adding the full stack before microphone
acceptance. The [ESP-SR changelog](https://github.com/espressif/esp-sr/blob/master/CHANGELOG.md)
records P4 IDF 5.5.1/5.5.2 crash fixes in 2.4.5; do not choose an older release
merely because its manifest accepts IDF 5.5.1. Standalone WebRTC VAD is another
candidate; neither backend is integrated or claimed compatible/validated here.
The dependency lock remains unchanged; speech-selective VAD remains a milestone.

## Pinned embedding provider

The adapter follows [speaker_verification 0.3.1 source](https://github.com/espressif/esp-dl/blob/eb72563e6a4e61d2bbc0d46e4c79f2c9f816c3c6/models/speaker_verification/speaker_verification.cpp)
and its ~3.3.14 dependency with our exact locked ESP-DL 3.3.14. It does not import
the upstream component's dual-model provisioning defaults. Only the 3-second P4
model is admitted, through a whole-package SHA256 check before constructing a
Flash-backed model. Input must be int16 [1,300,80], output int8 [1,512]; sizes,
pointers and per-tensor exponents are checked. Model output is dequantized and
L2-normalized; similarity is cosine, separately from the face model.

Preprocessing uses 16 kHz mono PCM16 /32768, 400-sample symmetric Hamming windows,
160-sample shift, 512-point float FFT, DC removal, replicate-edge 0.97 preemphasis,
80 power mel bins (20..8000 Hz), log(max(energy,float32 epsilon)), per-bin mean
subtraction and P4 half-even INT16 quantization. Window helper tests cover exact
48,240 samples, right-zero padding and center cropping; runtime admits only a full
validated window. It uses locked low-level FFT/audio math with checked scratch and
mel allocations instead of the upstream convenience Fbank's unchecked constructor.
Frame-wise normalization avoids a 192,960 B full float-audio scratch buffer.
The 96,000 B feature scratch plus FFT/window/mel resources are temporary.

A nonblocking shared compute lease preserves mutual exclusion with face/JPEG AI
work. Admission requires >=128 KiB free internal, >=64 KiB largest internal and
>=8 MiB largest PSRAM block. These are provisional safeguards, not measured peaks.
The model is lazy-loaded per explicit diagnostic with Flash-backed parameters and
released on every exit. Inference runs in 16 graph sections with cancellation and
yield points. Upstream constructor/STL allocations and a single graph section are
not preemptible; real allocation-pressure/watchdog acceptance remains pending.
Metrics record sampled free/largest/low memory, stack, load/preprocess/inference
wall time including yields. They are not an exact allocation peak or CPU utilization.
No simultaneous-model fit or inference duration is claimed from host tests.

## RAM identities and fusion

`hb_voice_core` is a preparatory independent schema-1 RAM database with an exact
voice-model namespace, 8 stable household IDs and 4 templates per person. Three
consistent, distinct, non-overlapping windows are needed before matching. Cosine
agreement, unknown threshold, different-person margin and temporal confirmation
are voice-specific inputs; calibrated=false fails closed. No production thresholds
or calibration route is enabled. Unit-test values are synthetic, not product
settings. Identity mapping is supplied by the household owner workflow;
there is no PC enrollment, enabled public voice button or persistent voice write.
A real enrollment journey remains gated by valid microphone capture, real model
inference and measured thresholds. Concurrent speakers remain unsuitable input.

A pure fusion policy handles fresh, session-matched independent confirmations:
same ID -> BOTH, different IDs -> CONFLICT, sole reliable modality -> that source,
otherwise UNKNOWN/AMBIGUOUS. The explicit voice-acceptance gate defaults to absent
at the product boundary: no runtime caller connects fusion to greetings or HTTP.
The existing event bus/HA policy continues to accept FACE only. No raw score mixing.

## Optional USB model package

Raw 2,844,528 B SHA256:
`736ca24bc3430db65b063ce4af7341904d1970d8505aa2eaab0d7be9e760c42e`.
Single PDL3 2,844,624 B SHA256:
`b503ff049a0cd5c62b4c1ac337cf20b7de1549857a4d5329a1821cfb55e9b3a8`.
`sv_model_3s`: subtype 0x43, offset 0xc20000, capacity 3,145,728 B,
headroom 301,104 B. Package index/raw offset is 96 bytes, 16-byte aligned.
No partition changes and no 6-second model.

```
python tools/prepare_voice_model.py PATH/TO/sv_tdnn_tiny_3s.espdl local/speaker-model
python tools/package_firmware.py build-single-http dist/CANDIDATE --speaker-model local/speaker-model/sv_model_3s.espdl
python tools/flash_usb.py dist/CANDIDATE --port COM5 --mode application
python tools/flash_usb.py dist/CANDIDATE --port COM5 --mode speaker-model
```

All commands above are offline preparation/dry-run. `--execute` intentionally
writes only the selected image after installed-table/security preflight; do not
run it without an installation decision. The optional speaker image is NEVER
included in application, face-model or full-mode write plans. Application/model
writes restart the device and erase its present RAM enrollment. No automatic
flashing, NVS/profile/face-model erase, eFuse, C6, OTA, SD or MQTT operation.

## Future passive soak

`monitor_soak.py --port COM5 --hours 72 --output local/soak.jsonl` opens serial with
DTR/RTS deasserted, reconnects after gaps, caps output to 64 MiB and keeps only
whitelisted numeric resource/audio fields plus crash/reset markers. It records
host monotonic time, gaps and uptime regressions; excludes raw logs, names, tokens,
embeddings and PCM. Run using the pinned IDF Python (pyserial installed). This
collector is prepared and parser-tested, not a completed hardware soak. Compare
coverage/gaps, minimum/largest blocks, load/release counts, crashes, audio loss,
network and camera evidence before accepting a continuous run. Existing 30-second
system heap/uptime logs remain; optional diagnostics generate bounded extra data.


## Household workflow and gates (0.10.0-dev)

`household_voice.c` shares the household owner and stable IDs. Begin stages a replacement
without touching existing vectors. Four mutually consistent, distinct non-overlapping
windows enter VERIFY; a fifth fresh utterance must agree with every staged template,
reject duplicates and maintain a different-person margin against other stored voices.
Only explicit Commit replaces that person's voice templates in RAM. No face template
is changed. Rename preserves the ID; deletion/disabled-profile reconciliation scrubs
voice slots. Deadline, cancellation, epoch, stale PCM and failed verification block save.
A household-aware open-set wrapper rejects orphan/disabled IDs before matching and
confirms only fresh independent windows. Thresholds remain uncalibrated and unset.

The firmware command path and Polish enrollment page are compiled. Begin, Sample,
Verify and Save use the existing worker and a single-result mailbox. Opening setup
never starts capture. The disabled public entry and voice switch reflect absent
microphone/model/numerical-on-P4/resources/speech-selection/consent/calibration gates;
there is no UI/NVS/network bypass. No voice/fused greeting or HTTP integration is enabled.
A model failure leaves the facial worker and existing event transport independent.
The 120-second cooperative inference budget checks hash chunks, frames and graph
sections; it cannot preempt a constructor or a single section. Physical timing is pending.

## Reproducible numerical comparison

Run `python tools/generate_voice_reference.py build-host/voice_reference.inc` before
CMake host configuration. Offline: add `--source local/iteration11` with the pinned
source files. The generator verifies full-file hashes at upstream commit
`eb72563e6a4e61d2bbc0d46e4c79f2c9f816c3c6` and extracts unmodified math/Fbank methods.
Those source files are byte-equivalent to the locked ESP-DL 3.3.14 component.
`test_voice_features` compiles the SAME `voice_features.hpp` used by firmware and
compares all 300x80 log-mel entries on silence/sine/multitone/chirp/noise/transient.
Acceptance: max absolute error<=2e-5 log units; INT16 after mean subtraction<=1 LSB
at exponents -12,-8,-4,0. Observed log-mel max error=0 for all six host fixtures.
PCM scaling/crop/padding, saturation, half-even ties, finite inputs, tensor shape,
size/type/null buffers, exponent range/per-channel rejection, output normalization,
allocation faults and greedy-arena aliasing have separate tests.

Both feature paths use the SAME portable host FFT and scalar dot product. This
isolates assembly/order/mel-table/CMVN/quantization defects but does not validate
ESP32-P4 FFT assembly, optimized dot products or actual model operators. Device
numerical execution and reliable speaker identity remain separate acceptance gates.

## VAD admission decision

The quality/activity gate remains active and is NOT production speech VAD.
Compared options: existing gate adds no dependency; standalone libfvad/WebRTC uses
integer GMM speech classification without AFE/model assets; ESP-SR VADNet requires
AFE/model integration and measured coexistence, as documented by Espressif.
A reproducible `evaluate_voice_vad.py` probe pins standalone libfvad commit
`532ab666c20d3cfda38bca63abbb0f152706c369`. C11 ASan/UBSan passed synthetic fixtures;
core-state size is 752 B (not total allocations/stack). At 16 kHz/20 ms, mode 3:
silence 0/151, 440Hz sine 151/151, deterministic full-range noise 150/151,
transient 5/151 frames classified positive. These are probe outcomes, not speech
accuracy, TV/music trials, P4 CPU/Flash measurements or evidence of compatibility.
The observed false positives and lack of household validation do not justify
importing this backend yet. No vendor source or new AFE dependency is in firmware.
The pinned probe is retained for repeatable evaluation; neither alternative is
claimed to reject playback or simultaneous speakers. Continuous listening stays off.
