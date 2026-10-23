# Internal Flash model architecture, iteration 6

No microSD, USB storage, network filesystem or runtime model download is used.
Inference stays on P4/PSRAM. Generic weights are provisioned at installation.

## Measurements and selected mechanism

Clean recognition baseline from 430487a with IDF 5.5.1: app 4,383,872 bytes.
Pinned MBF EDL2: 3,522,832 bytes; SHA256
0cd2629f7111b821f419531d0d3ff33d81018391fa57039acd1022a4531cfd48.
MNP 129,968; MSR 61,248; ESPDet416 578,160 bytes, retained in app rodata.
Zlib level 9 of the actual MBF file is 2,638,927 bytes; XZ default 2,582,280.
Neither fits the former 1,966,080-byte model partition. Compression is therefore
not selected. Startup/decompression peak and performance were NOT measured on P4.
Embedding uncompressed MBF alongside the baseline app would require 7,906,704
bytes before packaging, exceeding both 5 MiB and former 6 MiB OTA slots.

Select the upstream dedicated Flash partition loader. Locked recognition 0.3.3
uses label human_face_feat and a model name lookup in a packed image. Locked
ESP-DL 3.3.14's packer emits PDL3 v1.0 with one MBF entry, data offset 112.
Total image size is 3,522,944 bytes; whole-file SHA256:
f8aa1d21c47814bf324b78ec3a050f993dfa38b1f6a582b6afb5ce0264daf294.
The PDL3 embedded digest (computed with its digest field zeroed) is separately
549e05f7c02c4f10ae97988053eb1cec8c280564e162b1936385e73fe11c6625.
These are different digests by definition. Packaging verifies both contract and
raw MBF digest. Firmware validates exact version/index/size, the entire pinned
package SHA256, read errors and mapping capacity before invoking ESP-DL. Missing,
corrupt or incompatible images fail recognition startup without formatting storage.
The default models/SPIFFS partition is not treated as a compatible model image.

## Candidate map (build-tested, NOT installed)

Bootloader at 0x2000; candidate partition table at 0x10000. A security-assessment
build measured 0x85a0 (34,208) bytes, which FAILED the old 0x8000 table boundary.
The relocated table leaves room for that measured security bootloader. This
assessment never flashed or provisioned a device; signed recovery remains pending. Data alignment is 4 KiB;
application alignment is 64 KiB. Ends are exclusive; total Flash 0x1000000.

| Region | Start | End | Size |
|---|---:|---:|---:|
| NVS | 0x11000 | 0x17000 | 24 KiB |
| OTA metadata | 0x17000 | 0x19000 | 8 KiB |
| PHY | 0x19000 | 0x1a000 | 4 KiB |
| NVS key reserve, encrypted | 0x1a000 | 0x1b000 | 4 KiB |
| alignment gap | 0x1b000 | 0x20000 | 20 KiB |
| ota_0 | 0x20000 | 0x520000 | 5 MiB |
| ota_1 | 0x520000 | 0xa20000 | 5 MiB |
| human_face_feat | 0xa20000 | 0xe20000 | 4 MiB |
| profiles, encrypted flag | 0xe20000 | 0xea0000 | 512 KiB |
| reserved | 0xea0000 | 0x1000000 | 1,441,792 bytes |

partitions.csv remains the original map for standard diagnostic/bringup/wifi builds.
Only config/sdkconfig.recognition chooses partitions.identity.csv. No partition
migration is automatic. Ordinary app OTA is not a table migration. See
FLASH_MIGRATION.md for actual read-only device inventory, private backup and
required explicit authorization. Both slots use rollback-enabled recognition
firmware, but physical OTA/rollback and security-enabled bootloader capacity are
not physically accepted. Current measured app and headroom are in ITERATION6_REPORT.md.
CI enforces at least 256 KiB growth margin; this is an engineering guard, not a
promise that future voice/runtime/TLS features will fit.

## Future voice, explicit capacity limitation

The upstream Espressif speaker_verification model directory inspected 2026-10-23
lists sv_tdnn_tiny_3s.espdl and sv_tdnn_tiny_6s.espdl at 2,844,528 bytes EACH
(blob SHAs 1ac47a6fe8d57bfacdde5b235b5a43f646d816bd and
a9bde025305e4258ee3e79151988e1cbc0baecf7). This is a research candidate from
upstream master, NOT part of our locked dependencies, converted/benchmarked here,
or validated for this device. The README describes P4 support, separate 3/6-second
models and separate partition labels. Even ONE model exceeds the reserve by
1,402,736 bytes. Reserve plus spare MBF partition capacity is 2,113,152 bytes,
still 731,376 bytes short before packing. The current face milestone layout is
therefore NOT a final face+voice production layout.

Onboard-only next options: move shared ESPDet/MNP/MSR weights out of duplicated
app slots and remeasure smaller OTA slots with a combined larger model area;
quantize/compress an explicitly assessed compatible speaker model and measure its
true accuracy/decompression requirements; or replace the dual-app strategy with
an independently verified compact recovery application and a larger main slot.
Each requires another measured proposal and migration approval. No external
storage is an acceptable fallback. A/B model updates would consume still more
space and cannot be assumed feasible. Do not replace MBF with MFN to hide this.

Sources: [locked recognition source](https://github.com/espressif/esp-dl/tree/eb72563e6a4e61d2bbc0d46e4c79f2c9f816c3c6/models/human_face_recognition),
[voice model directory](https://github.com/espressif/esp-dl/tree/master/models/speaker_verification/models/p4),
[voice contract](https://github.com/espressif/esp-dl/blob/master/models/speaker_verification/README.md).
