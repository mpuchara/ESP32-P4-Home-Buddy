# Accuracy calibration protocol

No new device accuracy measurement has been performed. Synthetic host tests only
establish deterministic geometry/matching/enrollment policy behavior.

Use consenting participants and separate enrollment from fresh evaluation frames.
Select a person from Domownicy -> Testuj rozpoznawanie; use Wybrana osoba or Gosc
before deliberately recording Ocen probe. The UI reports raw local trial counts;
the service also exposes ambiguous/unavailable counts. Unknown false acceptance
and confusion between enrolled people are separate categories. Repeated frame IDs
are refused. Independent trials still require scene exit/re-entry and a documented
sampling design; adjacent frames are correlated.

Record counts and conditions: light, distance, yaw/pitch, glasses, expressions,
similar-looking people, unknown visitors, multiple faces, obstruction and re-entry.
Report end-to-end latency, all unavailable trials and all errors alongside TAR,
FRR, unknown FAR/rejection, enrolled-person confusion and ambiguity frequency.
Use the denominators defined in [full protocol](FACE_RECOGNITION.md). Zero samples
means no estimate. No image/vector export or public calibration dataset is created.

Do not lower thresholds to hide uncertainty. Compare ESPDet -> MNP and official
MSR -> MNP on the native ROI, visually inspect landmark positions and collect
within-person/different-person similarity distributions before selecting production
thresholds. None of these physical/calibration criteria is marked accepted yet.
