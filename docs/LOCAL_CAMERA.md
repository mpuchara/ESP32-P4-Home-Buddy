# Local camera, iteration 3

Firmware 0.3.0-dev continues the reviewed 0.2.1 branch/PR1. Local camera frames
never replace the UBox screensaver and have no network/storage destination.

## Hardware and source evidence

On COM5, a read-only SCCB transaction on existing I2C0 (SDA7/SCL8) returned
address 0x36, registers 0x300a/0x300b = 0x5602. This identifies OV02C10 on this
unit; it is stronger evidence than the product suffix. Touch remains on that
same IDF master bus. No second I2C controller or touch/audio reset is introduced.

GUITION schematic sheets `2_LCD&CSI.png` and `3_ESP32-P4.png`, in SDK commit
534b6cffba51c87ff28076513b53bb389c22bfbe, show FPC2 CSI differential lanes and
clock, shared ES_I2C_SDA/SCL and 3.3V supply. CSI_IO0 is pulled up; CSI_IO1 has no
assigned P4 reset control in these sheets. The existing board display already
owns internal LDO3 at 2.5V for the MIPI PHY; camera does not reconfigure it.
The sensor module supplies its clock; no guessed GPIO XCLK/reset/pwdn is driven.
The selected source mode expects 24MHz input and one 400Mbit/s lane, despite the
connector exposing two lanes. The physical module oscillator is not measured
with an oscilloscope.

Mode is the SDK's actual 1288x728 RAW10, not an invented VGA register table.
IDF5.5.1 native CSI/ISP produces RAW8 GBRG in PSRAM from RAW10 input.
ISP explicitly selects PLL160 at160MHz. Streaming clears gated-clock0x4800,
matching the source sensor-driver stream operation without CSI line-sync packets.
The initial direct RGB565 route produced green/pale/noisy operator images and was
rejected. The RAW8 route is now operator-accepted for colors and detail. Capture
counters are around30fps; visible LCD preview is independently measured and slower.
No face model is loaded.

See [vendor receipt](../components/camera/vendor/SOURCE.md) for per-file licensing
and the decision not to import the GPL-influenced ESPHome implementation.

## Ownership and memory

One worker owns sensor, CSI, ISP and recovery. Three stable aligned RAW8 slots
are reserved by the ISR, published as latest-only and leased by readers. A fourth
IDF discard buffer handles overload without queue growth. A held slot is never
selected for DMA. Generation/token leases reject repeated/stale release; up to
32 distinct leases per published generation are bounded. API clients must release
leases promptly. No buffer is freed underneath a consumer during recovery.

Each capture/discard slot is937,664bytes; total four slots3,750,656bytes.
Portrait preview has one180x318 RGB565 allocation(114,480bytes), released on page
close. It holds a shared frame only while converting the thumbnail. The UBox
image descriptor and buffer stay independent.

The project-only native driver adapter in components/esp_driver_cam requires the
exact pinned IDF source hash, generates a build-directory CSI source and changes
two expressions: DMA transfer length uses output bits per pixel, and completion
invalidates the complete known framebuffer before CPU use. Stock input-bit sizing
would overrun RAW8 buffers when receiving RAW10. A compile guard prevents this
camera component from building against uncorrected CSI. Installed SDK files are
untouched; see the adapter SOURCE.md. Re-evaluate it on any IDF upgrade.

Initialization/PSRAM/I2C failures and 3s no-frame timeout stop only the camera.
Recoverable retries back off 5..300s. A failed ISP constructor or uncertain driver cleanup is quarantined
until reboot because pinned IDF5.5.1 can retain global claims without returning a
cleanup handle. This constructor failure was observed during a rejected clock
experiment; no endless allocation/reinitialization loop is permitted. Stable allocations are reused across recovery. There
are no raw frame logs/uploads. Failure injection and extended recovery/lease soak
on hardware remain acceptance work; host tests cover pool saturation, abort and
stale/double-release behavior.

## Local light and exposure

No dedicated ALS has been identified in the inspected board documentation or
current BSP. This is not a claim that arbitrary hardware revisions contain none;
no unverified ALS driver is enabled. Local camera is the current measurement
source, never UBox/Open-Meteo/HA.

