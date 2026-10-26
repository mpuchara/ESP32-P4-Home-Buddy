# P4 USB installation, update and recovery

## Current iteration 12 policy (0.11.0-dev)

The selected product mode is **persistent, unencrypted, local household storage with
automatic restoration**. The recognition build runs the existing schema-2 engine in
two 512 KiB banks inside the unchanged 1 MiB partition. Intentional household changes
save automatically; valid face/voice profiles and the previously enabled AUTO mode
restore locally at startup. No password, PIN, manual unlock, eFuse provisioning or
mandatory encryption. Physical extraction is not prevented. Experimental voice
identification remains gated. Implementation/host tests do not establish physical
reboot persistence; installation and the combined acceptance session remain pending.
See [storage decision](PERSISTENCE_DECISION.md), [database](PROFILE_DATABASE.md) and
[iteration report](ITERATION12_REPORT.md). Older iteration notes below are historical.


Use the existing Windows ESP-IDF 5.5.1 environment. No Wi-Fi OTA, second app image,
chip erase or C6 firmware update is supported/needed. P4 is COM5 in this session;
identify the actual P4 port with python -m serial.tools.list_ports -v. Never select
C6 by guesswork. Close serial monitors before flashing. Public packages have only
generic models/code; private Flash backups and credentials must stay outside Git/CI.

## Build and package

```powershell
idf.py -B build-single-http -D SDKCONFIG=build-single-http/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;config/sdkconfig.recognition" build
python tools/check_effective_config.py build-single-http/config/sdkconfig.json recognition
python tools/identity_flash_budget.py build-single-http --enforce
python tools/package_firmware.py build-single-http dist/single-http
```

Use a fresh sdkconfig; CMake rejects an old partition-table offset/rollback setting.
MBF is built into a separate verified PDL3 model partition image, not into the app.
Build resolves generic assets on the development PC; normal boot/enrollment does
not download models or require that PC. Verify SHA256SUMS before installation.
All following flash_usb examples default to a dry run unless --execute is present.
The tool requires Python with the matching esptool, normally IDF's activated Python.

## Ordinary application update, SAME installed layout

```powershell
python tools/flash_usb.py dist/single-http --port COM5 --mode application
python tools/flash_usb.py dist/single-http --port COM5 --mode application --execute
idf.py -B build-single-http -p COM5 monitor
```

Only factory app bytes at 0x20000 are written and read-verified. NVS, profiles,
face/voice models and reserve are excluded. The tool checks the actual installed
table first and refuses a layout mismatch. It refuses encrypted/secure devices;
those require an independently reviewed signed USB update/recovery procedure.
Once the unchanged layout has already been verified, ESP-IDF's direct equivalent is:

```powershell
idf.py -B build-single-http -p COM5 app-flash monitor
```

No automatic fallback image exists. If power loss corrupts the single app, use
P4 ROM download/BOOT-reset and reflash the verified application through USB.
This is a conscious tradeoff for model/profile capacity, not OTA rollback.

## Generic model provisioning/update, SAME layout

```powershell
python tools/flash_usb.py dist/single-http --port COM5 --mode model --execute
```

Writes only human_face_feat at 0x720000, checks pinned model integrity and installed
layout, then read-verifies Flash. Reboot/quiesce inference first (the tool resets
P4 into ROM). Model-version changes require compatible profiles or explicit
re-enrollment; never silently reuse embeddings with a different model namespace.

## Full installation/layout migration

The owner authorized a necessary migration on 2026-10-23. Authorization covers the
single-factory layout and settings relocation, not security provisioning/eFuses.
Use FLASH_MIGRATION.md preflight and a NEW backup path outside the repository:

```powershell
python tools/flash_usb.py dist/single-http --port COM5 --mode full
python tools/flash_usb.py dist/single-http --port COM5 --mode full --execute --authorize-layout-change --backup C:/PRIVATE/HomeBuddy/new-full-flash.bin
idf.py -B build-single-http -p COM5 monitor
```

