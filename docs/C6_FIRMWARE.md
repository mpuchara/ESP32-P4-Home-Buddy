# ESP32-C6 compatibility and recovery

P4 contains no Wi-Fi radio. The optional diagnostic uses ESP-Hosted SDIO on
slot1, pins in HARDWARE.md. Host version is pinned to 2.12.2.
The installed C6 firmware version must be read from the actual transport handshake.
The upstream report of factory 2.3.0 and success with 2.12.9 is not a measurement
of this board and is not a guarantee that any 2.12 release pair is compatible.

No Home Buddy code invokes coprocessor OTA. Scanning is explicitly enabled.
A mismatch or missing transport should be reported and investigated; do not solve
it by automatically downloading/flashing another C6 image.

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
