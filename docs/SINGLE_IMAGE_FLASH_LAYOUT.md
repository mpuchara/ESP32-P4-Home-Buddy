# Single factory image, 0.6.1-dev

16 MiB onboard Flash, 32 MiB PSRAM. One factory app; no OTA metadata or rollback.
All supported build profiles use this same layout. Updating requires P4 USB/ROM
flashing. MBF and personal inference remain onboard. The former dual-OTA table is
obsolete and is not installed by an application-only update.

| Partition | Start | End, exclusive | Capacity |
|---|---:|---:|---:|
| bootloader | 0x2000 | before 0x10000 | 56 KiB maximum |
| table | 0x10000 | 0x11000 | 4 KiB sector |
| nvs | 0x11000 | 0x17000 | 24 KiB |
| small system/config gap | 0x17000 | 0x19000 | 8 KiB |
| phy_init | 0x19000 | 0x1a000 | 4 KiB |
| nvs_keys, encrypted reserve | 0x1a000 | 0x1b000 | 4 KiB |
| alignment gap | 0x1b000 | 0x20000 | 20 KiB |
| factory, app subtype factory/0 | 0x20000 | 0x720000 | 7 MiB |
| human_face_feat, data 0x40 | 0x720000 | 0xb20000 | 4 MiB |
| profiles, encrypted data 0x41 | 0xb20000 | 0xc20000 | 1 MiB |
| sv_model_3s, reserved data 0x43 | 0xc20000 | 0xf20000 | 3 MiB |
| reserved, data 0x42 | 0xf20000 | 0x1000000 | 896 KiB |

Applications are aligned to 64 KiB, data to 4 KiB. Existing security assessment
measured a 45,056-byte unsigned Secure Boot v2 + Flash-encryption bootloader,
which fits before table offset 0x10000. Production signed/release security and
recovery still need separate provisioning acceptance. The actual distributable
image does not enable security or burn eFuses.

MBF remains the exact 3,522,832-byte EDL2 file and SHA256
0cd2629f7111b821f419531d0d3ff33d81018391fa57039acd1022a4531cfd48.
Locked ESP-DL 3.3.14 packages it as PDL3 v1.0, 3,522,944 bytes. The partition
label remains human_face_feat, so the working loader does not need replacement.
Model-partition headroom is 671,360 bytes. No boot download or external storage.

## Voice capacity assessment, not a voice implementation

Inspected Espressif upstream speaker model sv_tdnn_tiny_3s.espdl is actually
2,844,528 bytes, EDL2, SHA256
736ca24bc3430db65b063ce4af7341904d1970d8505aa2eaab0d7be9e760c42e.
Raw provisioning leaves 301,200 bytes in 3 MiB. Packaging this exact basename
with the locked PDL3 packer yields 2,844,624 bytes (96-byte model offset), leaving
301,104 bytes. Thus one such asset fits with alignment/packaging accounted for.
Both 3s and 6s models do NOT fit together. These are asset/format measurements,
not runtime/operator compatibility or accuracy validation of the current locked
ESP-DL component. No speaker code/dependency is added in this iteration.

Upstream runtime explicitly selects sv_model_3s for a 3-second model and sv_model_6s
for a 6-second model, using dl::Model(partition_label, FLASH_PARTITION), which also
supports a raw EDL2 image. Its current CMake flashes BOTH models by default.
Future integration must select the 3-second model explicitly and adapt the upstream
flash target to provision only sv_model_3s. Do not import that two-partition default
blindly or select the 6-second default with a missing partition.

Profile capacity is detailed in PROFILE_DATABASE.md. The second 512 KiB is reserved
for a future independently versioned voice database; no voice templates are assumed
compatible with the face format. Preserve source-specific thresholds and reject
face/voice identity conflict rather than choosing arbitrary cross-model similarity.

Sources inspected 2026-10-23:
[model files](https://github.com/espressif/esp-dl/tree/master/models/speaker_verification/models/p4),
[speaker source](https://github.com/espressif/esp-dl/blob/master/models/speaker_verification/speaker_verification.cpp),
[provisioning CMake](https://github.com/espressif/esp-dl/blob/master/models/speaker_verification/CMakeLists.txt).
