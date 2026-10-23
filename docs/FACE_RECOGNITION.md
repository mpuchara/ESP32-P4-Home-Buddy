# Local household recognition, 0.5.0-dev

This is an **experimental RAM-only implementation**, not completed hardware acceptance.
The baseline is development branch commit `223a4ff1977b6bcb3bdb73f013b59801b6d30c7d`.
Remote branch heads were checked before development; main contained only the initial commit.
No device was flashed, no partition table was changed, no C6 update was performed.

## Architecture implemented

A separate bounded worker uses the existing physical camera and compute mutex.
ESPDet416 runs on the existing upright 234x416 RGB565 input. Its boxes are mapped
back using the actual thumbnail sampling convention. A native-resolution, bounded
face ROI (20% context per side, at most 640x640) is extracted from that **same**
RAW8 snapshot. Oversized ROIs are rejected rather than secretly downscaled.
GBRG cell demosaicing, black-level correction, bounded gray-world white balance
and sRGB conversion produce RGB888. This color pipeline is provisional and needs
camera validation, especially low light and saturation.

MNP receives a candidate box in ROI coordinates. It makes the box square, clips
it to the image, and transforms predicted landmarks through the actual crop
and resize. This is API/coordinate compatible with ESPDet candidates by source
inspection; recognition quality of this combination is **not experimentally
validated**. The diagnostic MSR/ESPDet button switches to official MSR -> MNP
on the same native ROI for an on-board comparison. ESPDet remains the primary
scene detector in both modes. This is not a full-scene MSR accuracy comparison.

The five points are validated before `HumanFaceFeat::MBF_S8_V1::run`. The
upstream feature preprocessor fits a similarity transform and resizes to
112x112 RGB, using (pixel-127.5)/127.5. It does not accept an empty keypoint list.
The wrapper validates expected input shape, 512 float outputs, finiteness and
L2 norm. Upstream postprocessing normalizes features; dot product is therefore
cosine similarity. No MFN fallback exists.

The portable core is isolated from ESP-DL and FreeRTOS. Another model backend
must use another model namespace/database. `HB_ID_MODEL` is checked in database
operations. Generic MBF weights are size/SHA256 checked before model construction.

## Memory ownership and scheduling

One RAW8 scratch buffer, allocated once per loaded recognition session, preserves
937,664 bytes (1288x728) after the camera lease is released. This is the bounded
source data needed to identify the ROI after detection. There is no per-candidate
full-resolution allocation and no full-resolution RGB copy. ROI scratch is
1,228,800 bytes, detection input 194,688 bytes. A 180x318 RGB565 diagnostic snapshot
uses 114,480 bytes; the UI has its own preview buffer while that page is open.
The in-memory database holds at most 8 people x 15 x 512 floats (245,760 feature
bytes plus metadata); enrollment staging holds at most 30,720 feature bytes.
These are allocation bounds, **not measured peak inference RAM**.

Model work, allocation/unloading and inference share `hb_compute_enter` with JPEG
work. Queue capacity is eight commands, one inference is active, and all AI work
runs at priority 1 outside LVGL. The UI polls a mutex-protected status snapshot
on its existing timer; greetings use the existing LVGL command queue. No worker
accesses LVGL objects. Exact-frame thumbnails prevent landmark overlays on a
newer camera image. Initial resource admission requires 20 MiB PSRAM free,
12 MiB contiguous PSRAM, 180 KiB internal free and a 64 KiB contiguous internal
block. After load at least 6 MiB PSRAM and 96 KiB internal must remain. These
are conservative engineering guards awaiting board measurements, not acceptance.

Stop/navigation/cancellation invalidate the session immediately; in-flight output
cannot greet. Models and capture scratch unload under the compute mutex after
stop. Profile/enrollment storage belongs to the worker. Camera leases end before
inference. The mounted model card remains mounted across sessions. FAT mount
never formats a card, uses BSP TF pins on SDMMC slot 0, and keeps C6 on slot 1.
Default IDF slot-scoped deinit is preserved on mount failure. Actual dual-slot
Wi-Fi/SD behavior still requires a physical test. Upstream ESP-DL constructors
can assert on internal allocation/model failures; prechecks reduce that risk but
cannot establish failure isolation without hardware stress tests.

