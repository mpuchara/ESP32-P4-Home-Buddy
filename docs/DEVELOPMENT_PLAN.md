# Incremental development plan

## Iteration 13 status

0.11.3-dev advances blank-storage initialization, bounded Flash transactions,
recovery isolation and combined voice readiness. See ITERATION13_REPORT.md.
The latest baseline already records owner-confirmed retention/AUTO after restart
on 0.11.2-dev. Candidate app and speaker model are now installed and verified under owner
approval. Initial physical boot restored the same generation and resumed ready
AUTO. Post-change restart, greeting/HA and audio/model execution still need the
combined operator session. No recovery was needed for the valid database.


## Current iteration 12 policy (0.11.0-dev)

The selected product mode is **persistent, unencrypted, local household storage with
automatic restoration**. The recognition build runs the existing schema-2 engine in
two 512 KiB banks inside the unchanged 1 MiB partition. Intentional household changes
save automatically; valid face/voice profiles and the previously enabled AUTO mode
restore locally at startup. No password, PIN, manual unlock, eFuse provisioning or
mandatory encryption. Physical extraction is not prevented. Experimental voice
identification remains gated. Implementation/host tests do not establish physical
reboot persistence; installation and the combined acceptance session remain pending.
See [storage decision](PERSISTENCE_DECISION.md), [database](PROFILE_DATABASE.md) and
[iteration report](ITERATION12_REPORT.md). Older iteration notes below are historical.


Each phase needs a buildable commit and evidence in DEVELOPMENT_LOG.md.
Physical acceptance cannot be inferred from successful compilation.

Active continuation: onboard MBF execution, one-person RAM enrollment/fresh-frame
confirmation, personalized saver greeting and real recognized REST event delivery
are operator-confirmed. The real recognized-only notification action is also
confirmed by a matching HA trace and operator receipt. Five saver/re-entry cycles
passed after the Flash read watchdog fix. The controlled 90-second stay and real
leave/return notification test also passed with two matching HA traces. Continue
automatically to offline/stress checks. Unattended background completion and clean
model release are observed on a retry; exact lease-start timing was not captured.
The first attempt cancelled during navigation and is not an expiry result.
The Stop/navigation path has
operator-confirmed absence of new notifications; its exact command/release timing
was not independently captured.
Quantified multi-person accuracy remains pending. Voice remains
capacity planning only in this iteration. Secure provisioning needs separate
authorization; the current candidate profile mode is LOCAL_UNENCRYPTED.

| Phase | Deliverable | Current state |
|---|---|---|
| 0 | Research, dependency pins, architecture, budget, CI, skeleton | Initial research/foundation delivered; all CI jobs passed |
| 1 | P4/PSRAM, 800×480 LVGL, touch, backlight, C6 networking | Display/touch/brightness/station/DHCP verified; cold boot/reconnect pending |
| 2 | Fullscreen cached UBox JPEG, cover crop, ETag, NTP/Warsaw clock, Open-Meteo temperature layers | Implemented; JPEG/clock verified, Real UBox/colors/304 and device weather verified; outage/soak pending |
| 3 | Direct setup, persistent configuration, complete Polish touchscreen settings | All settings without PIN per operator request; Wi-Fi provisioning verified; encryption pending |
| 4 | Exact camera driver, frame capture, illumination metric, two-anchor calibration | OV02C10 ID and ~30fps capture verified; RAW8 colors/detail/upright operator-confirmed; real calibration and stability pending |
| 5 | Face detection/alignment/embedding, local profile enrollment and unknown handling | Real ESPDet416/MNP/MBF execution and one-person RAM enrollment/fresh-frame confirmation/leave-return scenario accepted; multi-person and impostor accuracy pending |
| 6 | ES8311/PCM/VAD, local speaker embeddings and on-device enrollment | Pending |
| 7 | Temporal fusion, greetings, independent cooldowns, Webhook/REST events and HA helper examples | Personalized greeting, real recognized REST delivery, recognized-only notification, controlled one-person encounter deduplication/return and operator-reported Stop/navigation notification outcome accepted; timed expiry and outage acceptance pending |
| 8 | Secure persistent storage, migrations, deletion, USB recovery | Transactional gate and USB tool implemented; secure provisioning and physical power-loss/deletion acceptance pending |
| 9 | Profiling, fault injection, reconnect, recovery and 72-hour physical soak | Pending |

## Next acceptance gates

1. CI build all four profiles and Linux host regression for 0.6.1-dev.
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


## 0.5.0-dev local identity experiment

See [face recognition implementation and limitations](FACE_RECOGNITION.md),
[onboard Flash layout and migration proposal](IDENTITY_FLASH_LAYOUT.md), and
[actual build/test versus pending hardware acceptance](IDENTITY_VALIDATION.md).
The experimental MBF worker remains opt-in. The 0.6.1-dev candidate uses onboard
Flash for model storage, gated encrypted profile persistence and native identity
HTTP. Iteration 12 selects LOCAL_UNENCRYPTED independently of hardware provisioning. See ITERATION7_REPORT.md and
FLASH_MIGRATION.md before installation. UBox/Open-Meteo/clock and camera leases remain.


## Iteration 8 continuation

Candidate implements AUTO/OFF/TEST, reboot waiting, evaluation and bounded audio diagnostics. Physical gates: scheduled app update/profile loss, navigation/AUTO and microphone acceptance, consenting multi-person negatives, outages and 72-hour soak. Iteration 12 selects automatic local unencrypted persistence with the existing USB updater; speaker runtime remains conditional.


## Iteration 10, 2026-10-25

Implemented foundation: single-owner timestamped capture, preliminary activity gate, pinned 3s embedding adapter, synthetic RAM matching/enrollment and gated fusion. Next physical gate: one silence/speech/slot receipt, then real model numerical/resource tests and voice-specific calibration. Real household voice enrollment and secure persistence are still pending.


## Iteration 11, 2026-10-25

Implemented: pinned reference feature comparison; four-template voice enrollment with fresh verification and atomic RAM replacement; stable-ID reconciliation; separately authorized schema-2 transactions and synthetic migration. Remaining milestones: physical microphone/model/resource acceptance, speech-selection evaluation with consenting household data, voice-specific open-set calibration, then explicit voice enablement. Iteration 12 supersedes the unlock proposal with automatic local unencrypted persistence; see PERSISTENCE_DECISION.md.
