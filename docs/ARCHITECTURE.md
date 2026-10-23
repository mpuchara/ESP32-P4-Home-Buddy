# Architecture, iteration 3

The default profile keeps outputs disabled; bringup enables board/UI and wifi also
enables the ESP-Hosted station worker. Dependencies remain locked to IDF 5.5.1,
LVGL 8.3.11 and the existing BSP. Local camera capture runs independently from LVGL/networking. No biometric inference or audio capture runs.

* board: native DSI 480x800 RGB565 ping-pong and PPA rotation 270 degrees from
  logical 800x480. GT911 mapping remains x=physical_y, y=479-physical_x.
* ui: one LVGL owner, 2 ms ticks, handler sleeps at most 20 ms. A four-entry
  command queue transfers completed image ownership and copies status/temperature/
  greeting data. Configuration has a separate two-entry work queue. Network,
  NVS and JPEG decoding never run in LVGL callbacks.
* configuration: validated schema-1 snapshots, mutex-protected reads and a single
  NVS blob transaction. No automatic erase on corruption. All settings open
  directly without a PIN, as explicitly requested by the operator. Existing
  schema-1 records load without losing network/bridge/broker settings; the old
  48-byte PIN field is reserved, ignored on read and zeroed on the next save.
  Wi-Fi passwords are masked; UBox token editing is intentionally visible. NVS encryption remains incomplete.
* connectivity: ESP-Hosted 2.12.2 SDIO, legacy station APIs, bounded scan and
  disconnect requests, eight-result SSID list, DHCP events/RSSI and retries at
  5..30 seconds. Metadata RPC failure reports UNKNOWN rather than inventing a C6
  version. Scan and association/DHCP are physically verified. Wi-Fi storage on C6 is RAM.
* ubox_client: one task requests only original cached /snapshot.jpg. Two MiB
  staging, 8-second per-call timeout and 20-second total transfer deadline, no redirects, exponential 5..300-second backoff.
  ETag advances only after decoder success and UI queue acceptance; 304 preserves
  the image and accepted-image age. No capture, PTZ or private JPEG persistence.
* screensaver: baseline 8-bit JPEG only, source <=4096 per dimension, <=4 MP. IDF hardware
  JPEG engine uses aligned input/output; raw RGB565 follows8/16-pixel MCU dimensions from the first
  component sampling factors. CPU nearest-neighbor cover scale creates a separate 800x480 frame.
  It preserves aspect ratio, crops centrally and never stretches to force fit.
* services: explicit-validity SNTP plus one bounded Open-Meteo worker. Geocoding
  returns up to5locations for manual selection; chosen coordinates persist in a
  separate weather record. TLS CA/hostname verification,8192-byte JSON cap, depth8,
 20-second total deadline. Current Celsius is fetched every600seconds, expires
  after3600seconds and is rejected before time sync or for a future timestamp.
  No MQTT client/menu/subscription remains. Model weather is not a sensor reading.

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

NVS init/corruption preserves data and disables settings. Display failure
leaves serial diagnostics. Wi-Fi/C6 failures leave offline synthetic JPEG testing
usable. HTTP/decoder failures keep the old image. Weather invalid/stale data keeps the
last valid value until its timestamp expires; explicit unavailable clears it.
Before SNTP, time and all available-temperature measurements are rejected/unavailable.
Secrets never enter general diagnostics/logs. The UBox token is deliberately
visible in its edit field per operator request; Wi-Fi passwords remain masked.

The previous identity, local-camera, audio and profile architecture remains a
future milestone, not implemented behavior. Secure boot, encrypted persistent
storage, migrations, OTA rollback and 72-hour acceptance remain open.

## JPEG color boundary

Pinned IDF5.5.1 jpeg_decode.c selects RGB565 big-endian byte scrambling when its
rgb_order is RGB. LVGL LV_COLOR_16_SWAP=0 uses native little-endian words. Select
BGR for the decoder's native RGB565 byte order and explicit BT.601 conversion.
This corrects the byte layout, not a global red/blue panel swap; UI colors/mapping
remain unchanged. The operator confirmed corrected real-camera colors.

## Local camera and brightness

One camera task owns SCCB/CSI/ISP, three leased RAW8 GBRG frames and one discard buffer.
No FIFO of stale frames; preview and local light consume latest only. Preview closes
without disturbing UBox. A separate 100ms brightness worker owns existing PWM;
calibration/modes persist only on explicit config-worker saves in hb_display,
independent from existing main/weather records. See LOCAL_CAMERA.md and BRIGHTNESS.md
for metadata, memory, failure boundaries and acceptance limits.

## Diagnostic face consumer,0.4.0-dev

One lazy face worker consumes camera leases, preprocesses an upright4:3 RGB565
canvas and runs pinned local MSR/MNP. UI only selects the diagnostic and displays
bounded/expiring box metadata. The worker releases source leases before inference
and frees model/input on exit. A compute-budget mutex serializes expensive model
work and JPEG decode outside LVGL. See FACE_DETECTION.md for admission limits and
failure limitations. No local identities, remote inference or new persistence.
