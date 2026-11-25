# 0.14.0 voice-only local trial

Development candidate based on 0.13.5-priority-calibration. No partition, camera,
face thresholds, model or HA event format changes.

1. In advanced audio diagnostics, run Model 3 s: slot L (try R if unsuccessful).
   Speak alone, with TV/music off. The firmware requires an actual successful
   16 kHz capture, 512D embedding and callback/PCM cleanup; this selects the
   working slot for this boot, not a certification of speech recognition.
2. Open Domownicy, select an existing enabled profile, then Głos: rejestracja /
   TEST. Accept the existing local biometric storage notice if necessary.
3. Rozpocznij, capture four different utterances; then verify with a fifth new
   utterance. Explicit Save uses the existing dual-bank profile persistence.
   Repeat for each consenting household member. No raw PCM is retained.
4. On the same page select Rozpoznaj po głosie (TEST). Speak alone. The candidate
   is compared with the existing voice profiles; best/second are raw cosine scores
   (not probabilities). UNKNOWN, AMBIGUOUS and UNAVAILABLE are valid outcomes.
5. Leaving the page cancels microphone work and restores previous face AUTO intent.

The trial threshold .65, margin .10, agreement .55, duplicate .9995 are engineering
starting values, not calibrated. Manual single-window candidates do not produce
confirmed identity, greetings, HA events or fused scores. The production speech
selection, model/numerical/resource and accuracy gates are not closed. TV/music and
multiple speakers remain unvalidated. Disabled/orphaned slots are excluded without blocking other profiles.
A capture result more than five seconds old fails closed; measure real device
inference before changing this policy. Voice profile data is local and follows
the existing storage security setting; no new dependencies or model asset.
Physical test and CI are required before release; do not automatically flash.

Next: collect genuine and impostor score distributions, quantify FAR/FRR, test
reboot/persistence, cancellation, silence, television, overlapping speakers,
noise, timings and heap. Then add validated VAD and voice confirmation, followed
by session-aligned face/voice scoring with explicit conflict and missing-modality
handling. Do not average raw cosine values from different models.

## Automatic voice-only trial (0.14.1)

Recognition > Głos TEST: start 10 min starts a bounded local session after a
successful standalone diagnostic and at least one enabled voice profile. Face
processing stops during this test and resumes prior AUTO preference on timeout
or Voice Off. The worker re-arms after results/silence using the experimental
activity gate. The current candidate is visible in Recognition, with provisional
confirmation after two independent windows; the saver shows a microphone TEST
indicator. No auto-start on reboot, voice events/greetings/HA, fusion or production
acceptance. Stop and identity administration cancel capture and clear the trial.
Disabled voice profiles are excluded without erasing enrolled vectors.

## Face + voice shadow scoring (0.14.2)

Recognition > Twarz + głos TEST starts a ten-minute session with both providers.
Inference is serialized by the compute mutex, and microphone activity selection
remains provisional. The UI shows independent voice candidate, joint decision,
threshold-relative support score and face/voice weights. Scores are NOT
probabilities. Poor face quality reduces its weight; voice weight uses the
active fraction of the accepted PCM window (not speech probability). Confirmed
modalities only contribute positive support; simultaneous differing candidates
are a conflict; evidence separated by more than 3 seconds is not combined.
Stale evidence and session changes are rejected. The existing face event path
is paused during this shadow trial and resumes afterward; no VOICE or BOTH
identity event is emitted. Neither calibration nor physical co-residency is
claimed. No production VAD or replay/speaker-overlap acceptance yet.
