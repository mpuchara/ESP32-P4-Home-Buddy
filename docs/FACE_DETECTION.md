# Experimental local face detection, 0.4.2-dev

Kamera lokalna -> Test twarzy explicitly enables local detection. Up to four green
rectangles and model inference milliseconds are shown. Closing/timing out the page,
disabling preview or selecting the color pattern stops detection. Model/input free
after current work exits; one bounded 8 KiB task stack remains until reboot.
No identity names, embeddings, enrollment, recognition events or saved face images.

## Accuracy-oriented input and model

Native RAW8 GBRG latest-frame lease -> CCW90 portrait 234x416 RGB565LE -> existing
appearance-only white balance/sRGB -> ESPDet Pico Face 416x416. The SDK letterboxes
the portrait input without stretching and maps detections back to that input.
Home Buddy scales coordinates to the 180x318 diagnostic preview with integer bounds
checks. Results expire 1200 ms after completion with a 2500 ms absolute source-age cap.
They are capped at four, require score >= 0.7 and
are cancelled across enable/disable epochs. The worker targets a 500 ms start-to-start period (at most 2 Hz).
The displayed time excludes our thumbnail conversion. Capture FPS is independent.

0.4.0 used MSR 160x120 with a 4:3 canvas; the portrait occupied only about 68x120
model pixels. Enlarging that source alone could not increase the fixed network
resolution. 0.4.1 changes the network itself and supplies about 234x416 portrait
pixels, at a higher expected latency. This is a hypothesis to test on this camera,
not an accuracy guarantee. No lowered acceptance threshold or held-over detection
is used to hide misses. A rectangle means face detection, not household identity.

The operator confirmed actual 0.4.0 detections below 40 ms, but frequent misses
with glasses/beard/dark clothing and a dark background. Camera image/rotation were
accepted. The operator has not yet accepted 0.4.1 detection stability.

Sensor AE already adjusts integration and analog gain every 500 ms, with settling
checks and bounded controls. These controls and the calibrated illumination meter
remain unchanged in 0.4.1. AI preprocessing does not change RAW exposure metadata
or display brightness anchors. Limited illumination/noise can still cause misses.
Do not use this experimental diagnostic as the sole trigger of sensitive actions.

## Dependencies and resource admission

Exact registry pins remain ESP-DL 3.3.14 / human_face_detect 0.5.1, with DL-FFT 0.8.0
and ESP-New-JPEG 1.0.2. Selected ESPDet416 weights are 578,160 bytes, embedded in
app rodata with a configure-time SHA256 check. No model partition or runtime download.
Source and exact licenses: components/face/vendor/SOURCE.md. ESP-New-JPEG retains
its Espressif-product restriction; other new components declare MIT.

The portrait input allocation is 194,688 bytes. A static mutex serializes model
load/inference/free and hardware JPEG decode outside LVGL. JPEG waits up to 1 s;
AI uses nonblocking acquisition and yields. Loading needs PSRAM free/largest
>=16/8 MiB and internal >=192/96 KiB; inference needs >=12/4 MiB and >=64/32 KiB.
Observed load deltas above 4 MiB PSRAM or 64 KiB internal stop/release diagnostics.
These shared-heap checks are not a hard private arena. Upstream allocation failure
can assert; conservative admission does not establish complete fault isolation.
Source camera leases are released before inference. No recognition/audio capacity
is inferred from these limits.

## Verification boundary

Host tests cover admission, clipping, nonfinite scores, input-to-preview scaling,
extreme coordinates, expiry and disable. They do not run the neural network.
0.4.1 build/physical receipts are recorded in DEVELOPMENT_LOG.md as obtained.
Remaining: detection stability in the operator's room, actual load memory, latency,
cleanup, combined full-size UBox load and sustained/fault tests.


## Measured latency and 0.4.2 UI correction

On this board ESPDet416 takes about 757-803 ms in the captured sample, with a
1,862,120-byte PSRAM and 29,328-byte internal load delta including input. The old
750 ms source-only expiry hid completed detections and showed AI: czekam. The
operator confirmed that symptom. 0.4.2 separately bounds completed-result age
(1200 ms) and source age (2500 ms), with capture<=completion<=now checks. A newly
completed zero-face result clears rectangles immediately; there is no successful-
detection hold across a newer miss. These are display diagnostics, not presence
or identity events. Host tests reproduce 800 ms inference and both age limits.


Operator accepts that 0.4.2 detects faces much more consistently, usually 700-800ms,
with occasional short waiting captions. On reopening, the old 128KiB internal
largest-block admission refused despite281,495 free internal bytes and a100KiB
largest block after actual cleanup. This was a policy refusal, not an allocator
failure. Based on measured ~30KiB load delta, require96KiB largest while retaining
192KiB aggregate admission,64KiB observed internal load guard and all PSRAM limits.
Tests cover the new exact boundary. Repeated physical reopening remains to verify.