At configurable 1..60s intervals (default5s), sample 32x24 points in the central
three-quarter ROI. Use unquantized8-bit channels from each2x2 Bayer cell, subtract the source
table black target16, then trim upper/lower quartiles. Reject >25% saturated
samples, >=90% near-black samples and invalid exposure/gain metadata. Dim valid
signals are retained without RGB565 quantization. Metric is
`log2(trimmed linear RGB luminance / exposure_us / analog_gain / digital_gain)`.
This is relative illumination, **not lux**. Reflective/colored scenes, occlusion,
the sensor's spectral response, quantization and low-light noise limit it.

Sensor exposure/gain are read locally. Digital reserved bits are excluded.
Exposure microseconds use the source mode's nominal 81.6667MHz pixel-clock/timing
description, not a measured clock; calibration works in relative stops. The host
exposure loop adjusts shutter/gain every >=500ms, bounded by read VTS minus15,
minimum4 rows, analog gain1..8. It does not leave the sensor permanently at a
fixed exposure to measure light. Measurements exclude the 200ms settling period
after control changes. No gamma/creative color processing is added to the metric.
Exposure and color quality under changing light still need physical validation.

## Face AI follow-up

Consumers can acquire immutable RAW8 GBRG frames through `hb_camera_acquire` and
release through `hb_camera_release`. Frame metadata sets rotation_ccw=90 for this
module mounting. Native storage and Bayer phase stay unchanged. The preview helper
hb_raw_thumbnail_upright converts and rotates90degrees counterclockwise in one
bounded pass, producing canonical upright width=source height,height=source width.
Future AI preprocessing must honor that metadata or use the same upright helper;
no inference implementation is claimed. Future inference runs outside LVGL, with a
single latest-frame lease and explicit bounded preprocessing allocations.

[ESP-DL3.3.14 manifest](https://github.com/espressif/esp-dl/blob/master/esp-dl/idf_component.yml)
declares ESP32-P4 and IDF>=5.3. This includes 5.5.1 by constraint, **not** a tested
integration on our rev1.0 chip. Human-face-detect0.5.2 declares MIT and ESP-DL
~3.3.14. [ESP-WHO](https://github.com/espressif/esp-who) documents P4 and IDF5.5
branch support, but GUITION is not its demonstrated reference BSP.

Candidates documented upstream: MSR120x160x3 plus MNP48x48x3, or one-stage
ESPDet224x224x3/416x416x3. RGB888 preprocessing alone needs 57,600/6,912,
150,528/519,168 bytes respectively, before tensors/weights/workspace/alignment.
Input RAW8 must be demosaiced, oriented and explicitly converted/resized into the model's actual
`dl::image::img_t` pixel type. Flash-rodata, dedicated model partition or mounted
SD are upstream loading options; our reserved `models` partition is unmounted and
not automatically the library's required partition name. No model was downloaded
or flashed. Actual weights, peak internal/PSRAM allocation and combined UBox
decode/inference timing must be measured before admission. No claimed AI FPS or
assumption that future audio fits.

Next: validate camera/light, then local enrollment/recognition and HA events;
speaker identification follows only after the face pipeline is stable.

## Preview diagnostics and quality

A brief startup sensor color-bar self-test records only eight synthetic RGB
channel samples; no room-frame samples/images are logged. Test kolorów toggles
that same local sensor pattern, disables light measurement and restores normal
capture on page close. Synthetic diagnostic mode is explicitly marked TEST.
Preview uses bounded gray-world red/blue gains and linear-to-sRGB lookup tables
only for the displayed thumbnail, never for the light meter or shared frame.
Colored scenes/low light can defeat gray-world; this is not calibrated color.
Operator accepted RAW8 colors/detail on2026-10-22 and reported head pointing right.
Portrait preview rotates CCW90; the operator confirmed upright head orientation
with the device used in landscape. Black target subtraction precedes preview white balance/gamma.
The light metric uses linear channels before either display operation. Synthetic
quadrants verify rotation direction and unchanged source data. This is a bounded
2x2 demosaic, not a calibrated photographic ISP or an AI accuracy measurement.
