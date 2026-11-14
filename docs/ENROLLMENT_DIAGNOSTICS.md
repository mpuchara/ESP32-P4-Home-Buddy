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
