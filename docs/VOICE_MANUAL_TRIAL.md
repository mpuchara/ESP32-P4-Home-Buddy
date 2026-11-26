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

0.14.3 hardening: whole-window activity quality (not last-frame silence), automatic
capture bound to the face session at start, cancellation-aware diagnostic receipt,
local saver candidate only after trial confirmation. No raw audio or network event.
The test session is limited to ten minutes, with gaps during inference/rearming.
Full microphone/VAD/co-residency accuracy and timing still require physical trials.

## 0.14.4 continuous local AUTO trial

Both voice-only and fusion AUTO now run until explicitly stopped, administrative
cancellation or reboot. They still require a successful L/R diagnostic during
this boot, a consenting local household and enabled voice profiles. No automatic
microphone activation after reboot. Switching modes is direct. Automatic capture
adds an adaptive idle noise floor, bounded sign-change/energy onset filter and
three consecutive onset frames retained in the candidate buffer. Manual enrollment
and diagnostic capture preserve the original gate. Synthetic tests cover silence,
rapid sign changes and a low-frequency signal; these fixtures do not validate
speech, playback, television, music or concurrent-speaker rejection. No new model
or dependency. Healthy no-window capture is distinguished from capture/model
faults; faults retry with bounded 1/2/4/8/16/30-second backoff. A successful window
or healthy no-window resets backoff. Local UI/serial expose counts and last model
processing time; the saver remains visibly marked while AUTO is enabled. Gaps
during inference and audio-driver rearm still exist. Nothing is sent to HA by the
voice/fusion trial and scores remain uncalibrated.

## 0.14.5 bounded double-buffer AUTO capture

AUTO allocates two 48,240-sample PSRAM windows (192,960 bytes total). The audio
callback fills them in order and never modifies a full buffer. The worker runs
inference on the first while the callback can fill the second. Each window has
its own gate/timestamp. A first result is published promptly; one more result or
terminal receipt follows. The mailbox never overwrites a pending result, and
backpressure is bounded. No third window is queued; audio is stopped/detached and
both buffers scrubbed before rearming. Manual capture is unchanged. This reduces
but does not eliminate gaps; real throughput, memory pressure, actual speech VAD
and HA acceptance remain unverified. The host mock exercises capture during
inference and distinct timestamps; it is not a physical concurrency benchmark.

## 0.14.6 rotating continuous capture

AUTO now starts the ADC once per explicit session and rotates two fixed PCM
buffers until stop, driver/model failure or bounded mailbox timeout. The manual
60-second diagnostic retains its separate API. Producer and consumer exchange
ownership via atomic full flags; no callback waits for inference. A saturated
producer drops frames, counts them and resets partial onset after a gap. Old
windows are discarded before/after inference, without reporting them as a
recognized identity or hardware fault. A 60-second idle heartbeat reports
healthy silence without restarting the ADC. Terminal failures use existing
recovery backoff. No raw audio leaves RAM. Host tests exercise more than two
windows with one driver start, cancellation, overload and stale rejection.
This is not a promise of lossless audio or physical real-time performance.

## 0.14.7 acoustic window screening

Before automatic speaker inference, a bounded worker-side PCM screen checks 150
20 ms frames for active energy, centered sign changes, derivative power and
300 ms envelope variation. It rejects stationary tones, very broadband noise,
low-activity and invalid windows; the support fraction caps voice fusion weight.
Rejected windows are counted separately from hardware faults and clear positive
voice evidence without restarting capture. Automatic inference now uses the
remaining five-second freshness budget instead of a 120-second cooperative
budget; an expired otherwise-successful/cancelled result is stale, not a driver
fault. Constructor/graph sections are still non-preemptible. Model load timing
now includes package verification. Manual diagnostics/enrollment retain the
original capture and inference budget. Synthetic tone/noise/modulated fixtures
are regression checks, not validated VAD, playback rejection, diarization or
speech accuracy. A television carrying natural speech can still pass.

## 0.14.8 bounded feature workspace and natural pauses

AUTO keeps its checked FFT plan, mel coefficients and feature/frame scratch for
at most ten seconds since the last inference. Feature and frame arrays are
scrubbed after every attempt. The worker releases the workspace on idle,
pressure, errors, cancellation and stop; no model, flash mapping or compute
lease is held across windows. Each model load still verifies its pinned hash.
This optimizes repeated frontend allocations, not model construction. The
manual diagnostic still creates and destroys its own resources. AUTO allows
up to 750 ms pause within a fixed 3.015 s window (versus the manual 500 ms),
retaining the two-second activity and acoustic screening floor. Adapter and
capture agree on the same policy. Host regressions cover pause tolerance,
allocation reuse and release, including faults. No trained VAD, TV rejection
or measured latency improvement is claimed.

## 0.14.9 independent statistical VAD

The pinned BSD-licensed libfvad/WebRTC GMM detector runs in the AUTO worker,
not the I2S callback, after the existing acoustic window screen. Mode 3,
16 kHz, 150 full 20 ms frames, provisional minimum 60% positive votes. A
negative vote rejects the window without stopping ADC or running the speaker
model. The vote fraction additionally caps fusion quality, not probability.
Each window starts with fresh detector state. Cancellation and allocation errors
are handled; no extra model partition or cloud. Manual enrollment and standalone
diagnostics retain their previous policy. Prior standalone synthetic false
positives motivate retaining the independent acoustic veto. Real speech,
playback and multi-speaker validation are still required. Host tests use the
actual vendored VAD and separate worker mocks for rejection flow.
