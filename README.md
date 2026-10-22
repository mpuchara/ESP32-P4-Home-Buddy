# ESP32-P4 Home Buddy

Local smart-home assistant for GUITION JC4880P443C_I_W_Y (SKU 10153003).
Apache-2.0 application with an attributed MIT board-support component.

**Development firmware 0.2.0-dev, not a finished product.** Implements a fullscreen
cached UBox JPEG pipeline, hardware JPEG decoding and cover cropping, Warsaw clock,
MQTT temperature, queued greeting overlays and direct Wi-Fi setup and PIN-protected UBox/MQTT settings.
Physical acceptance remains incomplete. See [actual results](docs/DEVELOPMENT_LOG.md).

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

Tap the saver to open Wi-Fi, UBox, MQTT, brightness and diagnostic controls.
Missing image/time/temperature remain unavailable. Test JPEG uses a committed
synthetic image; Test greeting explicitly says Test UI. No biometric inference runs.
Wi-Fi opens directly: choose a network and enter its password, as requested by
the operator. UBox/MQTT edits require a self-chosen 6-12 digit PIN. Passwords/tokens are
masked, saved in NVS, and excluded from diagnostics. **NVS is not encrypted at
rest in this development build**; PIN controls touchscreen access, not physical
flash extraction. Never provision production secrets on an untrusted unit.
Thirty idle seconds return to the saver; critical saves are exempt.

Wi-Fi scanning works on the connected C6 despite unavailable metadata RPC.
Association/DHCP/reconnect, live UBox and MQTT need operator configuration and
physical validation. No C6 firmware update was performed.
Use the wifi profile for connectivity; the bringup profile remains offline.
See [user guide](docs/USER_GUIDE.md), [UBox API](docs/UBOX_API.md) and
[MQTT temperature example](docs/MQTT_API.md).

## Tests

```sh
cmake -S tests -B build-host
cmake --build build-host
ctest --test-dir build-host --output-on-failure
python -m pip install -r tools/requirements-dev.txt
python tools/check_project.py
```

C tests execute the actual cover/crop and touch-coordinate code on the host,
plus the committed font through the real LVGL decoder with font compression off.
Dashboard tests exercise transfer bounds, ETag commit/rollback, JPEG header checks,
RGB565 cover, strict temperature parsing, freshness and time. Linux CI runs DST
boundaries; Windows CRT does not support that POSIX timezone rule.
Supply CJSON_SOURCE_DIR pointing to cJSON 1.7.18 (IDF components/json/cJSON).
Resolve firmware dependencies first, or supply LVGL_SOURCE_DIR to the test build.
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
