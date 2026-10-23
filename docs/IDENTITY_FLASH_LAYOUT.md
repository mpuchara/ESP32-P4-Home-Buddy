# Identity model flash assessment (no migration applied)

Measured original Wi-Fi binary rebuilt from 223a4ff: **4,111,328 bytes**.
Managed P4 generic model files: MBF **3,522,832**, MNP **129,968**, MSR **61,248**,
ESPDet416 **578,160** bytes. MBF SHA256:
`0cd2629f7111b821f419531d0d3ff33d81018391fa57039acd1022a4531cfd48`.
These are actual file sizes, not RAM estimates. Firmware build receipts are in
IDENTITY_VALIDATION.md. Detection/landmark code and alignment growth are included
in the measured experimental app; MBF weights are deliberately external on SD.

## Current map, unchanged

| Area | Offset | Bytes | End (exclusive) |
|---|---:|---:|---:|
| NVS | 0x9000 | 0x6000 | 0xf000 |
| OTA metadata | 0xf000 | 0x2000 | 0x11000 |
| PHY | 0x11000 | 0x1000 | 0x12000 |
| OTA0 | 0x20000 | 0x600000 | 0x620000 |
| OTA1 | 0x620000 | 0x600000 | 0xc20000 |
| profiles | 0xc20000 | 0x200000 | 0xe20000 |
| models | 0xe20000 | 0x1e0000 | 0x1000000 |

Current models capacity is 1,966,080 bytes: MBF alone exceeds it by 1,556,752.
Original Wi-Fi + MBF alone is 7,634,160 bytes, already larger than a 6,291,456-byte
OTA slot before recognition code growth. Keeping the two OTA slots and the 2 MiB
profile area leaves no contiguous internal location for MBF. The current models
SPIFFS partition is not an ESP-DL raw `human_face_feat` model partition.

## Experimental deployment selected

Keep the partition table, settings, existing profiles and OTA capacity unchanged.
Store only generic MBF on local FAT microSD; MNP/MSR and ESPDet weights remain
in application rodata. No runtime download. The 3,522,832-byte MBF file is checked
against its pinned digest before construction. No household data is written to SD.
The app guard checks that the resulting experimental binary still fits OTA.
Model load/inference activations must be measured separately; file size is not
inference RAM. SDMMC slot 0 hardware/mount/dual-slot coexistence acceptance is pending.

## Exact internal-flash candidate, proposal only

This option is **not applied** and is not approved for migration or flashing.
It requires reducing OTA slots to 5 MiB and moving/shrinking the existing profile
partition. Production must validate growth/recovery and voice requirements first.

| Area | Offset | Size | End |
|---|---:|---:|---:|
| NVS/OTA metadata/PHY | unchanged | unchanged | unchanged |
| OTA0 | 0x20000 | 0x500000 | 0x520000 |
| OTA1 | 0x520000 | 0x500000 | 0xa20000 |
| generic models | 0xa20000 | 0x400000 | 0xe20000 |
| profiles | 0xe20000 | 0x80000 | 0xea0000 |
| future voice reserve | 0xea0000 | 0x160000 | 0x1000000 |

MBF+MNP+MSR files total 3,714,048 bytes, leaving 480,256 bytes inside the proposed
4 MiB models area before pack/alignment/filesystem overhead. ESPDet stays in the
app. A packed raw image needs labels compatible with its loader, or a separate
filesystem-backed loader; the table alone does not implement either. A 512 KiB
profile area exceeds the 245,760-byte maximum feature payload, but transactional
two-copy storage, encryption metadata, wear levelling and future voice templates
could exceed it. The 1,441,792-byte voice reserve is explicitly provisional because
no voice model has been selected. Thus this is a capacity candidate, not a settled
production storage architecture. Staying on SD or using larger flash may be preferable.

## Mandatory migration procedure before applying any candidate

1. Record actual active OTA slot, version, flash encryption/Secure Boot state and
   all installed partition entries. Stop new profile/settings changes.
2. Make a private verified 16,777,216-byte full P4 flash backup and SHA256, plus
   logical settings/profile exports appropriate to their schema/security state.
   Store backups privately, outside Git/CI/release artifacts, and test recovery.
3. Explicitly authorize destructive relocation; flashing the new table without
   migration can overwrite the old profile region and old OTA application.
4. With a recovery image/tool and physical access, migrate compatible settings and
   versioned profiles to a fresh layout, verify integrity/decryption and only then
   activate firmware. Preserve model IDs; never reuse a different model database.
5. Verify both OTA slots, rollback selection, settings persistence and profile
   recovery. Normal OTA cannot safely substitute for a partition-table migration.
6. Recovery uses the same device's complete verified backup and matching security
   provisioning. Restoring only the old app is insufficient after a changed map;
   profiles created after backup would be lost. Secure Boot/flash encryption may
   further limit raw rollback; do not burn eFuses during this experiment.

No erase, migration, settings/profile write, flash-encryption enablement, production
release or firmware flash was performed in this iteration.
