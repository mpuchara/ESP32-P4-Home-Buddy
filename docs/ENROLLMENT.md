# Local touchscreen enrollment

1. Menu -> Domownicy. Enter a display name and explicitly confirm adding it.
2. Refresh the list, select the person and choose Rejestruj / dodaj probki.
3. Use the local preview. Capture 8-15 diverse samples at least one second apart.
   Try modest left/right/up/down poses, usual glasses and varied illumination.
4. Missing/multiple faces, invalid alignment and highly redundant vectors are
   rejected. Brightness/contrast/detail metrics are advisory heuristics.
5. Verify with a fresh frame at least one second after the last enrolled sample.
6. Explicitly confirm saving. Cancellation leaves the prior committed set intact.

Samples are locally generated MBF vectors; no PC, cloud, phone app or photo file
is used. Without authorized secure provisioning, data remains explicitly RAM-only and disappears after restart. With gated protected persistence enabled, save/rename/delete commits to onboard Flash; check the persistence status and follow PROFILE_DATABASE.md reboot acceptance. A repeated session copies the existing set into staging and adds diverse new
samples up to 15 total, committing only after fresh verification and confirmation.
Cancellation leaves the existing samples intact. Duplicate similarity >0.995 and sample counts
are initial engineering settings, awaiting camera calibration.

Rename preserves the ID; delete requires a separate deliberate confirmation and
clears all the person's vectors. No real household entries are automatically
created. [Full behavior and limitations](FACE_RECOGNITION.md).
