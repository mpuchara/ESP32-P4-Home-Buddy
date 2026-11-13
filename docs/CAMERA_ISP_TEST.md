# Camera ISP test firmware 0.12.0-isp-test

Experimental physical-board candidate, based on 0.11.12-dev. The existing CSI
owner, audited DMA output-length/cache fix, sensor mode, I2C bus, rotation,
leases and recovery are retained. The ISP now emits RGB565 with hardware Bayer
filter, demosaic, CCM and gamma; tuning values from the OV02C10 SDK JSON
linked below. This is a native-IDF integration, not a port of the complete
esp_video/IPA controller. Fixed CCM and gamma, simple existing sensor AE
using processed luma; no adaptive AWB, no 50 Hz anti-flicker, no full ISP
statistics calibration. Color order and physical stability need testing.
The face detector and same-frame native ROI use the ISP RGB pixels; no
second gamma/white balance or RAW reinterpretation. MBF model and matching
thresholds unchanged. Preview interval 33ms; actual FPS not guaranteed.
RGB565 storage doubles camera pool and identity snapshot; measure PSRAM
and DMA headroom in combined recognition/audio/Wi-Fi test.

Old RAW illumination calibration is not valid with processed RGB; the
published light reading is unavailable in this candidate. Manual backlight
remains usable; use manual mode during testing. No storage/partition/model
changes. Re-enroll existing person IDs if necessary; no automatic deletion.
Use only app-only update on previously migrated single-factory-v1 boards.
Check daytime/nighttime colors, FPS, five landmarks, enrollment, unknown
person false accepts, reboots and profile persistence. If camera fails,
restore the preceding app-only binary. No C6 flash or full erase.

Reference: https://github.com/wegi1/ESP32P4-JC4880P443C-I-W/blob/534b6cffba51c87ff28076513b53bb389c22bfbe/1-Demo/idf_examples/ESP-IDF/esp_brookesia_phone/components/espressif__esp_cam_sensor/sensors/ov02c10/cfg/ov02c10_default.json
