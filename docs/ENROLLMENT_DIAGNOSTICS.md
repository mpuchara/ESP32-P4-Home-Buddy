# Enrollment diagnostics 0.13.2

Use desktop Chrome/Edge, normal running device, USB Serial JTAG console at
115200 (SDK defaults). Close any other monitor. Installer and capture share
one serial port: no simultaneous access, no writes/reset in capture mode.
Start capture, enroll a household member on the device, reproduce failure,
optionally add markers, stop and download TXT. Keep browser open. Full log
up to 4 Mi characters, last 48k in view, explicit dropped-data marker.
Capture uses streaming UTF-8. Export remains local. Existing logs may
contain network/device information; review before sharing.

Enrollment-only logs: BEGIN, ACQUIRE, DETECT, ROI_MAP, FRAME, QUALITY, SELECT,
CAM, MNP_INPUT, MNP_RESULT, POINT, GEOMETRY, ALIGNMENT, INFER, SIM,
ENROLL_STABLE, VERIFY_SIM, VERIFY, COMMIT. Correlate by frame seq and session.
MNP_RESULT coords=10 means 5 x/y pairs; zero results means detector rejected
the crop, not a bad similarity threshold. GEOMETRY uses squared inter-eye
distance and normalized signed nose/mouth cross-products. ALIGNMENT reports
both the existing alignment-residual gate and the combined geometry gate.
SIM and VERIFY_SIM are cosine similarities, not probabilities; reason 0
accepted, 1 invalid/stale, 2 duplicate, 3 mismatch. No new photo, raw frame,
embedding, names or persistent biometric files are written by diagnostics.
Point coordinates and similarity values are biometric-related diagnostic
data: share only voluntarily. No changes to quality gates, models, exposure,
sample acceptance, profile schema, flash layout or recognition thresholds.

## 0.13.3: changes based on hardware log

One 298-second USB recording: 114 detections (113 single faces), 38 final
candidates, 35 valid MNP/geometry passes, 3 MNP empty results, 8 accepted
samples, 9 explicit similarity rejections. 10 final verification attempts,
only the last accepted; log ends during commit, not a durable-save receipt.
First verification seq 4960 has best .6933, 2 above .65 and all 8 above .55.

Use first good settled enrollment frame, otherwise a second candidate.
Preserve 5s frame freshness gate and log stale outcomes. Retry empty MNP
once with slightly tighter candidate, still require five genuine landmarks
and unchanged geometry. Keep frontal anchor admission .60. Later require
2 anchors >=.50 or best accepted template >=.65 and 2 anchors >=.43.
Guided held-out verification requires best >=.65 and at least 3 >=.55;
manual verification, conflict checking and ordinary recognition unchanged.
This is an experimental household greeting policy, not a calibrated
biometric access policy. Test with other people before judging accuracy.
No camera ISP, model, storage schema, partition or display changes.

## 0.13.3 follow-up
The physical trace shows good landmark geometry but varying cosine scores,
long multi-frame cycles and LCD underruns. Early exit after one good settled
frame, max two candidates; MNP retry on a slightly tighter box, same ROI.
Preserve eight accepted templates; allow strong alternate matches with anchor
support. Final verification: two distinct fresh frames, top score >=0.60,
two templates >=0.50 each; reset on mismatch, conflict or invalid detection,
15 s maximum gap, 1 s minimum separation. No live recognition threshold
change. Enrollment-only completion deadline 8 s, fresh acquisition still
<=500 ms, session/sequence checks unchanged; identity events retain 5 s.
Do not infer once verified while awaiting save. UBox pauses new snapshot
requests and defers JPEG decode during enrollment, without changing ETag or
failure counters; already running decode may finish. Guided UI refresh 1 Hz.
No model/flash/ISP changes, no hardware validation yet.
