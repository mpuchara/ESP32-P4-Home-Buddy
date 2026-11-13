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

## 0.12.1: black level correction

Physical feedback on 0.12.0: preview is substantially smoother and image
quality appears improved, but a gray veil remains even with lens covered.
The sensor mode sets a RAW10 black target of 64 (registers 0x4002/0x4003).
The old software RAW8 pipeline subtracted 16 before gamma. The first
hardware RGB565 version omitted this and its gamma exponent 0.518 maps
the pedestal to roughly 60/255, producing lifted blacks.

The hardware gamma curve now maps 0..16 to zero and normalizes 16..255
before the existing power curve. IDF samples 16 points (first at 16,
last at 255); near-black interpolation retains the first visible steps.
No extra per-frame pass, no CSI/ISP ownership, AE thresholds, buffer,
color matrix, face model, storage or partition changes. Both preview
and full-resolution face ROI receive the correction. Host tests check
black, midtones, highlights and monotonicity. The 16 DN pedestal is
derived from the existing sensor configuration, not a new physical
measurement. Verify covered lens after AE settles, then dim/daylight
faces and enrollment. Exact black and low-light results require hardware.

## 0.12.2: fixed white-balance and saturation candidate

Physical feedback on 0.12.1: contrast substantially improved, colors pale
and greenish. The OV02C10 reference SDK specifies AWB ratios R/G
0.573..0.9096, B/G 0.5368..0.9634 and ACC saturation 136 (Q1.7 unity 128).
Our native IDF 5.5.1 pipeline has no vendor IPA white-balance controller.
Keep the existing CCM, and use distinct hardware gamma curves for R/G/B
as a conservative fixed post-CCM correction: red gain 1.65, blue 1.55,
green 1.0 in the black-corrected linear domain, with a soft highlight
shoulder. All channels retain zero at input <=16 and 255 at input 255.
These gain values are initial estimates, not physical calibration values.
Enable the hardware color block at the reference saturation 136/128,
contrast 128/128, brightness 0 and hue 0. Both preview and native face
ROI use the same corrected ISP frame; no new full-frame CPU processing.
Do not change Bayer order, frame dimensions, exposure control, camera
ownership, AI model, thresholds, stored profiles, or partitions.
Host tests check endpoints, monotonicity, midtone gain and highlight
headroom. Verify neutral gray/white, skin tones, red/blue objects and
covered lens in daylight and warm artificial light. Adaptive AWB
remains a follow-up if a fixed correction cannot cover both illuminants.
