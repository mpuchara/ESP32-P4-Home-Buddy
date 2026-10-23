# Offline Flash migration and recovery: explicit authorization required

This is a reviewable development procedure, not an executed migration. Never run
erase_flash or idf.py flash on the currently installed map as an ordinary update.
No C6 firmware or HA_UBOX_bridge changes are involved.

## Read-only preparation completed on 2026-10-23

The P4 ROM security query reports Secure Boot disabled, Flash encryption disabled,
SPI_BOOT_CRYPT_CNT=0. A private full 16,777,216-byte backup was read and verified
with SHA256. It is outside the repository and must never be uploaded to CI/GitHub.
The partition-table MD5 is valid; its entries match partitions.csv. OTA bank 0 has
sequence 1/state VALID and a valid CRC; bank 1 is blank. Boot selection is ota_0,
whose application description is 0.4.5-dev. The bootloader header is present at
0x2000. Passive serial observation after the read shows the existing camera/UI
firmware running. No new firmware, partition table or eFuse was written.

The former profiles/models areas are fully erased (0xff) in this backup, so
there is no stored biometric database or legacy model to relocate.

Backup and private receipt: C:\Users\mpuch\.codex\private\home-buddy-iteration6-20261023.
Re-read Flash/security immediately before an authorized migration if settings have
changed. Keep a second private durable copy and verify its hash. Stop settings
changes while taking the final backup.

## Concrete candidate and consequences

See partitions.identity.csv and IDENTITY_FLASH_LAYOUT.md. The partition table moves from 0x8000 to 0x10000 to accommodate the measured
security bootloader. NVS moves 0x9000 -> 0x11000, OTA metadata 0xf000 -> 0x17000,
PHY 0x11000 -> 0x19000; an encrypted 4 KiB NVS-key reserve is at 0x1a000.
The ota_0 start remains at 0x20000. Both app slots shrink to
5 MiB. ota_1 moves from 0x620000 to 0x520000; the model range 0xa20000..0xe20000
overlaps the former ota_1 and profiles regions. New profiles occupy
0xe20000..0xea0000, within the former models area. Therefore the old ota_1,
profiles and models contents cannot be presumed usable after changing the table.
Current household profiles are RAM-only; do not claim their recovery from Flash.
NVS Wi-Fi/UBox/weather and brightness settings require byte-for-byte relocation
from the private backup BEFORE activation. tools/prepare_private_migration.py
prepares only private NVS/PHY images and hashes outside the repository, never
writes a device. Verify restored values after installation; never print them.

The prepared recognition package contains firmware, bootloader, partition table,
initial OTA metadata, the validated generic PDL3 model, licenses and SHA256SUMS.
It contains no credentials, biometric templates, Flash backups or encryption keys.
Its flash_args describe a full initial installation and carry a migration warning.
The development package does NOT enable Flash encryption or Secure Boot.

After explicit migration authorization, with physical access and reliable power:

1. Reconfirm security state, active slot, backup hash/length and bootloader image
   compatibility. Check every package hash and build size guard. Keep ROM download
   access available. Authorize P4 migration independently of security provisioning.
2. Relocate the private NVS image to 0x11000 and PHY to 0x19000. Write the
   compatible bootloader at 0x2000/table at 0x10000, initialize NEW OTA metadata
   at 0x17000 for ota_0, write the app at 0x20000 and verified model at 0xa20000. Do not
   erase the whole device. The old ota_1 cannot serve as rollback after relocation;
   full-backup ROM recovery is the recovery path until a compatible second slot is
   installed. Initialize the new profile area only through an explicitly reviewed
   provisioning/migration action, never a format-on-error path.
3. Cold boot and verify Wi-Fi, UBox, Open-Meteo, touch, brightness, clock/temperature
   positions and the fullscreen dashboard. A missing/corrupt model must report a
   diagnostic while these remain operational.
4. Load MBF and perform the local recognition/combined JPEG tests. Capture model
   load time, allocation deltas, largest blocks and stage timings. No accuracy or
   memory acceptance can be inferred from a successful build.
5. Install a compatible application into the NEW ota_1 using an OTA-aware updater
   (esp_ota APIs), not a raw app write that ignores metadata. Exercise both slots
   and an unconfirmed/failed image rollback. The application cancels pending
   rollback only after 60 seconds with UI/config/camera and model-integrity checks.
   This is a boot-health gate, not recognition-accuracy certification.
6. Reject the migration if settings cannot be preserved/recovered, either slot
   fails, or resource/recognition checks fail. Restore the entire verified backup
   on the SAME unencrypted device through P4 ROM download; restoring the app alone
   does not restore the old partition map. Data changed since backup is lost.

No executable destructive command is automated here. Migration and rollback have
not been physically tested. Ordinary app OTA later preserves model/profile ranges,
but the model is shared by both app slots: incompatible model updates require an
offline coordinated update or a future A/B-model scheme. Do not overwrite a live
model during inference or assume app rollback restores model weights.

## Separate security provisioning, not authorized or executed

Use the exact ESP-IDF 5.5.1 P4 security documentation and the actual board's ROM
recovery capabilities. Prepare a signed bootloader/app build and measure bootloader
growth before approving its layout. The initial Flash-encryption-only build measured 0x85a0 and FAILED the
old 0x8000 table boundary. The effective Secure Boot v2 + Flash encryption
assessment measured 0xb000 (45,056 bytes) and PASSED at 0x10000 with 12 KiB spare.
Signing was disabled, no key was generated, and no provisioning was performed. The candidate therefore explicitly uses 0x10000 and
relocates NVS/OTA/PHY; its unsigned bootloader size check is tested separately.
Signed/encryption-release builds must still be measured and recovered on hardware
before security authorization. No security-assessment binary is included in the
development installation package.

Provision a unique hardware Flash encryption key and protected Secure Boot signing
trust through a controlled offline process. Never hardcode a key or commit/export
private signing keys to firmware packages. Protect signing-key escrow and document
who can authorize signed recovery. Release-mode encryption and debug/download
restrictions are irreversible and may eliminate raw-backup restoration. A plaintext
backup taken before provisioning cannot simply be written over a secured device.
Test signed recovery on a sacrificial equivalent device before provisioning this one.

Only after explicit security authorization, verified signed recovery, encryption
and Secure Boot state, and an initialized encrypted profiles partition, build with
HB_IDENTITY_PERSISTENCE enabled. The adapter checks hardware state AND the
partition encrypted flag at runtime. It never generates keys or burns eFuses.
For NVS credentials use separately provisioned NVS encryption and protected key
storage; marking an NVS partition encrypted is not sufficient. This development
configuration still stores ordinary settings/MQTT credentials in plaintext NVS.

Then perform the full consented enroll -> save -> power cycle -> recognize ->
rename -> power cycle -> confirm stable ID/name -> delete -> power cycle sequence,
plus interrupted-write and recovery tests. Until then secure persistence acceptance
is explicitly incomplete. Hardware encryption gives confidentiality; SHA256 in the
profile format detects corruption, not authenticated storage or anti-rollback.
Replay-resistant/authenticated profiles would require a separate protected-key and
generation-counter design if that threat is in scope.

Sources: [P4 Flash encryption, IDF 5.5.1](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32p4/security/flash-encryption.html),
[Secure Boot v2](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32p4/security/secure-boot-v2.html),
[NVS encryption](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32p4/api-reference/storage/nvs_encryption.html).
