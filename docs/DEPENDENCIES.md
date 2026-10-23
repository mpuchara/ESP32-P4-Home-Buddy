# Dependencies and provenance

| Component | Selection |
|---|---|
| ESP-IDF | 5.5.1; fcae32885b0296b32044cb99ecbdc50d98dddb83 |
| LVGL | 8.3.11, MIT; matching the inspected BSP's LVGL 8 integration |
| GT911 | espressif/esp_lcd_touch_gt911 1.1.1, Apache-2.0 |
| Wi-Fi remote | espressif/esp_wifi_remote 1.2.2 |
| ESP-Hosted host | espressif/esp_hosted 2.12.2, Apache-2.0 |
| Transitive components | exact versions + integrity hashes in dependencies.lock |
| BSP | ultramcu/guition-jc4880p4-bsp 324970bade0d1f4e52880fe8016580368bc1e06e, MIT |
| Polish font | Noto Sans, SIL OFL 1.1; generated subset, assets/fonts/OFL-NotoSans.txt |

Direct manifest constraints are exact; commit the generated lockfile. The IDF
manifest deliberately rejects other IDF versions. CI uses the same IDF image tag.
LVGL 9 is not selected simply because it is newer: the current port uses verified
LVGL 8 APIs. A future migration needs its own render/memory/hardware checks.

## Vendored sources

Only four source/header files and the MIT license from the pinned BSP are copied
under components/board/vendor. Attribution and upstream register facts are retained.
See that folder's SOURCE.md for local deviations. Do not remove its MIT notice
when distributing application binaries. The independent schematic repository is
reference material, not copied application code.

## Future model compatibility research, not implemented dependencies

Espressif's [speaker verification example](https://github.com/espressif/esp-dl/tree/master/examples/speaker_verification)
lists IDF 5.3/5.4/5.5 support on P4. It uses embedded prerecorded samples and a local
feature database; it does **not** supply this board's microphone/VAD/enrollment UI.
The inspected manifest references speaker_verification ^0.3.0 and feat_database ^0.1.0.
Select exact compatible versions and measure their model before integration.
Do not copy sample thresholds into household identification without evaluation.

ESP-DL human-face detection/recognition models are candidates for P4; ESP-WHO
examples help with alignment and databases, but their board/audio/camera BSP is
not interchangeable with GUITION. The inspected
[esp_cam_sensor table](https://github.com/espressif/esp-video-components/tree/master/esp_cam_sensor)
does not list OV02C10. Iteration3 now identifies the physical OV02C10 and uses an attributed SDK mode
table with native IDF capture. Similar names OS02N10/OV5647 are not substitutes.

## Iteration-2 font extension

Same Noto Sans source/hash/license and lv_font_conv 1.5.3, still --no-compress.
Additional range 0x2013-0x2014,0x2022,0x2026,0x2212; --lv-fallback
lv_font_montserrat_14 provides the pinned LVGL keyboard/control symbols. Real
bitmap tests cover those symbols plus Polish/ASCII. No new font license is added:
Montserrat/control glyphs are already distributed by the pinned LVGL component.
The complete reproducible command is in the generated hb_font_20.c header.

## Iteration 3 camera

IDF-native esp_driver_cam/esp_driver_isp and the existing shared I2C bus; no new
registry/ESPHome runtime dependency or lockfile change. Selected GUITION SDK
Apache-2.0 mode table and its LICENSE/SOURCE receipt are in components/camera/vendor.
Sullb/profi-max/esp-video-components were inspected with provenance recorded there.
OV02C10 is now physically identified; similar-name drivers are not substituted.
ESP-DL/ESP-WHO research is in LOCAL_CAMERA.md. The optional diagnostic AI consumer is described below.

The native CSI adapter requires the exact pinned IDF source hash and applies only
output DMA length/cache invalidation corrections in the build directory. It adds
no registry dependency or external runtime; source/Apache receipt is distributed
with packages. See components/esp_driver_cam/SOURCE.md.

## Experimental face diagnostic pins

ESP-DL3.3.14, human_face_detect0.5.1, DL-FFT0.8.0 and ESP-New-JPEG1.0.2 are in the
lockfile. Main direct dependencies use exact versions. Existing board/LVGL/C6 pins
are unchanged. Model hashes, source commits and retained licenses are documented
in components/face/vendor/SOURCE.md. ESP-New-JPEG permission is Espressif-product
restricted; do not label it unqualified MIT. Model rodata is built into the P4 app.
Compatibility is being tested on IDF5.5.1/rev1; upstream timing is not our result.
