# Incremental development plan

Each phase needs a buildable commit and evidence in DEVELOPMENT_LOG.md.
Physical acceptance cannot be inferred from successful compilation.

| Phase | Deliverable | Current state |
|---|---|---|
| 0 | Research, dependency pins, architecture, budget, CI, skeleton | Initial research/foundation delivered; all CI jobs passed |
| 1 | P4/PSRAM, 800×480 LVGL, touch, backlight, C6 networking | Display text verified, scan verified; touch corners/backlight/DHCP pending |
| 2 | Fullscreen cached UBox JPEG, cover crop, ETag, NTP/Warsaw clock, temperature layers | Implemented in 0.2.0-dev; host policy tests passed, live acceptance pending |
| 3 | PIN first-run setup, persistent configuration, complete Polish touchscreen settings | Direct Wi-Fi per operator request, PIN for UBox/MQTT; NVS/settings implemented; physical provisioning and encryption pending |
| 4 | Exact camera driver, frame capture, illumination metric, two-anchor calibration | Pending sensor identification |
| 5 | Face detection/alignment/embedding, local profile enrollment and unknown handling | Pending |
| 6 | ES8311/PCM/VAD, local speaker embeddings and on-device enrollment | Pending |
| 7 | Temporal fusion, greetings, independent cooldowns, MQTT Discovery/events/HA examples | Pending |
| 8 | Secure persistent storage, migrations, deletion, OTA/recovery | Pending |
| 9 | Profiling, fault injection, reconnect, recovery and 72-hour physical soak | Pending |

## Next acceptance gates

1. CI build all three profiles and Linux DST tests for iteration 2.
2. Confirm synthetic JPEG colors/crop, no bars, greeting fades, full touch range,
   slider range, cold boot and repeated menu/image transitions.
3. Configure Wi-Fi on device; prove association/DHCP, reconnect and reboot restore.
4. Configure live bridge/broker; verify 200/304/401/503, partial/oversize/error
   rejection, retained/stale/unavailable temperatures and SNTP/Warsaw time.
5. Repeated swaps and failures with heap/largest-block tracking, watchdog and soak.
6. Finish encrypted credential storage/provisioning/recovery before production.

Model integration must follow camera/audio bring-up. Validate peak memory before
concurrent face/voice operation. Brightness must use local camera exposure/gain
metadata or a confirmed dedicated sensor, never the remote UBox image.
Face/voice enrollment must remain on-device, authenticated and offline-capable.
The full product is incomplete until every definition-of-done workflow and the
physical 72-hour test have passed.
