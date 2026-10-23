# Incremental development plan

Each phase needs a buildable commit and evidence in DEVELOPMENT_LOG.md.
Physical acceptance cannot be inferred from successful compilation.

| Phase | Deliverable | Current state |
|---|---|---|
| 0 | Research, dependency pins, architecture, budget, CI, skeleton | Initial research/foundation delivered; all CI jobs passed |
| 1 | P4/PSRAM, 800×480 LVGL, touch, backlight, C6 networking | Display/touch/brightness/station/DHCP verified; cold boot/reconnect pending |
| 2 | Fullscreen cached UBox JPEG, cover crop, ETag, NTP/Warsaw clock, Open-Meteo temperature layers | Implemented; JPEG/clock verified, Real UBox/colors/304 and device weather verified; outage/soak pending |
| 3 | Direct setup, persistent configuration, complete Polish touchscreen settings | All settings without PIN per operator request; Wi-Fi provisioning verified; encryption pending |
| 4 | Exact camera driver, frame capture, illumination metric, two-anchor calibration | OV02C10 ID and ~30fps capture verified; RAW8 colors/detail/upright operator-confirmed; real calibration and stability pending |
| 5 | Face detection/alignment/embedding, local profile enrollment and unknown handling | Pending |
| 6 | ES8311/PCM/VAD, local speaker embeddings and on-device enrollment | Pending |
| 7 | Temporal fusion, greetings, independent cooldowns, MQTT Discovery/events/HA examples | Pending |
| 8 | Secure persistent storage, migrations, deletion, OTA/recovery | Pending |
| 9 | Profiling, fault injection, reconnect, recovery and 72-hour physical soak | Pending |

## Next acceptance gates

1. CI build all three profiles and Linux DST tests for iteration 2.
2. Confirm synthetic JPEG colors/crop, no bars, greeting fades, full touch range,
   slider range, cold boot and repeated menu/image transitions.
3. Wi-Fi association/DHCP and SNTP verified; reconnect and reboot restore pending.
4. Configure live bridge/weather; verify 200/304/401/503, partial/oversize/error
   rejection, stale/unavailable model temperatures and SNTP/Warsaw time.
5. Repeated swaps and failures with heap/largest-block tracking, watchdog and soak.
6. Finish encrypted credential storage/provisioning/recovery before production.

Model integration must follow camera/audio bring-up. Validate peak memory before
concurrent face/voice operation. Brightness must use local camera exposure/gain
metadata or a confirmed dedicated sensor, never the remote UBox image.
Face/voice enrollment must remain on-device, authenticated and offline-capable.
The full product is incomplete until every definition-of-done workflow and the
physical 72-hour test have passed.

## Iteration 3 acceptance overrides

Continue reviewed06f8a11 on the existing draft PR1, never merge/release implicitly.
Camera and brightness software now exist; colors/detail/upright are operator-confirmed. Validate visible preview pace,
actual day/night anchors, manual/temporary/auto, settings restore, camera timeout
recovery and repeated LVGL cycles. Preserve naturally available2304x1296 and
cold-power/long-WiFi/72h soak as pending until observed. Source/build/host results
do not substitute for those checks. Face enrollment/HA events follow camera/light;
speaker work follows face. No C6 writes or bridge modifications.

## Acceptance update, 2026-10-23

Physical five-minute temporary override returned to AUTO per operator. Natural
UBox2304x1296 complete transfer, hardware decode and800x480 cover are now observed
on0.3.2-dev. Full-size visual/repeated-replacement acceptance, real day/night
anchors/reboot persistence, injected camera recovery and cold-power/72h tests
remain pending. Camera pool/recovery regression now joins8host suites; all CI
profiles pass for implementation19e88f0. No model/inference milestone is claimed.

## Brightness acceptance update, 2026-10-23

Night-anchor save and AUTO dimming were operator-confirmed on0.3.3-dev after a
second flashlight test anchor was saved. Repeat the day point under representative
daytime lighting and explicitly save AUTO. Physical reboot restore, changing-light
stability and extended soak remain pending; do not treat the flashlight as final
daytime acceptance.
