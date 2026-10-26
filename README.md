# ESP32-P4 Home Buddy

Local smart-home assistant for GUITION JC4880P443C_I_W_Y (SKU 10153003).
Apache-2.0 application with an attributed MIT board-support component.

**Development firmware 0.11.4-dev, not a finished product.** Implements a fullscreen
cached UBox JPEG pipeline, hardware JPEG decoding and cover cropping, Warsaw clock,
Open-Meteo temperature, queued greeting overlays and direct Wi-Fi/UBox/weather settings without administrator PIN.
Adds local OV02C10 capture/diagnostic preview and local brightness calibration with AUTO, MANUAL and temporary override. Adds an explicit local face-detection diagnostic with bounded boxes and pinned ESP-DL weights. Adds diagnostic temporal presence with distinct-frame confirmation, delayed absence and unavailable-data handling. Adds an explicit bounded ten-minute presence test on the UBox screensaver. Physical acceptance remains incomplete. See [actual results](docs/DEVELOPMENT_LOG.md).

Candidate additions: local face enrollment/AUTO and HTTP identity events, a four-section
Polish product menu, bounded microphone diagnostics and a one-shot SV 3s embedding
prototype. Voice accuracy and microphone acceptance remain pending. Iteration 12
activates the existing unified schema-2 database with automatic local unencrypted
persistence, automatic restoration and retained AUTO preference. No password, PIN
or unlock. Physical extraction is not prevented. On 2026-10-26 the owner confirmed
the household member and enabled recognition setting survived a restart on
installed 0.11.2-dev after approved storage recovery. Greeting/HA delivery and
physical resource peaks remain separately unconfirmed. See [current report](docs/ITERATION13_REPORT.md) and
[owner storage decision](docs/PERSISTENCE_DECISION.md).

## 0.11.4-dev profile-toggle follow-up

The operator's disable/enable test on installed 0.11.3-dev saved both changes
(generation 4), but exposed Flash-associated display flicker and a fragmented-heap
MBF reload failure. The follow-up parks the already loaded model set only during
an AUTO member pause, preserves explicit Stop/cancel release and all admission
guards, and enables verified DSI/DW-GDMA IRAM-safe interrupt handling. Exact-path
physical retest is pending. See [iteration 13](docs/ITERATION13_REPORT.md).

## 0.11.3-dev persistent identity stabilization

Blank storage receives a verified empty generation before enrollment. First user
save retains that fallback; interrupted/unknown data still blocks writes without
formatting. Flash operations are bounded to 4 KiB and yield between calls. Recovery
keeps the app stopped in the loader throughout comparison, scoped erase and readback.
Combined voice readiness reports PCM, 512D output, timing and cleanup numerically.
This candidate is host-tested, built and installed under owner approval, with
application/model readback and preservation of profiles/other sectors verified.
Actual boot restores the existing generation and resumes ready AUTO; changed-save,
greeting/HA and audio/model execution acceptance still need operator interaction. See [iteration 13](docs/ITERATION13_REPORT.md).

## 0.11.2-dev interrupted first write and diagnostics

Physical enrollment now reaches completion after the stack fix. A private read-only
Flash audit found a blank lower bank and headerless partial data in the upper bank,
consistent with the earlier crash during the first create/save. Approved recovery
erased only the incomplete upper bank, with readback verification. The owner then
confirmed profile retention and the enabled recognition setting after a restart;
the firmware never automatically formats interrupted/unknown storage.
The candidate fixes the 11-entry advanced menu, exposes storage result codes and
delays the create navigation receipt until its save attempt completes. See
[recovery scope and evidence](docs/ITERATION12_STORAGE_RECOVERY.md).

## 0.11.1-dev enrollment stack fix

The first installed 0.11.0-dev candidate exposed an LVGL stack protection fault
when moving from Add household member into guided enrollment. 0.11.1-dev uses
three bounded UI-owned status buffers instead of nested automatic snapshots and
checks actual P4 compiler stack frames on every firmware build. Physical
registration completed without the original reboot; profile retention and the
enabled recognition setting after restart were confirmed on 0.11.2-dev.
See [captured fault and fix](docs/ITERATION12_STACK_FIX.md).

## Hardware and status

USB interrogation on 2026-10-22 identified ESP32-P4 **rev. 1.0**, **16 MB flash**;
boot diagnostics confirmed **32 MB PSRAM** and a passing memory test.
The upstream BSP targets JC4880P443C_I_W; compatibility with this exact _Y unit
must be established per peripheral. Camera ID is physically confirmed as OV02C10 (0x5602 at SCCB0x36). Audio routing, C6 firmware
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
USB is the supported update path; this architecture has no OTA.

## Current interface

