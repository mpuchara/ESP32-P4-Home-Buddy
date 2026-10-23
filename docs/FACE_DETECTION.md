# Experimental local face detection, 0.4.0-dev

Kamera lokalna -> Test twarzy explicitly enables the diagnostic. It turns on the
local portrait preview and shows up to four green face rectangles and inference
milliseconds in the button. No identity names, embeddings, enrollment or HA events.
The default UBox screensaver never shows the local camera. Closing/timing out the
page, disabling preview or selecting the sensor color pattern stops detection.
Model/input allocations are freed by the worker after current work exits; one
bounded 8 KiB task stack remains until reboot after the first diagnostic use.

## Actual pipeline

Native RAW8 GBRG latest-frame lease -> CCW90 thumbnail 180 x 318 -> appearance-only
white balance/sRGB -> center into 424 x 318 RGB565LE canvas -> ESP-DL MSR/MNP.
The 4:3 canvas matches MSR 160 x 120 aspect; 122 pixel side margins avoid stretching
faces. Source leases are released before inference. Results are mapped back by
subtracting the horizontal margin, clipped to 180 x 318, capped at four and hidden
when older than 750 ms or from an earlier enable/disable epoch. Model classification
score>=0.7 filters boxes; this is not confidence of household identity.
The worker waits 500 ms after each inference (at most 2 Hz). The displayed time is
model run duration, excluding our thumbnail conversion. Capture FPS is independent.
Good lighting and front-facing, sufficiently large faces are required for testing.
This 2 x 2 Bayer demosaic/gray-world input is not calibrated color or an accuracy claim.

## Dependencies and deployment

Registry pins: espressif/esp-dl 3.3.14 and human_face_detect 0.5.1; lock also pins
DL-FFT 0.8.0 and ESP-New-JPEG 1.0.2. IDF remains 5.5.1 and P4 minimum revision 1.
MSR 61,248 bytes plus MNP 129,968 bytes are embedded in application rodata, with
CMake-checked hashes. No runtime download, separate model partition or SD mount.
Code/license/hash receipt is in components/face/vendor/SOURCE.md. The JPEG helper
has an Espressif-product restriction in its retained license; other new components
declare MIT. Existing hardware UBox decoder remains the active dashboard pipeline.

## Resource admission

A static mutex serializes model load/inference/teardown and hardware JPEG decode.
JPEG waits up to 1 s and keeps its existing error/last-good path on timeout; AI uses
nonblocking acquisition and yields to an active decode. Both run outside LVGL.
Loading requires free/largest PSRAM>=16/8 MiB and internal>=192/128 KiB. Inference
requires>=12/4 MiB and 64/32 KiB respectively. A 269,664 byte input canvas is checked
before model construction. Observed load heap deltas above 4 MiB PSRAM or 64 KiB
internal stop and release diagnostics. Heap deltas are shared-heap observations,
not a private allocator enforcing a hard model arena cap. ESP-DL can assert on
allocation failure; conservative admission reduces this risk but is not proof of
complete fault isolation. Combined stress/fault injection is still required.

## Verification boundary

Host policy tests exercise memory thresholds, invalid/clipped boxes, expiry/disable
and in-place letterboxing with complete pixel parity and an overrun sentinel.
They do not run the neural network. Initial Wi-Fi firmware builds 0x38d8b0 bytes
within unchanged 6 MiB app slots. Actual initialization memory, inference latency,
visible boxes, cleanup and coexistence with full-size UBox need hardware receipts.
No vendor benchmark is presented as a measurement of this board.


Local and Linux CI verification (2026-10-23): nine host suites and all three
firmware profiles pass on implementation 84a0cb0c68cc1f9dfc26c8f47084d977de9cb24e.
CI run: https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39902790684
COM5 boot and passive samples confirm the flashed 0.4.0-dev firmware runs the
camera around 30 fps with detection disabled. A natural 2304x1296 UBox JPEG
(857,018 bytes) was accepted and decoded/covered in 224 ms. No model activation
appeared in these samples. This does not verify inference or visible boxes.
