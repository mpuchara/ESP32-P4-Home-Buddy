# Iteration 6 development report: 0.6.0-dev

Acceptance is INCOMPLETE pending authorized physical migration and separate secure
provisioning. No new firmware was flashed, no Flash erase/partition migration,
security enablement/eFuse operation, C6 update, PR merge or production release was
performed. HA_UBOX_bridge was not changed. Work continues on PR #2's existing
codex/local-face-recognition branch, checked against origin at 430487a.

## 1. Storage architecture and deployment

MBF remains MBF_S8_V1. Dedicated onboard human_face_feat Flash holds the pinned
PDL3 package; PSRAM/internal RAM supply runtime allocations. ESPDet416, native
ROI, MNP, five-point alignment, 512-float matching and temporal confirmation remain.
No microSD mount, external model storage, boot download or external inference is
used by the implementation. Normal installation provisions the model together with
the firmware. Packaging checks raw/package size, version and SHA256; runtime checks
the exact package before ESP-DL. Missing/corrupt/incompatible/read/mapping failures
return recognition diagnostics while the existing independent dashboard/services
continue. Resource admission guards, camera lease release, shared compute mutex,
bounded queues and cancellation epochs are retained.

## 2. Exact candidate Flash map

| Region | Offset | Size |
|---|---:|---:|
| P4 bootloader | 0x2000 | ends before 0x10000 |
| partition table | 0x10000 | 0x1000 sector |
| NVS | 0x11000 | 0x6000 |
| OTA metadata | 0x17000 | 0x2000 |
| PHY | 0x19000 | 0x1000 |
| encrypted NVS-key reserve | 0x1a000 | 0x1000 |
| ota_0 | 0x20000 | 0x500000 |
| ota_1 | 0x520000 | 0x500000 |
| human_face_feat | 0xa20000 | 0x400000 |
| encrypted profiles | 0xe20000 | 0x80000 |
| reserve | 0xea0000 | 0x160000 |

The initial Flash-encryption-only assessment measured 34,208 bytes (0x85a0) and
failed the old 24,576-byte limit at table offset 0x8000. A second build with
effective SECURE_BOOT=y, SECURE_BOOT_V2_ENABLED=y and SECURE_FLASH_ENC_ENABLED=y
measured 45,056 bytes (0xb000), leaving 12,288 bytes before the revised 0x10000
table boundary. SECURE_BOOT_BUILD_SIGNED_BINARIES was disabled: no signing key
was generated or used. This offline bootloader size check PASSED. No assessment binary
is an authorized provisioned recovery image. NVS/OTA/PHY relocation is intentional
and requires the reviewed migration procedure. Defaults for other profiles retain
the original table at 0x8000; recognition explicitly chooses the candidate.

## 3. Measurements and capacity

- Clean original recognition rebuild: 4,383,872 application bytes.
- MBF: 3,522,832 bytes, requested raw SHA256 preserved.
- PDL3 model image: 3,522,944 bytes; 671,360 model-partition bytes remain.
- Actual MBF compression: zlib9 2,638,927; XZ 2,582,280 bytes; neither fits old models.
- Final recognition application: **4,438,640 bytes**; **804,240 bytes** remain
  in each 5 MiB OTA slot (15.34%). This exceeds the 256 KiB engineering growth
  guard but does not guarantee future voice/release features will fit.
- Application SHA256: ec2f80d2064e1ec4274ce11ef4e675cd036ca22344b45fe457ec0f110de1cf17.
- Reserve: 1,441,792 bytes. This is NOT enough for the researched 2,844,528-byte
  Espressif speaker model. No voice implementation was added. Onboard-only future
  layouts and recovery tradeoffs are in IDENTITY_FLASH_LAYOUT.md.
- Profile snapshot: 246,640 bytes plus 128-byte header per 262,144-byte bank.
  Total face capacity is 8 people x 15 templates. PSRAM scratch is one bounded
  extra database; event ring has 16 entries and 3 independent consumer cursors.
- New-firmware model load/inference memory, latency, JPEG coexistence, CPU/UI
  responsiveness and camera stability are UNMEASURED on hardware. Existing-device
  serial inventory does not validate this new pipeline.

## 4. Persistence and security

Versioned two-bank database implements create/rename/append/delete, persistent
next-ID, model namespace checks, SHA256 integrity, commit-last header, readback and
prior-generation recovery. Deletion scrubs the obsolete bank after commit. Failure
is reported; no format-on-corruption path exists. Alternating explicit commits
bound writes; this is not a general wear-levelled filesystem.

Persistence requires explicit firmware opt-in, hardware Flash encryption AND
Secure Boot and the encrypted profile partition. Actual ROM query on this P4:
both security features DISABLED, SPI_BOOT_CRYPT_CNT=0. It therefore remains
explicitly RAM-only. Ordinary settings/MQTT credentials are plaintext development
NVS. Hardware encryption, signing-key recovery and encrypted NVS provisioning are
separate authorization steps, not a claimed implemented security certification.
Profile SHA256 detects corruption; no authenticated-record/anti-replay guarantee
is made. See PROFILE_DATABASE.md and FLASH_MIGRATION.md for power-loss/scrub limits.

## 5. Identity, greetings and Home Assistant

