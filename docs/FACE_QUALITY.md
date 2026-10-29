# Diagnostic face-image quality, 0.4.5-dev

Menu -> Jakosc twarzy -> Uruchom pomiar enables the existing foreground detector.
The page shows scalar metrics for the largest detected face on its234x416 model
input. Stop/close removes foreground demand; an independently active background
lease remains governed by its existing deadline. No face image/template is saved,
sent or enrolled. The default screensaver, camera exposure and saved settings stay
unchanged. This prepares observable preprocessing, not household identity matching.

## Metrics and coordinate space

Use the same post-white-balance/sRGB RGB565LE input retained through inference.
SDK result boxes describe this original input before mapping to the180x318 preview.
Clip the ROI to the input, reject degenerate/outside boxes and flag edge contact.
All neighbour pairs contributing to detail remain inside the ROI.

Expand RGB565 channels to0..255 and compute integer luma with weights54/183/19
(sum256), rounded. Report:
* Width/height: pixels on the234x416 model input, not native sensor resolution.
* Mean: arithmetic luma mean0..255.
* Contrast: population luma standard deviation. An exact64-bit numerator
  (sum_of_squares*N - sum*sum) avoids cancellation on constant ROIs.
* Detail: mean absolute horizontal/vertical neighbouring-luma difference0..255.
  It varies with texture, noise, contrast and scale; it is not a calibrated focus
  measure and cannot establish whether a face is suitable for identification.
* Near-limit pixels: percentage with luma<=8 or>=247. This is measured on the
  processed RGB input, not proof of raw sensor saturation or underexposure.

Flags are provisional engineering hints: edge contact; either dimension<48px;
mean<45 or>220; contrast<12; detail<3. These thresholds are not validated on a face
dataset or calibrated to this camera. UI shows the highest-priority hint, while
all flags and metrics remain in the per-face status. No flag alters detection,
presence votes, exposure, greetings or future enrollment admission automatically.
No numerical score is presented as identity confidence or readiness.

## Resource and lifetime contract

No crop copy, histogram, extra image allocation, model or history is introduced.
Measure at most four accepted boxes on the existing bounded input before its reuse.
Per-face scalar results share the cancellation epoch/timestamps/status lock.
Source leases remain released before inference; image admission and compute mutex
are unchanged. quality_us measures post-model box/metric processing, while model
inference_us remains separate. Completion timestamp follows both operations.
Stale/missing detections show unavailable text; zero metrics are not invented.

The pure function supports dimensions3..800 by3..480, checks byte capacity and
coordinates before access. The maximum supported sum-product is below2^64;
uint32 would overflow sum-of-squares on large ROIs. No image or per-face coordinates
are logged; scalar quality processing time joins existing diagnostic metadata.

## Verification boundary

Host face_quality tests exercise black/white/constant ROI, high-detail versus
smoothed patterns, exact checkerboard moments, ROI-only neighbours, clipping,
minimum size, invalid dimensions/capacity/extreme coordinates, and maximum ROI
accumulator bounds with an unchanged sentinel. They verify arithmetic/bounds, not
image-quality classification or model accuracy. Actual camera metric response,
layout and overhead need physical receipts in DEVELOPMENT_LOG.md.


Final implementationbb2107fcda897a5cfdb91aa15d1d91dd572758bf passes host12/12,
all three local profiles/configs and four CI jobs39943561382. Final Wi-Fi image is
flashed/hash-verified on COM5. On63 nonzero-face samples, quality postprocess took
5.789..29.946ms (median17.323ms); no ROI allocation is added. This is postprocess
time, not total frame latency or an accuracy/readiness measure. Operator reports visible150x190px, mean126/255, contrast66, detail7.3,
near-limit4.3%, quality19ms and model578ms. Displayed metric availability is
physically accepted; distance-response, explicit layout acceptance and validated
quality classification remain pending. Final receipt in DEVELOPMENT_LOG.md.


## 0.5.0-dev local identity experiment

See [face recognition implementation and limitations](FACE_RECOGNITION.md),
[onboard Flash layout and migration proposal](IDENTITY_FLASH_LAYOUT.md), and
[actual build/test versus pending hardware acceptance](IDENTITY_VALIDATION.md).
The experimental MBF worker remains opt-in. The 0.6.1-dev candidate uses onboard
Flash for model storage, gated encrypted profile persistence and native identity
HTTP. Unprovisioned hardware remains RAM-only. See ITERATION7_REPORT.md and
FLASH_MIGRATION.md before installation. UBox/Open-Meteo/clock and camera leases remain.
