# Experimental local face detection,0.4.0-dev

Kamera lokalna -> Test twarzy explicitly enables the diagnostic. It turns on the
local portrait preview and shows up to four green face rectangles and inference
milliseconds in the button. No identity names, embeddings, enrollment or HA events.
The default UBox screensaver never shows the local camera. Closing/timing out the
page, disabling preview or selecting the sensor color pattern stops detection.
Model/input allocations are freed by the worker after current work exits; one
bounded8KiB task stack remains until reboot after the first diagnostic use.

## Actual pipeline

Native RAW8 GBRG latest-frame lease -> CCW90 thumbnail180x318 -> appearance-only
white balance/sRGB -> center into424x318 RGB565LE canvas -> ESP-DL MSR/MNP.
The4:3 canvas matches MSR160x120 aspect;122pixel side margins avoid stretching
faces. Source leases are released before inference. Results are mapped back by
subtracting the horizontal margin, clipped to180x318, capped at four and hidden
when older than750ms or from an earlier enable/disable epoch. Model classification
score>=0.7 filters boxes; this is not confidence of household identity.
The worker waits500ms after each inference (at most2Hz). The displayed time is
model run duration, excluding our thumbnail conversion. Capture FPS is independent.
Good lighting and front-facing, sufficiently large faces are required for testing.
This2x2 Bayer demosaic/gray-world input is not calibrated color or an accuracy claim.

## Dependencies and deployment

Registry pins: espressif/esp-dl3.3.14 and human_face_detect0.5.1; lock also pins
DL-FFT0.8.0 and ESP-New-JPEG1.0.2. IDF remains5.5.1 and P4 minimum revision1.
MSR61,248bytes plus MNP129,968bytes are embedded in application rodata, with
CMake-checked hashes. No runtime download, separate model partition or SD mount.
Code/license/hash receipt is in components/face/vendor/SOURCE.md. The JPEG helper
has an Espressif-product restriction in its retained license; other new components
declare MIT. Existing hardware UBox decoder remains the active dashboard pipeline.

## Resource admission

A static mutex serializes model load/inference/teardown and hardware JPEG decode.
JPEG waits up to1s and keeps its existing error/last-good path on timeout; AI uses
nonblocking acquisition and yields to an active decode. Both run outside LVGL.
Loading requires free/largest PSRAM>=16/8MiB and internal>=192/128KiB. Inference
requires>=12/4MiB and64/32KiB respectively. A269,664byte input canvas is checked
before model construction. Observed load heap deltas above4MiB PSRAM or64KiB
internal stop and release diagnostics. Heap deltas are shared-heap observations,
not a private allocator enforcing a hard model arena cap. ESP-DL can assert on
allocation failure; conservative admission reduces this risk but is not proof of
complete fault isolation. Combined stress/fault injection is still required.

## Verification boundary

Host policy tests exercise memory thresholds, invalid/clipped boxes, expiry/disable
and in-place letterboxing with complete pixel parity and an overrun sentinel.
They do not run the neural network. Initial Wi-Fi firmware builds0x38d8b0bytes
within unchanged6MiB app slots. Actual initialization memory, inference latency,
visible boxes, cleanup and coexistence with full-size UBox need hardware receipts.
No vendor benchmark is presented as a measurement of this board.
