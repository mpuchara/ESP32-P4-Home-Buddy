# ESP32-C6 compatibility and recovery

P4 contains no Wi-Fi radio. The optional diagnostic uses ESP-Hosted SDIO on
slot1, pins in HARDWARE.md. Host version is pinned to 2.12.2.
The installed C6 firmware version remains UNKNOWN: metadata RPC times out. The
pinned host documentation lists legacy Wi-Fi RPCs before the firmware-version RPC.
A controlled P4-only change allowed station start and scan despite unavailable
metadata; this succeeded on the connected unit. No exact-version compatibility
claim is made. Association, DHCP and reconnect remain separate acceptance gates.

No Home Buddy code invokes coprocessor OTA. Scanning is explicitly enabled.
A mismatch or missing transport should be reported and investigated; do not solve
it by automatically downloading/flashing another C6 image.

## Initial result on the connected unit, 2026-10-22

The Wi-Fi profile built and was temporarily flashed to P4. SDIO slot1 at 40 MHz
initialized and detected ESP32-C6 with WLAN capability. The transport emitted
"Host [2.12.0] > Co-proc [0.0.0]" and the GetCoprocessorFwVersion RPC timed out.
The package is 2.12.2; the diagnostic transport string is reproduced verbatim.
**0.0.0 is not a confirmed installed firmware version.** The version field/query
is unavailable under this pairing. Scanning/association were not attempted.
The display-only P4 image was restored after this test. C6 flash was untouched.

## Iteration-2 P4-only result

The metadata-gated failure above was not proof that legacy Wi-Fi RPCs were broken.
With the existing untouched C6 and host 2.12.2, P4 station start and scan return
ESP_OK (probe found 2 networks; 0.2.0-dev boot found 8 records). SSIDs are omitted
from logs. Metadata remains UNKNOWN; do not interpret 0.0.0 as an installed version.
No C6 write/erase/OTA took place. Current P4 uses the wifi profile. A C6 update is
not needed merely to fix unavailable metadata; live connection tests come next.

## Recovery-safe update planning

Before any separate C6 update: record the installed version, back up the entire
C6 flash over its direct UART, verify its actual partition table and save a hash.
Use the board schematic to identify C6 UART/boot/reset, a 3.3 V adapter and common
ground. Halt P4 so RESET54 cannot interrupt C6 programming. Use a known compatible
official image and verify its hash, chip, partition offset and recovery path.
Do not assume the upstream app offset 0x10000 and otadata 0xd000 apply to this unit.
Only after those checks perform an explicitly chosen update, verify it by readback
and handshake, then test Wi-Fi reconnect/uptime. Keep the backup for direct UART
restore. This project currently provides no automatic C6 update procedure.

### Direct-UART command sequence

These are operator steps for a verified connector/partition layout, not an
automatic updater. Replace COM_C6 and the uppercase placeholders with measured
values; do not paste them unchanged. Keep the backup private.

1. Close monitors. Park P4 in ROM so its app cannot pulse C6 reset mid-operation:
   `python -m esptool --chip esp32p4 --port COM5 --before usb_reset --after no_reset flash_id`.
2. Verify the board's labeled C6 UART/BOOT/reset connections against its schematic.
   Enter C6 ROM download mode using those lines. Query it through the separate
   adapter: `python -m esptool --chip esp32c6 --port COM_C6 --after no_reset flash_id`.
3. Read the **entire detected C6 flash size** before any write:
   `python -m esptool --chip esp32c6 --port COM_C6 --after no_reset read_flash 0 DETECTED_C6_FLASH_BYTES local/original-c6-flash.bin`.
   Verify length, save a SHA256 and keep a second copy. Decode the backed-up
   partition table offline using its verified location; record boot/OTA selection.
4. Inspect the selected official image with `python -m esptool image_info IMAGE.bin`.
   Verify chip, version, hash, image type, SDIO configuration, flash size and exact
   destination. An app image and a merged full-flash image require different offsets.
5. Only after those checks, write and verify the **chosen measured offset**:
   `python -m esptool --chip esp32c6 --port COM_C6 --after no_reset write_flash VERIFIED_IMAGE_OFFSET IMAGE.bin`,
   then `python -m esptool --chip esp32c6 --port COM_C6 --after no_reset verify_flash VERIFIED_IMAGE_OFFSET IMAGE.bin`.
   If OTA selection needs changing, use the verified firmware's documented method;
   do not blindly erase the upstream example's 0xd000 region.
6. Reset both devices, flash/select the P4 Wi-Fi profile and capture handshake,
   reported firmware version, scan, association and reconnect results.
7. If recovery is needed, halt P4 again, enter C6 ROM and restore the verified
   same-unit full backup at address zero:
   `python -m esptool --chip esp32c6 --port COM_C6 write_flash 0 local/original-c6-flash.bin`.

No C6 image, destination offset or update has been approved/tested for this unit
yet. Do not treat these preparation steps as proof of a working update.
