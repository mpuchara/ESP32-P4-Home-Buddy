# Development log

## 2026-10-22: initial foundation

Repository began at 0c361ab with LICENSE only. Work is on
codex/initial-hardware-foundation. HA_UBOX_bridge was read through GitHub only;
no checkout, file mutation or commit was made in that repository.

### Research and implementation

Inspected both requested ultramcu repositories, their source APIs and licenses,
the existing UBox snapshot handler, official ESP-DL speaker example and the
esp_cam_sensor supported sensor list. Selected IDF 5.5.1, LVGL 8.3.11 and exact
managed dependency pins. Created architecture, partition/PSRAM budget, roadmap,
hardware/API/privacy/build/recovery documentation, CI and draft-release workflows.

Implemented serial diagnostics, non-destructive NVS init, an experimental DSI/PPA
LVGL port, GT911 inverse coordinate mapping, Polish font, touch-open menu, manual
brightness and inactivity return, plus optional version-gated C6 scan diagnostics.
The menu explicitly labels unfinished product features. No fabricated recognition,
time, weather, network connection or camera image is presented.

### Confirmed physical evidence

* esptool 4.12.0, COM5: ESP32-P4 rev. 1.0, 40 MHz crystal, 16 MB flash.
* Full original P4 flash backup completed, 16,777,216 bytes, SHA256
  d951fe69eeb2e297c75e76ad970bf5caeafd4b7a71ab88cdf6167f1bd6c47c15.
  Private backup remains in ignored local/original-p4-flash.bin; never upload it.
* Initial diagnostics image compiled, flashed and hash-verified. Serial boot:
  IDF v5.5.1; 32 MB PSRAM detected; SPI SRAM memory test OK; app_main reached.
* Initial default image initialized ESP-Hosted bookkeeping via its constructor.
  Final configuration disables the component in diagnostics/display profiles;
  C6 is enabled only in the explicit Wi-Fi profile. No C6 firmware was flashed.

### Verification completed so far

* CMake/MSVC host test: cover/crop aspect ratios, overflow, coverage over many
  dimensions, invalid inputs and touch corners passed (CTest 1/1).
* Python configuration suite: dependency lock parity, flash partitions and all
  Polish glyphs passed (3/3).
* clang-format 19.1.7 application checks and actionlint 1.7.12 passed.
* Diagnostic and first experimental display firmware builds passed. Final
  display profile with ESP-Hosted disabled also compiled and flashed successfully
  (0x8b070-byte app). Effective configuration checker passed.
* Display-profile boot: DSI panel initialized, two 480×800 native buffers allocated,
  PPA rotation registered, GPIO23 LEDC enabled, GT911 chip ID 911 read at 0x5D,
  and LVGL 800×480 task started. Initial free memory: internal 429123 bytes,
  PSRAM 31245164 bytes. Twelve-second capture contains no reset or display error.
  This is initialization evidence; visual/touch acceptance remains separate.
* GT911 driver warns "Unable to initialize the I2C address" with no INT line,
  then reads chip ID/configuration successfully. Functional touch remains to test.
* Local Git push had no noninteractive credentials. Published the same file tree
  through the authorized GitHub connector, then reconciled the local branch to it;
  no credentials were extracted. Draft PR: https://github.com/mpuchara/ESP32-P4-Home-Buddy/pull/1.
* Wi-Fi profile built locally (0xf1330 bytes), effective GPIO/profile check passed,
  and all three profiles packaged with public-only outputs and license notices.
* Physical C6 diagnostic: slot1 / 4-bit / 40 MHz SDIO initialized, ESP32-C6 and WLAN
  identified. Version mismatch reported; firmware-version RPC timed out and the
  app reported ESP_FAIL without rebooting the UI during the 30 s capture. Installed
  version remains UNKNOWN (do not interpret the transport's 0.0.0 as a real version).
  No scans/association or C6 writes occurred. Display-only P4 image restored.
* Initial GitHub run compiled all three images and passed host checks, then failed
  in post-build Git repository discovery. Replaced that lock guard with a direct
  before/after byte comparison and explicit workspace paths. Full rerun passed:
  https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39752014808
  (host checks and all three firmware build/configuration/package jobs).
  Verified implementation commit: 1b967073fceb8db5b143348d839a81fac8e5c84d.
* Final documentation-only evidence update uses [skip ci]; firmware/workflows are
  unchanged from that successful run. No production release or merge was made.

### Acceptance still open

Visible display, four-corner touch, backlight behavior, cold-boot/repeated-menu
stability remain pending physical observations. C6 transport is confirmed, but
firmware identification/compatibility must be resolved before scan/association.
Wi-Fi provisioning/association/DHCP are not implemented yet. Camera, microphone,
face/speaker inference, profiles/PIN, UBox transport/JPEG display, NTP, temperature,
MQTT, automatic brightness and OTA are not implemented.
72-hour stability: NOT VERIFIED ON HARDWARE. This is not a completed product.

Next gate: visible 800×480 LVGL and peripheral verification, then fullscreen UBox
snapshot transport and rendering. Follow docs/DEVELOPMENT_PLAN.md.
