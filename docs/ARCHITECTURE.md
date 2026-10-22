# Architecture, iteration 2

The default profile keeps outputs disabled; bringup enables board/UI and wifi also
enables the ESP-Hosted station worker. Dependencies remain locked to IDF 5.5.1,
LVGL 8.3.11 and the existing BSP. No biometric or camera/audio capture runs.

* board: native DSI 480x800 RGB565 ping-pong and PPA rotation 270 degrees from
  logical 800x480. GT911 mapping remains x=physical_y, y=479-physical_x.
* ui: one LVGL owner, 2 ms ticks, handler sleeps at most 20 ms. A four-entry
  command queue transfers completed image ownership and copies status/temperature/
  greeting data. Configuration has a separate two-entry work queue. Network,
  PBKDF2, NVS and JPEG decoding never run in LVGL callbacks.
* configuration: validated schema-1 snapshots, mutex-protected reads and a single
  NVS blob transaction for settings plus salted PIN hash. No automatic erase on
  corruption. Wi-Fi setup is explicitly exempt from PIN per operator request and can only
  update SSID/password; UBox/MQTT and the existing PIN remain protected. A Wi-Fi
  save before PIN setup leaves the PIN unset, including after reboot.
  No default PIN. PBKDF2-HMAC-SHA256, random 16-byte salt, 20,000 rounds;
  constant-time digest comparison. Five wrong attempts cause a 60-second RAM-only
  lockout. Unlock lasts 120 seconds and save consumes it. Reboot resets lockout.
  NVS credentials are plaintext at rest: production encryption remains incomplete.
* connectivity: ESP-Hosted 2.12.2 SDIO, legacy station APIs, bounded scan and
  disconnect requests, eight-result SSID list, DHCP events/RSSI and retries at
  5..30 seconds. Metadata RPC failure reports UNKNOWN rather than inventing a C6
  version. Only scan is physically verified so far. Wi-Fi storage on C6 is RAM.
* ubox_client: one task requests only original cached /snapshot.jpg. Two MiB
  staging, 8-second HTTP timeout, no redirects, exponential 5..300-second backoff.
  ETag advances only after decoder success and UI queue acceptance; 304 preserves
  the image and accepted-image age. No capture, PTZ or private JPEG persistence.
* screensaver: baseline 8-bit JPEG only, source <=2048x2048, <=4 MP. IDF hardware
  JPEG engine uses aligned input/output; raw RGB565 is padded to 16-pixel MCU
  dimensions. CPU nearest-neighbor cover scale creates a separate 800x480 frame.
  It preserves aspect ratio, crops centrally and never stretches to force fit.
* services: SNTP with explicit sync-valid state; POSIX TZ
  CET-1CEST,M3.5.0,M10.5.0/3. Clock label changes only on a changed minute. MQTT
  reconnects independently, verifies TLS using the IDF CA bundle, assembles bounded
  fragments and validates timestamped Celsius data. No discovery or identity
  events. Temperature expires even while the broker is disconnected.

## Buffer ownership

Worker owns downloaded JPEG and aligned decoder input/raw output. It frees those
on all completion/error paths. A completed frame stays worker-owned until the
nonblocking UI queue accepts it. Failure to enqueue frees it immediately. On the
LVGL owner, invalidate the old descriptor cache, bind the completed replacement,
then free the old frame. Drawing and PPA flush complete synchronously on this task;
no JPEG worker changes a frame referenced by LVGL. Never clear the old image on
HTTP/decode failure. Up to four queued frame commands are bounded by queue capacity.
Clock, temperature and greeting are separate objects; updates composite the stored
frame but never download or decode it again. Greeting fades in 300 ms, holds 4 s,
fades out 500 ms; Test UI does not represent recognition.

## Failure boundaries

NVS init/corruption preserves data and disables protected setup. Display failure
leaves serial diagnostics. Wi-Fi/C6 failures leave offline synthetic JPEG testing
usable. HTTP/decoder failures keep the old image. MQTT invalid/stale data keeps the
last valid value until its timestamp expires; explicit unavailable clears it.
Before SNTP, time and all available-temperature measurements are rejected/unavailable.
Secrets never enter the general UI diagnostics or application log messages.

The previous identity, local-camera, audio and profile architecture remains a
future milestone, not implemented behavior. Secure boot, encrypted persistent
storage, migrations, OTA rollback and 72-hour acceptance remain open.
