# Architecture

## Implemented foundation

* `main`: read-only hardware diagnostics, non-destructive NVS initialization,
  opt-in board/UI and optional networking startup.
* `board`: small application adapter around the pinned MIT BSP. Native 480×800
  DSI RGB565 ping-pong buffers; PPA 270-degree rotation from logical 800×480.
* `ui`: LVGL 8.3.11, one owner task, 2 ms timer tick, 20 ms maximum handler sleep.
  Aligned single logical framebuffer in PSRAM. Touch remapping matches the PPA path.
  A screen click opens the menu only after release, so it cannot activate a hidden
  menu control. Touch activity updates an unsigned-wrap-safe 30 s timeout.
* `connectivity`: optional station initialization and bounded (one pending request)
  scan worker. Callbacks never block waiting for scans. Credentials are not accepted.
* `core`: framework-independent cover/crop geometry and touch transform.

All LVGL access after startup belongs to its task. The framebuffer remains alive
for the display lifetime. No camera or network buffers exist yet. A slow PPA flush
is synchronous; frame latency and tearing still need physical measurement.

## Intended service boundaries

Next add immutable configuration snapshots, a typed bounded event bus, and worker
commands. The UI submits work; it never downloads, writes profiles, enrolls or runs
inference in event callbacks. Suspend inactivity navigation during enrollment and
critical configuration transactions.

The UBox worker owns bounded HTTP/JPEG staging. An image becomes visible only
after a complete download, successful decoding and cover crop. Queue ownership
must transfer a completed buffer to the UI; the previous image is freed only after
LVGL can no longer reference it. ETag becomes committed only with that image.
Clock, temperature and greeting objects are independent of the source JPEG.
Redrawing an overlay may composite the image, but must not decode/download it again.
The time service must track synchronization separately from wall-clock value;
before NTP show unavailable time. ESP-IDF uses a POSIX TZ rule for Europe/Warsaw
(CET-1CEST,M3.5.0,M10.5.0/3), with daylight-saving boundary tests. Update the clock
label at minute boundaries without reloading the JPEG.

A single local camera owner shares frame references with face inference and
ambient-light estimation. Audio PCM and VAD run independently; models consume
bounded windows. Serialize or schedule expensive model operations after measuring
memory and CPU load; do not guess core affinity or parallel throughput.

Profiles have stable IDs separate from editable names. Store schema version,
length and integrity checks; use recoverable transactional writes. Corruption must
disable biometric use and preserve evidence, never auto-format a database.
PIN enrollment and administration must precede all sensitive workflows.

## Identity semantics (not implemented)

Maintain modality status, score, timestamp, quality and identity separately.
Different known IDs inside the fusion window produce conflict and suppress
personalized greetings. Matching IDs can confirm identity; face-only and voice-only
are valid. Unknown face plus known voice must preserve both observations without
claiming a face/voice association. Similarity is not probability.
Use explicit open-set thresholds, temporal consistency and bounded per-profile
cooldowns. Greeting and MQTT event cooldowns are independent and monotonic-clock based.

MQTT publishes retained discovery/state/availability, non-retained events with a
boot identifier plus sequence. Drop offline identity events rather than replaying
old detections. Do not hold AI work while reconnecting to HA.

## Failure isolation

A UI failure leaves serial diagnostics running. NVS failures are logged and data
preserved. Future camera, microphone and individual model failures disable the
affected service only. Last good snapshot, temperature subject to freshness, and
safe brightness persist in RAM during outages. No service gets an unbounded queue.