## Matching and temporal policy

Each enabled person needs at least 8 templates. For every person calculate a
fixed top-three mean and require at least three templates above the agreement
floor. Compare the top two **different people**, including plausible runners-up
whose score is below acceptance. Extra templates never add to a sum, although a
larger set can still change the order statistics; balanced enrollment counts and
measured calibration remain necessary. Quality metrics are diagnostic, not
unvalidated hard cutoffs. The initial cosine threshold is 0.70, agreement floor
0.60 and different-person margin 0.12, configurable as thousandths in menuconfig.
These are deliberately conservative **uncalibrated engineering values**. Raw
similarity is never a probability or accuracy percentage.

Insufficient evidence returns UNKNOWN; close different-person candidates return
AMBIGUOUS; invalid data/model returns UNAVAILABLE. KNOWN is a per-frame candidate.
Three consistent accepted identities from distinct chronological frames confirm
identity. Duplicate, stale (>5 s), future, out-of-order and cancelled results are
rejected. Unknown/ambiguous/failed inference resets confirmation. A >6 s gap
breaks the streak. Sequence wrap/restart requires a new session. Identity and the
existing temporal face-presence diagnostic remain separate.

## On-device enrollment and UI

Menu -> Domownicy creates a named profile only after the explicit add button.
The name is editable without changing its 64-bit boot-namespace/counter ID.
A deliberate delete confirmation clears that profile and all its templates;
freed array slots never reuse the former ID. No real profiles are preinstalled.

Select a profile -> Rejestruj / dodaj probki. Position the face with the local
preview, capture 8-15 diverse samples, verify using a new frame, then explicitly
confirm saving. Samples must be >=1 second apart; cosine >0.995 is treated as
redundant. Invalid geometry/no face/multiple faces are rejected. Use small head
turns, glasses and varied lighting; the sample count/diversity rule is an initial
engineering choice, not an established optimum. Verification requires three
matching staged templates and rejects a conflict with a different stored person.
Cancellation leaves committed templates unchanged. An additional-samples session starts with the committed templates in staging,
adds diverse new samples up to the 15-template bound, then verifies and commits
the complete set atomically in RAM. Cancellation never changes the committed set.

Menu -> Rozpoznawanie twarzy shows the exact inference preview/ROI/landmarks,
quality measurements, candidate versus confirmation, top person and different-person
similarity, detection/landmark/MBF+alignment/total times, memory and model state.
MBF timing includes upstream geometric alignment and preprocessing; alignment
alone is not separately instrumented. Test sessions never greet.

The background button explicitly enables an independent **10-minute recognition
session**. Select the saver to keep UBox fullscreen, clock and Open-Meteo visible.
Only fresh confirmed background identity invokes the existing fading greeting,
with a five-minute cooldown per stable person ID. Returning to the menu does not
turn face-presence diagnostics into permanent recognition. Always-on persistent
opt-in remains deferred until memory, thermal, power and stability acceptance.
To stop, open recognition diagnostics and press Stop, or wait for expiry.

## Local evaluation

With consenting participants, enrollment and verification/testing must use different
frames. From Domownicy select a person -> Testuj rozpoznawanie; 'Wybrana osoba'
labels subsequent trials as that selected person. 'Gosc' labels an unknown visitor.
'Ocen probe' records a deliberate current trial, once per distinct frame. Counts
remain local in RAM; no image/embedding export exists. The display shows true
accepts, false rejects, enrolled-person confusions and unknown accepts/trials.
`hb_id_evaluation` also exposes unknown rejections, ambiguous results and unavailable
attempts. No count is presented as statistically established accuracy. Repeated
adjacent frames are correlated: for independent acceptance measurements, separate
trials by a scene exit/re-entry and document participant/condition counts manually.
The automatic trial counter alone cannot guarantee a statistically independent set.

