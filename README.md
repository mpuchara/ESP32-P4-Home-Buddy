# ESP32-P4 Home Buddy

Local smart-home assistant for GUITION JC4880P443C_I_W_Y (SKU 10153003).
Apache-2.0 application with an attributed MIT board-support component.

**Development firmware, not a finished product.** The current iteration establishes
ESP-IDF build configuration, serial diagnostics, an experimental ST7701S/PPA
landscape LVGL port, GT911 touch, manual backlight control and optional C6
Wi-Fi scanning. See [development log](docs/DEVELOPMENT_LOG.md) for actual results.

The intended product combines a fullscreen cached UBox snapshot, clock, outdoor
temperature, touchscreen configuration, local face and speaker identification,
local brightness control, and MQTT Home Assistant events. Those features are
roadmap items until explicitly recorded as tested.

## Hardware and status

USB interrogation on 2026-10-22 identified ESP32-P4 **rev. 1.0**, **16 MB flash**;
boot diagnostics confirmed **32 MB PSRAM** and a passing memory test.
The upstream BSP targets JC4880P443C_I_W; compatibility with this exact _Y unit
must be established per peripheral. Camera model, audio routing, C6 firmware
and physical display behavior require separate evidence.
Never use upstream hardware test claims as results for this unit.

## Build

Use **ESP-IDF v5.5.1** (commit fcae32885b0296b32044cb99ecbdc50d98dddb83),
Python supported by that IDF, and its supplied RISC-V toolchain. Arduino is not used.
The existing ESP-IDF 5.1 installation cannot build this P4 project.
[Detailed Windows instructions](docs/BUILD.md).

From an activated ESP-IDF terminal:

```powershell
$env:PYTHONIOENCODING = "utf-8"
idf.py set-target esp32p4
idf.py build
```

The default build logs diagnostics without driving unconfirmed peripheral pins.
To build the experimental display profile into a separate directory:

```powershell
idf.py -B build-bringup -D SDKCONFIG=build-bringup/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;config/sdkconfig.bringup" build
```

Use `config/sdkconfig.wifi` instead for the opt-in C6 diagnostics profile.
Do not update C6 firmware automatically. Inspect [C6 compatibility](docs/C6_FIRMWARE.md).

## Install and recover

Back up the original P4 flash before first installation. Backups can contain
private configuration: keep them under ignored `local/` and outside public artifacts.
After checking the hardware profile, use the matching build directory:

```powershell
idf.py -B build-bringup -p COM5 flash monitor
```

COM5 is this session's port, not a fixed requirement. See [recovery](docs/BUILD.md).
No browser installer or OTA is provided yet.

## Current interface

The bring-up screen shows unavailable snapshot/time/temperature, never fabricated
values. Touch opens a Polish diagnostics menu with manual brightness and an
optional Wi-Fi scan request. Thirty seconds without touch returns to the saver.
Unimplemented household/recognition/HA/UBox sections are explicitly labeled.
There is no first-run wizard, persistent Wi-Fi provisioning or administrator PIN yet;
therefore enrollment, profile deletion and credential editing are not exposed.

## Tests

```sh
cmake -S tests -B build-host
cmake --build build-host
ctest --test-dir build-host --output-on-failure
python -m pip install -r tools/requirements-dev.txt
python tools/check_project.py
```

C tests execute the actual cover/crop and touch-coordinate code on the host.
CI builds diagnostic, display and Wi-Fi profiles, validates partition/dependency
configuration, runs host tests and checks whitespace. Manual release workflow
creates draft prereleases only after CI. Hardware validation remains separate.

## Architecture and integration

One task owns LVGL; network operations use a bounded worker queue.
PPA rotates 800×480 logical RGB565 to the 480×800 native DSI framebuffers.
The future identity pipeline runs entirely on P4; HA provides no inference.

* [Architecture](docs/ARCHITECTURE.md)
* [Hardware and pinout](docs/HARDWARE.md)
* [Dependencies and licenses](docs/DEPENDENCIES.md)
* [Flash/PSRAM budget](docs/MEMORY_BUDGET.md)
* [Roadmap](docs/DEVELOPMENT_PLAN.md)
* [UBox HTTP contract](docs/UBOX_API.md)
* [Planned MQTT API](docs/MQTT_API.md)
* [Privacy](docs/PRIVACY.md)
* [User guide](docs/USER_GUIDE.md)
* [Troubleshooting](docs/TROUBLESHOOTING.md)

The independent `mpuchara/HA_UBOX_bridge` repository is read-only reference material.
Home Buddy will consume its published API only.
