# Single-image migration and security boundary

Current map and exact offsets: [SINGLE_IMAGE_FLASH_LAYOUT.md](SINGLE_IMAGE_FLASH_LAYOUT.md).
Commands, verified package modes and ROM recovery: [USB_FLASHING.md](USB_FLASHING.md).

The owner authorized the necessary partition migration in this development
session. This does not authorize security provisioning or eFuse changes. A normal
application-only update must refuse a different installed partition table.

## Known original device

The earlier verified private 16 MiB backup contained 0.4.5-dev: table 0x8000,
NVS 0x9000/0x6000, OTA metadata 0xf000/0x2000, PHY 0x11000/0x1000,
OTA0 0x20000/6 MiB, OTA1 0x620000/6 MiB, blank profiles 0xc20000/2 MiB,
blank models 0xe20000/0x1e0000. Secure Boot and Flash encryption were disabled.
This is historical evidence; full installation always queries the current device
and obtains a NEW private full backup before writing.

## Executable preflight

The USB tool verifies package checksums, a single 7 MiB factory app, growth margin,
model integrity and explicit P4 port. It refuses secured/unknown devices. Full mode
requires `--authorize-layout-change` and a new backup outside the repository. It
records security status, full backup length and SHA256, and validates the installed
table MD5. A known old dual layout with blank profiles can relocate private NVS to
0x11000 and PHY to 0x19000 from that backup. Wi-Fi/UBox/weather settings stay private.
Existing nonblank profiles or an unknown different factory layout block migration
for a reviewed logical/security migration. Old OTA1 may be overwritten by the new
MBF model: it is not a rollback bank after migration.

Writes are limited to bootloader 0x2000, table 0x10000, factory 0x20000, MBF 0x720000
and relocated settings if necessary. Every written range is verified. No chip-wide
erase, C6 operation, profile initialization or voice provisioning is performed.
If flashing fails, retain the private backup and use the P4 ROM procedure; do not
try to boot an app against an incompatible table. Restoring an entire original
backup is a separate destructive recovery operation on that same unsecured device.

## Secure persistence is a separate milestone

The profile gate still requires opt-in, hardware Flash encryption, Secure Boot and
the encrypted profiles partition. This board therefore remains visibly RAM-only
until separately authorized secure provisioning. No plaintext biometric fallback.
The offline unsigned security assessment measured a 45,056-byte bootloader with
effective Secure Boot v2 + Flash encryption, fitting before table 0x10000 with
12 KiB headroom. That image is not installed and is not a provisioning package.
Signed-image size/alignment, key custody, recoverable USB download policy,
encrypted NVS credentials, initial encrypted profile erasure, secure app updates,
power-loss/delete acceptance and operator recovery must be designed and tested
before provisioning. Do not hardcode keys or burn irreversible eFuses implicitly.