Bounded thread-safe event ring supports recognized, unknown, ambiguous,
presence_enter and presence_leave. IDs include device/boot/counter, person ID when
persistent, timestamp/freshness/session/encounter, method/status and similarities.
Duplicate encounter events and stale/cancelled results are rejected. Overflow is
counted independently for each consumer. LVGL and MQTT consumption are independent;
five-minute per-person greeting cooldown remains separate from MQTT cooldown.
Thresholds are unchanged and uncalibrated; no real recognition accuracy observations
or false-identification rates are available for this iteration.

Native ESP-MQTT 5 provides local configuration UI, authentication, optional CA-bundle
TLS/hostname checks, retained Discovery/availability LWT, HA birth rediscovery and
non-retained QoS 0 identity events with five-second expiry. Disconnected queues are
drained rather than replayed. Network operations run outside AI/LVGL. Six default
entities: event.home_buddy_recognition; sensor.home_buddy_last_person,
sensor.home_buddy_identity_status, sensor.home_buddy_identity_method;
binary_sensor.home_buddy_face_detected and binary_sensor.home_buddy_identity_available.
Names/events only, no images/templates/embeddings. Four editable automation examples
and transport limitations are documented in MQTT_API.md. No real broker/HA test is
claimed until after physical migration. No security-sensitive unlocking automation.

## 6. Verification receipts

Host Windows suite: 15/15 passed, including preserved camera/brightness/JPEG-facing
core/dashboard/weather/font/navigation/detection/presence/quality checks and the
new profile/event/MQTT payload tests. Pure identity/storage/event tests also run
under GCC AddressSanitizer and UndefinedBehaviorSanitizer on WSL. Python checks
validate original/candidate alignment, capacity/overlap rejection, pinned actual
model packing and corruption/version/size failures; project/dependency/font checks
and formatting/whitespace guards remain enabled. Pinned dependency versions were
not changed.

Fault injection covers erase/write/read boundaries and torn commit headers, newest
copy corruption with fallback, maximum templates, model mismatch, protected-storage
gate, stable identifiers and reboot restoration. The injected checksum tests the
transaction protocol; firmware uses mbedTLS SHA256. These are not hardware power
loss, encryption, TLS/authentication, real HA restart or inference acceptance tests.

Code commit: `b4f8bc5cf420527ea7298396d89f6ec0cf10827d` (implementation
98c7726 plus invalid-event-type boundary guard). [CI run 39983216326](https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39983216326)
completed **successfully** for this exact SHA: host plus diagnostics, bringup,
wifi and recognition firmware. A manually dispatched earlier run/duplicate was
cancelled once native PR CI appeared; cancelled superseded runs are not passes.

| Local profile | Application bytes | Build / effective configuration |
|---|---:|---|
| diagnostics | 227,536 | PASS |
| bringup | 3,907,360 | PASS |
| wifi | 4,204,368 | PASS |
| recognition | 4,438,640 | PASS |

Local Windows host suite 15/15 and WSL GCC ASan/UBSan storage/event/identity
checks passed. Python model/layout suite 3/3 (actual pinned model present) and
project/dependency/font checks 3/3 passed; formatting and whitespace checks passed.
Recognition packaging verified every SHA256SUMS entry, model image/offsets and
absence of private files. Its sdkconfig confirms Flash encryption, Secure Boot
and profile persistence are NOT enabled in the distributable development image.

Review package: dist/home-buddy-0.6.0-dev-b4f8bc5-review.zip, 16,402,652 bytes;
SHA256 b2c0d31e25afcf6f856c459f804e79c036913309cebccd050dec4d4951d0b099.
It contains only public firmware/model/license/build files and the migration warning.
Private backup and relocation images are deliberately separate. No production
release was created. Subsequent receipt-only documentation edits do not change
the tested code; see the code SHA above rather than treating a skip-CI receipt as
an independently tested firmware commit.

## 7. Physical preflight completed, acceptance still required

Private full Flash backup: 16,777,216 bytes with SHA256 verified, outside Git and CI.
Partition-table MD5 and OTA CRC validated; selected ota_0/sequence 1/state VALID,
installed app 0.4.5-dev. Former profiles/models ranges are all 0xff. Bootloader header
present; existing camera/runtime observed after read. Private NVS/PHY relocation
images and per-file hashes are prepared outside the repo. No credentials/backup
contents are published. Re-snapshot if settings change before migration.

Required next steps: explicitly authorize the concrete offline P4 migration while
preserving/relocating settings; cold-boot and execute real Flash MBF, local enrollment,
landmark/alignment, separate known/unknown/ambiguous trials, model/JPEG memory and
latency/soak, both OTA slots and rollback, and real MQTT/TLS/HA automation. Provision
security only under a separate reviewed authorization after signed recovery tests,
then execute enroll/save/power-cycle/recognize/rename/power-cycle/delete acceptance.
The ten-minute background development lease is retained; always-on production
recognition is not enabled before these measurements. Future voice capture/VAD/
embedding/persistence/fusion interfaces are contracts only, and conflict must remain
AMBIGUOUS rather than arbitrarily choosing a source.

Recommended next milestone: authorized face-only hardware validation and recovery,
then secure persistence acceptance. Before adding speaker identification, remeasure
an onboard-only combined-model layout; the current reserve cannot be assumed enough.