Tap the saver to open Wi-Fi, UBox, weather, brightness and diagnostic controls.
Missing image/time/temperature remain unavailable. Test JPEG uses a committed
synthetic image; Test greeting explicitly says Test UI. Experimental biometric inference runs only after explicit opt-in in the recognition build.
All settings open directly, without an administrator PIN, at the operator's
explicit request. Choose a Wi-Fi network and enter its password; UBox settings and weather location are edited in their own screens. Wi-Fi
passwords stay masked; the UBox token is visible in its edit field as requested.
Secrets remain excluded from general diagnostics. **NVS is not encrypted at rest in this development
build.** Local touchscreen access allows changing settings. No default PIN or
hidden PIN gate remains.
Thirty idle seconds return to the saver; critical saves are exempt.

Wi-Fi scanning works on the connected C6 despite unavailable metadata RPC.
Association/DHCP, correct local clock, UBox images/conditional304 and device
Open-Meteo search/temperature are operator-verified.
Reconnect/fault/soak acceptance remains incomplete. No C6 firmware update was performed.
Use the wifi profile for connectivity; the bringup profile remains offline.
See [user guide](docs/USER_GUIDE.md), [UBox API](docs/UBOX_API.md) and
[Open-Meteo setup](docs/WEATHER_API.md).

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
RGB565 cover, Open-Meteo geocoding/current-data parsing, freshness and time. Linux CI runs DST
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

* [Local camera and face AI foundation](docs/LOCAL_CAMERA.md)
* [Diagnostic presence policy](docs/PRESENCE.md)
* [Face-image quality metrics](docs/FACE_QUALITY.md)
* [Brightness calibration](docs/BRIGHTNESS.md)
* [Architecture](docs/ARCHITECTURE.md)
* [Hardware and pinout](docs/HARDWARE.md)
* [Dependencies and licenses](docs/DEPENDENCIES.md)
* [Flash/PSRAM budget](docs/MEMORY_BUDGET.md)
* [Roadmap](docs/DEVELOPMENT_PLAN.md)
* [UBox HTTP contract](docs/UBOX_API.md)
* [Home Assistant Webhook / REST](docs/HOME_ASSISTANT_HTTP.md)
* [Privacy](docs/PRIVACY.md)
* [User guide](docs/USER_GUIDE.md)
* [Troubleshooting](docs/TROUBLESHOOTING.md)

The independent `mpuchara/HA_UBOX_bridge` repository is read-only reference material.
Home Buddy will consume its published API only.

## Experimental household identity

On-device MBF execution, one-person RAM enrollment/fresh-frame confirmation,
personalized saver greeting and a real confirmed recognition event received by
Home Assistant over REST are operator-accepted. A matching live HA trace and
operator receipt also accept the recognized-only notification action. Five saver/re-entry cycles pass
after the Flash-validation watchdog fix. Secure persistence, quantified accuracy,
extended reliability acceptance remain open. A controlled 90-second stay and
leave/return test produced one notification per encounter with matching HA traces.
See [hardware evidence](docs/IDENTITY_VALIDATION.md).

[Implementation and on-device enrollment](docs/FACE_RECOGNITION.md) uses
ESPDet416, real MNP landmarks, native ROI, MBF embeddings and multi-frame open-set
matching. The 0.6.1-dev candidate uses one 7 MiB factory application, dedicated
onboard MBF storage, a 1 MiB gated profile area and 3 MiB reserved for future voice.
The iteration-12 recognition candidate selects LOCAL_UNENCRYPTED. HA uses native Webhook or REST
events without a broker. No microSD, external inference or OTA.
See [current report](docs/ITERATION7_REPORT.md), [Flash map](docs/SINGLE_IMAGE_FLASH_LAYOUT.md),
[USB updates/recovery](docs/USB_FLASHING.md), [migration](docs/FLASH_MIGRATION.md)
and [HA setup](docs/HOME_ASSISTANT_HTTP.md). Physical acceptance is tracked separately.


## Iteration 8 continuation

Candidate 0.7.0-dev adds opt-in AUTO with ordinary NVS preference, preserved ten-minute TEST, RAM-only reboot waiting, fresh local evaluation and manual 60-second microphone diagnostics. Installed firmware remains unchanged. See docs/ITERATION8_REPORT.md and docs/ITERATION8_ACCEPTANCE.md.


## Iteration 10, 2026-10-25

0.9.0-dev adds an explicit one-shot speaker-embedding diagnostic under advanced microphone diagnostics, a bounded preliminary activity gate, checked Flash-backed SV adapter, optional USB model package and synthetic voice/fusion safety tests. Device microphone/model/accuracy acceptance remains pending; real voice enrollment and voice HA events stay unavailable. See docs/VOICE_RECOGNITION.md and docs/ITERATION10_REPORT.md.