For known trials, report TAR=true_accept/known_trials; distinguish FRR=false_reject/
known_trials from enrolled-person confusion/known_trials. For unknown trials,
FAR=unknown_accept/unknown_trials and rejection=unknown_reject/unknown_trials.
Ambiguous trials are included among rejected decisions and also counted separately.
Unavailable attempts have a separate denominator and must not disappear from the
report. Zero denominators mean unavailable, never 0% success/failure. Test same
person across lighting, distance, yaw/pitch, glasses and expressions; include similar
household members, consenting unknown visitors, two faces, obstruction and entry/exit.
Report the counts and confidence limitations before changing thresholds. Do not
lower thresholds to improve the displayed known rate.

## Storage and privacy

**RAM-only. Restart loses every profile, name, template and evaluation counter.**
No biometric persistence, database file parser, schema migration or encrypted
biometric store is implemented. Corrupt persistent database/failed-write/reboot
recovery acceptance is therefore pending, not represented by synthetic tests.
The database model namespace prevents incompatible in-memory feature sets.
The upstream `dl::recognition::DataBase` is not used: in the locked source,
`query_feat` returns a loop index instead of the stored feature ID; enrollment
mutates memory before storage writes finish and is not a household transaction.

Existing settings/NVS and profiles partitions remain untouched. NVS is plaintext
in this development firmware. Flash encryption/NVS encryption would require an
explicit provisioning design, backup/recovery policy and device-state verification.
Neither protects a plaintext custom biometric file automatically. This iteration
writes no templates and burns no eFuses or keys. RAM-only storage is not protection
against a compromised firmware/debug interface. No images, embeddings or biometric
records enter Git, Wi-Fi, MQTT, CI or firmware packages. Debug output contains only
resource/timing metadata, never embedding values. No cloud/HA/PC biometric processing.
Recognition is not liveness detection and must not unlock doors/disable security.

An internal FACE event accessor exposes status, confirmed stable ID, similarities,
session/frame and timestamp, independently from greeting cooldown. There is no
network/MQTT publisher. The interface reserves VOICE/BOTH sources but emits only FACE. Entered/left
events use their own presence tracker, separate from identity confirmation and the
existing diagnostic tracker. The future single local consumer must poll each result;
this bounded latest-result API is not a lossless historical event log.

## Deployment and acceptance

Use the separate `config/sdkconfig.recognition` profile. The three normal profiles
compile with experimental model execution disabled. Run:

```powershell
idf.py -B build-recognition -D SDKCONFIG=build-recognition/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;config/sdkconfig.recognition" build
python tools/prepare_identity_model.py local/sd-staging
python tools/identity_flash_budget.py build-recognition
```

Copy the generic `models/` directory to a FAT microSD outside the device. The
experimental worker mounts the existing TF slot only on explicit recognition start.
Do not hot-remove the card while loading. Mount/read/model failure reports unavailable.
This is generic model provisioning; personal enrollment remains entirely on the P4.
See [flash assessment](IDENTITY_FLASH_LAYOUT.md) before any future migration.
See [actual validation](IDENTITY_VALIDATION.md) for build/test receipts and blockers.

## Inspected primary sources

- [Recognition 0.3.3 registry](https://components.espressif.com/components/espressif/human_face_recognition/versions/0.3.3/readme), package source commit `eb72563e6a4e61d2bbc0d46e4c79f2c9f816c3c6`.
- [Detection 0.5.1 registry](https://components.espressif.com/components/espressif/human_face_detect/versions/0.5.1/readme), package source commit `17e9abb1c59cd66b77b4bdb95587abcfafbb124a`.
- Locked ESP-DL 3.3.14: `dl_feat_image_preprocessor.cpp`, `dl_feat_postprocessor.cpp`, `dl_detect_mnp_postprocessor.cpp`, `dl_recognition_database.cpp` in resolved managed source.
- [Espressif flash encryption](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32p4/security/flash-encryption.html) and [NVS encryption](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32p4/api-reference/storage/nvs_encryption.html).

Recognition component and generic weights ship under the component MIT notice,
retained in `components/identity/vendor/MBF-MIT.txt` and SD staging. Published
93.94% TAR at FAR=1e-4 and 146.2 ms P4 inference are upstream IJB-C/reference-board
benchmarks, not results from this GUITION device or this camera pipeline.
