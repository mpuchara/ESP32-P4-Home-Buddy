# Iteration 11 candidate, 2026-10-25

Version **0.10.0-dev**, based on `15f7fd00eeba9593a4b2f06841ed8e5a19bef5c4`,
branch `codex/local-face-recognition`, existing PR #2. Final source commit and
artifact SHA256 values are recorded in the candidate's `usb-package.json` and
`SHA256SUMS`. No production release or PR merge. No board installation/reset.

| Required result | Delivered status and boundary |
|---|---|
| Speaker pipeline | Existing microphone/window/model adapter preserved. Single-result mailbox joins it to the household enrollment worker. Capture is explicit, bounded and cancellable; no background listening. |
| Numerical comparison | Actual feature extractor versus hash-pinned Espressif source: all 24,000 log-mel values on six synthetic signals; max absolute difference 0 on host, tolerance 2e-5. CMVN/INT16 within 1 LSB at four exponents. Shared host FFT/dot-product shim means P4 DSP/model equivalence remains pending. |
| Numerical fault safety | Added null tensors/buffers, dtype/size, invalid/per-channel/out-of-range exponent cases; retained greedy-arena alias guard, normalization, finite-input, saturation, quantization and allocation/cancellation/cleanup tests. |
| VAD | Existing activity detector remains explicitly experimental. Standalone pinned libfvad probe passed ASan/UBSan but mode3 accepted 151/151 sine and 150/151 noise frames. No justified household benefit or speech accuracy evidence; no VAD/AFE dependency admitted. See VOICE_RECOGNITION.md. |
| Microphone hardware | Pending. No invented silence/speech, slot, cadence or touch coexistence receipt. GPIO48 RX and one I2S owner preserved. |
| Embedding hardware | Pending. Packaged model is verified offline; no physical model execution or speaker accuracy claim. |
| Load/preprocess/inference | Instrumented, including cooperative 120-second inference deadline. No P4 measurements. Constructors/individual graph sections remain nonpreemptible. |
| RAM/PSRAM/stack | Sampled resource diagnostics retained; load/release counters and numeric soak fields added. No measured board peak or simultaneous-model fit. Optional voice DB 65,768 B; unified snapshot 312,416 B. |
| Enrollment | Internal four-template staging, distinct/non-overlapping utterances, fresh verification against every template and other people, explicit atomic RAM commit, cancel/timeout/error handling. Polish page/command path compiled. Public entry remains disabled by absent physical/speech/resource/consent/calibration acceptance. |
| Open-set identification | Household-aware wrapper rejects orphan/disabled IDs, uses existing separate voice thresholds/UNKNOWN/AMBIGUOUS and temporal confirmation. Calibrated=false remains fail-closed; no product calibration values or active identity publication. |
| Fusion | Separate face/voice session IDs now reject cross-session observations. Fresh same-person BOTH, different-person CONFLICT and sole validated modality are host-tested. No runtime voice publisher or face-event wait is introduced. |
| Household profiles | Existing IDs/names remain authoritative. Rename preserves association, deletion reconciles voice slots, face/voice replacements are independent. Synthetic tests cover these paths. |
| Persistence | Separate schema-2 engine, two 512 KiB banks in unchanged 1 MiB partition, independent namespaces and stable-ID checks. First authorized migration preserves both legacy banks; interrupted migration refuses dirty retry. No runtime activation or destructive migration. |
| Security decision | Retain RAM-only now; implement/review owner-unlocked AEAD next. Hardware-protected unattended operation requires matching-spare provisioning and proved secure USB recovery. PERSISTENCE_DECISION.md details all three options and limitations. No plaintext fallback, key generation or eFuse writes. |
| Home Assistant | Existing FACE-only metadata payload, bounded queue, freshness and independent cooldowns preserved. No raw audio/images/embeddings, VOICE/BOTH events or second publisher. Hardware regression remains pending. |
| Automated validation | 30 C/C++/LVGL host tests, 12 Python tests and 3 project checks; ASan+UBSan across identity, PCM, voice, adapter, audio owner, household workflow, schema2, reference features and real voice capture handoff. Final build/config/Flash/CI receipts are recorded with delivery. |
| GitHub Actions | Existing five-job matrix retained; numerical reference generation added to host job. Inspect final-head run linked from PR #2; CI does not establish hardware acceptance. |
| USB package | Recognition application, symbols/map, bootloader/table, face model, optional verified SV image, validators, documentation and SHA256 manifest. Speaker-model write stays separate; application/full plans do not add it. Packaging is not installation permission. |
| Physical acceptance | One combined session in ITERATION11_ACCEPTANCE.md. No prolonged manual observation requested. Passive 72-hour collector prepared; no completed soak claimed. |
| Next milestone | Authorized candidate installation, one slot/silence/speech + real embedding/resource receipt, then consenting-speaker speech-selection and open-set calibration. Separately engineer/review owner-unlocked storage. |

## Reproduce the automated evidence

Generate pinned reference sources before configuring host CMake:
`python tools/generate_voice_reference.py build-host/voice_reference.inc`.
Build `tests` and run `ctest --test-dir build-host --output-on-failure`.
Run `python -m unittest discover -s tests -p 'test_*.py'`,
`python tools/check_project.py`, `python tools/check_format.py`,
`git diff --check`, and `UBSAN_OPTIONS=halt_on_error=1 bash tools/test_sanitizers.sh`.
The sanitizer script regenerates and compiles the numerical reference independently.

VAD probe: checkout `dpirch/libfvad` commit
`532ab666c20d3cfda38bca63abbb0f152706c369`, then run
`python tools/evaluate_voice_vad.py PATH_TO_CHECKOUT OUTPUT_DIR` on a C11 host.
It prints both sanitized and ordinary probe results; no firmware import occurs.
Its synthetic outcomes must not be presented as real speech accuracy or P4 usage.
