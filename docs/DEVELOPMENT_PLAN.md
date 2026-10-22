# Incremental development plan

Each phase needs a buildable commit and evidence in DEVELOPMENT_LOG.md.
Physical acceptance cannot be inferred from successful compilation.

| Phase | Deliverable | Current state |
|---|---|---|
| 0 | Research, dependency pins, architecture, budget, CI, skeleton | Initial research/foundation delivered; all CI jobs passed |
| 1 | P4/PSRAM, 800×480 LVGL, touch, backlight, C6 networking | Experimental implementation; hardware acceptance pending |
| 2 | Fullscreen cached UBox JPEG, cover crop, ETag, NTP/Warsaw clock, temperature layers | Geometry implemented/tested; transport/rendering pending |
| 3 | PIN first-run setup, persistent configuration, complete Polish touchscreen settings | Pending |
| 4 | Exact camera driver, frame capture, illumination metric, two-anchor calibration | Pending sensor identification |
| 5 | Face detection/alignment/embedding, local profile enrollment and unknown handling | Pending |
| 6 | ES8311/PCM/VAD, local speaker embeddings and on-device enrollment | Pending |
| 7 | Temporal fusion, greetings, independent cooldowns, MQTT Discovery/events/HA examples | Pending |
| 8 | Secure persistent storage, migrations, deletion, OTA/recovery | Pending |
| 9 | Profiling, fault injection, reconnect, recovery and 72-hour physical soak | Pending |

## Next acceptance gates

1. All profiles build from a clean checkout with committed lockfile: CI passed.
2. Physical display visible, correct orientation and four-corner touch.
3. Backlight range and cold boot/menu transitions; no recurring display corruption.
4. C6 actual firmware handshake, stable scan, then protected Wi-Fi provisioning and DHCP.
5. Add read-only UBox transport and JPEG decoder before either AI model.

Model integration must follow camera/audio bring-up. Validate peak memory before
concurrent face/voice operation. Brightness must use local camera exposure/gain
metadata or a confirmed dedicated sensor, never the remote UBox image.
Face/voice enrollment must remain on-device, authenticated and offline-capable.
The full product is incomplete until every definition-of-done workflow and the
physical 72-hour test have passed.