This mode writes bootloader, NEW table, app and generic MBF. It first queries P4
security, reads a complete 16,777,216-byte private backup, verifies size, records
SHA256 and table integrity, and prepares byte-exact private NVS/PHY relocation
when migrating the known old layout. Unknown nonblank profiles or a different
factory layout block automatic migration. No private files enter the public package.
The old app/model/OTA regions overlap the new allocation, so this is destructive
layout replacement and cannot be performed as app-flash. It never runs erase-flash.

After a successful factory-layout migration, a complete same-layout ESP-IDF flash
can be run as idf.py -B build-single-http -p COM5 flash monitor. It writes only its
listed build artifacts; it does NOT initialize/erase profile or voice partitions.
Prefer application mode for routine updates. NVS contents are preserved; personal
profiles can persist only after separately authorized secure storage provisioning.

## Recovery

Before security provisioning, restore the SAME P4's verified complete private
backup through ROM download using the matching esptool write_flash 0 BACKUP_FILE
command, after explicitly choosing full-image restoration. Restoring only the old
app after a table migration is insufficient. The old bootloader/table/NVS/OTA map
must return together. Verify backup SHA256/length first; later settings/profiles
are lost on full restore. Never write a P4 backup to C6 or another unit.

Flash encryption/Secure Boot may irreversibly restrict USB/debug/download and raw
backup restoration. Neither is enabled by this development package or tool. Test
signed recovery on equivalent hardware before authorizing that separate stage.


## Iteration 8 continuation

Use application-only USB update after scheduling loss of RAM enrollment; no installation was automatic. Current helper rejects secured packages/devices and is not a signed/encrypted updater. Security can disable ROM OTG/DFU; exact Serial/JTAG or USB-UART recovery needs validation per SECURITY_PROVISIONING.md. No C6/OTA/security provisioning added.


## Iteration 10, 2026-10-25

Optional speaker image: prepare_voice_model.py validates pinned raw/PDL3 hashes. package_firmware.py --speaker-model PATH adds one 3s image. flash_usb.py --mode speaker-model dry-runs only 0xc20000 after exact subtype/size/label validation; --execute retains installed-table/security preflight. Application/full plans never automatically write the speaker image. Model/app USB writes restart and lose RAM profiles. See VOICE_RECOGNITION.md; no writes were performed.


## Iteration 11, 2026-10-25

Iteration11 candidate generation: build all four profiles, run effective-config checks, run identity_flash_budget.py --enforce for recognition, then package_firmware.py with --speaker-model local/iteration10/speaker-model/sv_model_3s.espdl. The optional model is verified and remains a separate speaker-model USB mode. App/full plans preserve profiles and do not add the optional SV write. ITERATION11_ACCEPTANCE.md is the one combined physical checklist. Firmware update resets the device and loses RAM enrollment; installation is not authorized by packaging.

## Iteration 12 candidate and single acceptance session

Use the application-only helper for an unchanged installed layout. It writes only
0x20000 application bytes; NVS (including AUTO/privacy acknowledgment), profiles and
face/voice models are preserved. A selected model-only update also excludes profiles;
namespace compatibility checks reject incompatible saved templates without reinterpretation.
Full recovery requires an explicit decision and a new private backup. Never use chip
erase as an ordinary update. No personal database is included in firmware packages.

The first update from an installed RAM-only firmware destroys its current RAM enrollment
when the device restarts. Explain this once before approving installation. No such
installation was executed during this development iteration.

After approved installation: enroll one member, wait for the saved status, enable AUTO,
restart once, confirm the member and AUTO restore, then confirm one greeting and one
eligible HA metadata event. Offline fault injection covers interrupted transactions.

## Explicit interrupted-first-write recovery

Application-only updates intentionally preserve partial profile data too. When
there is no committed generation, firmware fails closed rather than erasing it.
The offline-default recover_profile_storage.py helper requires a private 1 MiB
backup and exact SHA256. Only separately approved discard of headerless upper
bank data is supported, with unchanged-layout/security checks and a byte-for-byte
current-data comparison before erase. It never performs chip erase. Do not run
it merely to clear a warning. See ITERATION12_STORAGE_RECOVERY.md for scope.
