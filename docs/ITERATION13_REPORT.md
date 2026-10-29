# Iteration 13: persistent identity stabilization

Date: 2026-10-26, updated 2026-10-29. Installed candidate: **0.11.12-dev**; final-source transactions and restart verified; 0.11.4/0.11.5 withdrawn, branch `codex/local-face-recognition`.
Baseline: `6c095eb505f371c121211117f83280a46fb61c8b` (verified against origin).
Final source commit and binary SHA256 are recorded in the USB package metadata.

## 2026-10-29 0.11.12 coherent enrollment installed, profile 33 retained

Subsequent physical acceptance: owner completed guided acquisition, fresh-frame
verification and explicit Save. Generation 34 readback validates both banks 34/33,
one face record changed, same identity/name/enabled/count and unchanged voice.
Normal restart restores 34. All eight stored samples meet the intended criteria:
pairwise min/median/max 0.6085/0.8023/0.9370; leave-one-out top-three means
0.7849-0.9098. Previous generation 33 had three unsupported outliers.

Collection needed 40 admitted candidates and 132058 ms from the first candidate
to eight supported samples; this is still a slow enrollment, not a latency success.
The first held-out verification failed, a later fresh frame passed. Pool never
exceeded 15; measured selector range 513-253288 us, enrollment stack watermark
5424 B, no observed restart/overflow. These bounded-session measurements do not
prove every input's worst case or a long soak. Reliable recognition/greeting/HA
and phone latency remain pending on the new unchanged generation 34.

Source `cd3b2a104cd7e38e1bf9bcab24714cfbbada3e19`; application 4,508,672 B,
headroom 2,831,360 B, SHA256
`3cd7f1e9ccbd83e3e1a2beda30ed770257180c8701faa76b57628d6fcf65e541`.
The owner found the MSR/ESPDet comparison steadier but without confirmation.
Actual saved generation 33 contains three leave-one-out outliers, motivating
coherent acquisition rather than classifier threshold relaxation. Guided collection
selects eight from a rolling pool capped at the existing 15 slots, using all-pair
agreement >=0.600 and each sample's top-three mean >=0.700. A separate fresh frame,
other-profile conflict check and explicit Save are still required. Counter reports
the selected cohort. This does not establish calibrated accuracy or live latency.

32/32 CTest, 24/24 Python, 3/3 checks, complete ASan/UBSan, formatting/whitespace,
target build and 17 IRAM symbols pass. Target compiler frames: selector 960 B,
support helper 16 B and worker 4112 B; fixed selector scratch 900 B, no allocation
or schema/status growth. UI frame subtotal remains 2736 B. Real enrollment stack
and selection latency remain pending and are logged numerically on each admission.
All five exact-source CI jobs pass:
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/40907790754.

Fresh private full backup validates banks 32/33. App-only write at 0x20000,
verify_flash and full 16 MiB readback pass: all other sectors, including profiles,
NVS, table and models, are byte-identical. Normal boot restores B/33 saved/clean,
camera 29.5-30.1 FPS and accepted UBox snapshot. Recognition is STOP after the
foreground diagnostic, with the AUTO preference preserved. No crash observed in
the bounded 45 s boot capture. This initial install retained the existing profile;
the later owner-approved generation-34 commit is recorded above.

## 2026-10-29 0.11.11 cadence candidate installed with profile 26 intact

Source `08288bc2ac544bd549c3fce1525f78be4b57af5c`. App 4,507,168 B,
headroom 2,832,864 B, SHA256
`bbde53bc6d8ae79a6e3e06ed0533b1ec755ca74c5b3cf4f277367bebab46fd54`.
Fresh private full backup, application-only write at 0x20000, verify_flash and
complete 16 MiB readback pass. All other sectors/profiles/NVS/models/table are
unchanged. Normal boot restores A/26 saved/clean, ready AUTO, models 1/0. UBox
image and HA host GET HTTP 200 recover after initial network retries. No recognized
admission/greeting was observed during the boot capture before the new encounter.

All five exact-source CI jobs pass, including sanitizers and four target builds:
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/40893822504.
Local validation and safety bounds are below. Final public package is
`dist/0.11.11-dev-confirmation-cadence`, with 54 verified hashes and a verified ZIP.
A subsequent runtime receipt shows a known 1/3 candidate with wait_ms=100 followed
by UNKNOWN/0 with wait_ms=1000. This validates configured cadence selection; no
new physical latency improvement is inferred. The next three-confirmation,
visible greeting and HA encounter remains an operator acceptance test.

## Subsequent 0.11.11 test context changed before latency acceptance

The live capture later records a new power-on (reset_reason=1), boot generation 28,
then a new eight-sample enrollment/verification and saved clean generation 29.
No panic/watchdog marker accompanies that boot. Owner confirms changing the profile and restarting the device. The response to
the timing prompt is over 10 seconds/no greeting; it is not an unchanged
profile-26 comparison or evidence of a spontaneous firmware reset.
Configured fast cadence is observed, but physical speedup remains unaccepted and
the previous profile-26 latency baseline cannot be treated as a controlled A/B of
this later enrollment. Preserve current saved generation 29 for the next controlled encounter. The
cadence change has runtime confirmation, but matching stability remains open.
A source audit finds camera GBRG conversion consistent with its configured format
and native landmark order consistent with the pinned MBF alignment reference;
this limited audit does not establish physical pixel/color or population accuracy.

## 2026-10-29 0.11.11 first greeting about 5 s, second remains candidate

Owner reports a display greeting after about five seconds, then no second UBox
notification and about five seconds to recognition in diagnostics. Clarification:
the diagnostic label was KNOWN/candidate, not confirmed. The recorded current
boot/profile generation is 32, with fresh re-enrollment/restart activity earlier,
so this is not a controlled same-profile-26 before/after latency comparison.

Log: first KNOWN/1 at 88610 ms, actual KNOWN/3 confirmed at 91513 (2903 ms series),
recognized admission 91518, UI command admitted 91931 (418 ms after confirmation).
Only one confirmed result is recorded in this boot window. Later weak matches
reset hits before 3/3; there is no GREETING_SKIP/cooldown entry. The diagnostics
TEST mode does not publish background recognized events or greetings. Therefore
KNOWN/candidate is not evidence of a lost second notification. Five-minute greeting
and separately configured HA cooldowns remain unchanged; repeatability stays open.
The owner's ~5 s is one approximate display observation, not p95 or LCD timing.

## Saved generation-32 profile cohesion diagnostic

Private USB readback validates both 32/31 banks. Eight enabled face templates have
valid unit normalization. Local pairwise cosine min/median/max: 0.2940/0.5748/0.7715.
Leave-one-out means of each template's top three other templates range 0.5642 to
0.6934; none reaches the existing 0.700 matching threshold. Each template has at
most two other templates above 0.700. This is a diagnostic of internal cohesion
relative to the current policy, not a calibrated enrollment gate or proof of person
identity, impostor rejection, the exact cause of live variability or model failure.
Fresh-frame verification against three samples does not establish whole-set cohesion.

No templates/names/IDs were printed or committed; all embeddings and the complete
profile readback remain private. No data, thresholds, matching or enrollment rules
were changed. A controlled ordinary restart restores 32 saved/clean and ready AUTO.
Next: one controlled frontal enrollment update on the same identity, with steady
front lighting/minimal pose changes, then compare stored-set cohesion before more
notification trials. This distinguishes acquisition variance from profile coverage.

## Controlled frontal acquisition saved generation 33; mixed cohesion remains

Owner completes the requested frontal update on the same saved profile. The live
worker verifies a fresh frame, commits 33 in 5,431,958 us and resumes ready AUTO,
models 1/0. Private readback validates 33/32; exactly one face record changes, with
ID/name/enabled/count preserved and voice data unchanged. Normal restart restores
33 saved/clean and ready AUTO. No firmware or matching thresholds were changed.

The new eight unit-normalized samples have pairwise cosine min/median/max
-0.0197/0.3498/0.8405. Five leave-one-out top-three means reach 0.700 (previously
zero); the other three are approximately 0.363/0.403/0.393. This is a mixed set,
not a uniformly improved profile or calibrated accuracy result. The frontal
acquisition yields several strongly supported samples while substantial outliers
remain. Do not attribute all failures to operator pose or relax the classifier.

Next controlled comparison uses the existing foreground MSR/ESPDet switch with
the same profile to compare reference native MSR+MNP landmarks with the current
ESPDet-proposal/MNP adapter. The initial ESPDet crop remains common to both; this
is not an independent full detector benchmark. Keep the same pose/light, inspect
actual confirmed/candidate state, return the switch afterwards and compare private
runtime receipts. No extra profile replacement or new decision policy is assumed.
Biometric vectors and per-sample data remain private; these are aggregate receipts.

## 2026-10-29 real greeting/HA accepted on 0.11.10; latency baseline

Owner reports more stable landmarks. Current profile is now generation 26 following
fresh enrollment during the session, so this is not an isolated accuracy comparison
against retained profile 23. Foreground TEST reaches known score 0.836 and actual
3/3 fresh confirmations. That mode intentionally suppresses background notifications.

After return to UBox, AUTO reaches 3/3 with score 0.758 at boot uptime 339531 ms.
RECOGNIZED_EVENT queued=1 at 339535; GREETING_ACCEPTED and GREETING_UI queued=1 at
339879. Owner confirms visible greeting and the event in HA, while reporting a
long wait. This is one real display/HA encounter, not population accuracy,
calibration, phone delivery or target p95 acceptance. Confirmation-to-UI-command
admission is 348 ms (not a physical LCD timing measurement). First matching
candidate to confirmation is 34068 ms, with intervening weak scores resetting the
tracker. The final first-to-third accepted series spans 4911 ms. Actual entry time
and HA/phone end-to-end latency are not measured. Models stay 1/0, saved/clean 26.

## 0.11.11 pending-background-confirmation cadence candidate

The worker previously inserted ten cancellable 100 ms idle slices after every
active analysis. Use one slice only for ready AUTO/BACKGROUND with known match,
not yet confirmed, and hits strictly between zero and the required count. After
confirmation, unknown/ambiguous/unavailable or no hits, the normal ten-slice wait
returns. STOP remains one slice; TEST and ENROLL remain ten. There is always a
scheduler yield and command check. Compute mutex/ownership, model instances,
priorities, buffers, thresholds, three distinct fresh-frame confirmation, resets,
HA/greeting cooldowns and opt-in are unchanged.

The production helper's host regression verifies 200 ms total configured waiting
between the first/second and second/third matches instead of 2000 ms. This is an
1800 ms scheduling reduction under three successful analyses, not a promised
physical latency reduction. Weak-match retries observed above remain a separate
limitation. MATCH_RESULT reports configured wait_ms for target verification.
Local 32/32 CTest, 24/24 Python, 3/3 checks, full ASan/UBSan, formatting/whitespace,
recognition build, 17 linked IRAM functions and 2736 B UI frame subtotal pass.
Target installation and real speedup require separate evidence.

## 2026-10-29 corrected native MNP crop installed

Firmware source `585669dcde99d67a22fb313521a71531a734a0e8`. App 4,506,992 B,
headroom 2,833,040 B, SHA256
`a0aecf5944e703a4853d8e893207f24274f4b5cce637dbc70ef22dfbdb9200d3`.
A fresh private full 16 MiB backup preceded app-only installation at 0x20000.
verify_flash and full readback pass; every other sector, profiles 23/22, NVS,
models and table are unchanged. Normal boot restores B/23 saved/clean.
AUTO remains inactive after the preceding foreground diagnostic; the update does not change that opt-in. Enable AUTO explicitly before the next background encounter.

Exact-source CI: all five jobs passed:
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/40889248031.
Local 32/32 CTest, 24/24 Python, 3/3 checks, complete ASan/UBSan, formatting,
recognition build, 17 linked IRAM symbols and 2736 B UI frames pass. Public package
`dist/0.11.10-dev-mnp-square` includes validation receipt, hashes and verified ZIP.
The old helper demonstrably fails the new MNP containment regression. Hardware
point stability, matching against retained templates, greeting and HA/phone delivery
remain the next acceptance checks; source/boot evidence does not establish them.

## 2026-10-29 sharp preview but intermittent landmarks; crop bug reproduced

Owner reports a sharp image and stable rectangle, while five points sometimes
vanish under a nearly unchanged pose. Foreground TEST log still alternates valid
MBF/matches with ROI refusal, no face and missing landmarks. Code review against
the pinned human_face_detect MNP::run identifies a concrete adapter mismatch:
MNP converts the candidate to a square before limit_box(), whereas the application
expands width/height independently. A portrait box can have its MNP square clipped
by the cropped input even though that square fits in the original sensor frame.

A regression using box (70,100)-(160,240) in the 234x416 detector input fails the
old helper at containment of the native MNP square. Candidate 0.11.10 computes the
same square/inclusive edges, adds bounded context, shifts to sensor bounds and
caps context at 640. True squares larger than the existing buffer remain rejected.
Tests assert no additional MNP clipping beyond native sensor boundaries, including
portrait/near/edge examples and more than 500 varied accepted detector boxes.
Native RGB, MNP/MBF models, real landmarks, alignment checks, model identifier,
profile ABI/data, matching thresholds and three-frame confirmation stay unchanged.
Local checks pass: 32/32 CTest, 24/24 Python, 3/3 project checks, full ASan/UBSan,
formatting/whitespace and target build with seventeen IRAM symbols and 2736 B UI
frame subtotal. The old helper failed the new geometry regression before the fix.
This proves the geometry defect/fix, not that every physical landmark/matching
variation is caused by it. Existing profile 23 is retained; target retest is needed.

## 2026-10-29 notification trace firmware 0.11.9 installed

Firmware source `714bfd15905129231c7b7788fed2003eb7cc6b6e`. App 4,506,912 B,
headroom 2,833,120 B; SHA256
`f6d97ff60b898c463149af3db9fe89c48a19fce6bbfa437010fc766f7bbe6388`.
Fresh private 16 MiB backup; application-only write at 0x20000, verify_flash and
full readback pass. All other sectors, profiles B/23 and A/22, NVS, models and table
are unchanged. Normal boot restores saved/clean B/23 and ready AUTO, models 1/0.
UBox JPEG resumes after network initialization; HA_STATUS confirms enabled=1,
reachable=1/result=HOST_OK, HTTP 200 at about 36 s. That is a host GET, not proof of
webhook/recognized-event/automation/phone delivery. Initial startup connection
failures recovered, so they are not treated as a persistent HA connectivity fault.

Source CI run: https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/40886130930.
Exact-source CI status at this check: all five jobs passed. Local source checks passed as recorded below. The current encounter
must be judged from MATCH_RESULT/event/greeting/HA receipts and owner observations.
No diagnostic POST, config/opt-in change, synthetic recognized event or profile
edit was performed. Live unhalted debugger metadata is not used as acceptance.

## 2026-10-29 runtime encounter: unstable match prevents confirmation

Owner repeated the UBox encounter on 0.11.9 and saw no greeting. Runtime receipts
show UNKNOWN with best=0.544/0.457 at threshold 0.700; a KNOWN candidate reaches
0.726 and hits=1/3, then a 0.515 UNKNOWN resets hits=0/3. No confirmed recognized
admission or GREETING_ACCEPTED receipt is observed in this window. This identifies
a failure before notification production; it is not evidence of a broken display
queue. Three fresh confirmations remain required and no threshold was changed.

HA_STATUS is enabled/reachable, result=DELIVERED HTTP 200 with sent counter rising
1->2. Without confirmed recognition, those posts are not proof of a recognized
payload or a phone action. HTTP response alone does not prove HA automation or
phone delivery. The next controlled test needs a frontal, sharp complete face in
the existing diagnostic preview, to assess matching stability before re-enrollment
or changing any calibration. Greeting/HA/phone acceptance remains failed/open.

## 2026-10-29 generation 23 saved; no greeting/phone encounter observed

Owner reports the new profile saved. Fresh passive log confirms generation 23,
saved/clean, ready AUTO, models 1/0 and actual 512D MBF executions (roughly 1.3-1.4 s
pipeline time), mixed with no-face/ROI/landmark rejection. Owner reports no visible
greeting and no phone notification. MBF success does not establish matching or
three-frame confirmation. Existing logs omit those decisions and HA enable/status.
Live unhalted JTAG metadata returned an invalid event-bus next=0 despite production
init next=1; coherency is unverified, so those reads are discarded for conclusions.
No HALT/injection/configuration change was requested during that read.

Candidate 0.11.9 adds bounded numeric MATCH_RESULT (scores, policy threshold/margin,
confirmation hits), RECOGNIZED_EVENT queue admission, GREETING_ACCEPTED/SKIP,
GREETING_UI queue admission and periodic HA_STATUS (enabled/reachable/result/HTTP/
counters). It logs no names, IDs, credentials, frames, PCM or embeddings. No sample,
matching, confirmation, freshness, event, cooldown, HA opt-in or voice-gate change.
The purpose is exact runtime triage, not assuming successful notification delivery.
Local validation: 32/32 CTest, 24/24 Python, 3/3 checks, formatting/whitespace and
recognition build pass, with seventeen linked IRAM functions and 2736 B UI frames.
Candidate installation and runtime results are separate from these source checks.

## 2026-10-28 delete/recreate exposes guarded cold-start fragmentation

Later 0.11.8 log records successful fresh-frame verification after several mismatch
attempts (accepted=0/conflict=0 followed by accepted=1/conflict=0), eight samples,
then a 6,620,810 us durable commit, clean generation 18 and ready AUTO. This is real
worker/save evidence; it does not by itself prove visible greeting or HA delivery.

Owner then deleted the profile and created a new one under artificial lighting.
The log shows model release (1/1), new generations 19/20, then MBF admission failure:
236,079 B internal free but largest block only 43,008 B, below the unchanged 65,536 B
guard. No detection/landmark/embedding ran and ENROLL_STATUS remained samples=0,
ready=0. Therefore that attempt does not measure artificial-light recognition.
This route is distinct from administrative disable/enable parking; explicit deletion
continues to release resources. Do not mask this failure by lowering admission.

Private full profiles readback validates banks 20/19, one enabled current profile
with zero samples. A controlled ordinary restart restores 20 and raises the largest
internal block to 180,224 B. No old profile restore, Flash erase or firmware update.
Restart is recovery, not a permanent fix for repeated delete/recreate fragmentation.
Next: real new enrollment after this cold restart, then assess lighting only if the
model actually starts. Fresh-frame matching and physical greeting/HA remain open.

## 2026-10-28 feedback firmware 0.11.8 installed and boot verified

Firmware source 8528881b7d9e64247013e3419991d5e773da30e1; all five CI jobs pass:
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/40718883733.
The preceding unsaved session was already STOP; durable generation 17 remained.
After a fresh private full 16 MiB backup, installed only the app at 0x20000:
4,505,632 B, headroom 2,834,400 B, SHA256
`d85dd9e99f4341382093a8798a234c2d283a38bb87d04b98c35c23e385c917a3`.
verify_flash and full 16 MiB readback pass. Every other sector, including both
profile banks, NVS, table and models, is unchanged. Normal boot restores B/17,
saved/clean, ready AUTO, models 1/0 and continuing camera frames. No recovery,
profile edit, C6/eFuse action or new enrollment was performed during installation.

Local 32/32 CTest, 24/24 Python, 3/3 checks, formatting/whitespace, target build,
2736 B UI frame subtotal and seventeen linked IRAM functions pass. Package:
`dist/0.11.8-dev-enrollment-feedback`, with source metadata and validation receipt.
The guided reason display and a successful new-light verification/save still
require the next operator attempt; boot/CI do not constitute that acceptance.

## 2026-10-28 eight-sample save remains gated; feedback correction candidate

Owner reports eight samples and visible landmarks but cannot confirm a subsequent
re-enrollment under changed lighting. A running-target metadata read found ENROLL,
samples=8, verified=false, with the saved generation still 17. Concurrent metadata
reads can overlap publication and are not atomic snapshots; transient zeroed reads
are not treated as device state. No halt/queue injection/profile write was requested.
The current passive log reports ENROLL/ready and does not record verification
success/failure, so it cannot establish the exact earlier rejection cause.

Source review confirms a UI feedback defect: the guided page overwrites the actual
worker reason with generic waiting text. Candidate 0.11.8 shows the reason and
separates fresh-frame verification mismatch from another-profile conflict, with
bounded numeric ENROLL_VERIFY/ENROLL_STATUS logging and no names or embeddings.
All quality thresholds, three matching samples, freshness/conflict checks and
explicit verified-only Save remain unchanged. Local validation passes: 32/32 CTest,
24/24 Python, 3/3 project checks, formatting/whitespace and target build, including
seventeen linked IRAM functions. No target installation or physical acceptance
is claimed for this candidate. Visible landmarks/eight samples
alone do not prove verification. Candidate validation and installation must remain
separate; do not replace the active unsaved RAM session during this diagnosis.

## 2026-10-28 existing profile re-enrollment and restart verified

Owner completed guided fresh re-enrollment, eight samples, verification and save
using the existing profile. Actual commit took 6,687,649 us, generation 17. Private
full profile-partition readback validates header/payload SHA256 in both 17/16 banks.
Comparison finds exactly one changed face record, eight new face samples, preserved
ID/name/enabled state and byte-identical voice database. No profile deletion or
new identity. A controlled debugger-free restart restores bank B/17, saved/clean
and automatically ready AUTO, models 1/0. No firmware change. This confirms new
sample persistence, not yet an actual post-update identity/greeting/HA encounter.

## 2026-10-28 operator touch acceptance and passive background observation

Owner confirmed installed 0.11.7, successful touchscreen disable/enable and no
visible flicker. Passive log confirms two clean commits (15/16), 5,955,644 and
6,026,045 us, model loads/releases 1/0. A controlled debugger-free USB restart
restored generation 16, saved/clean and ready AUTO without manual activation.
UBox initially retried its network connection and later accepted an HTTP 304;
subsequent full JPEG retry also recovered. No new firmware/profile edits or JTAG.

Passive background window: eleven HEALTH samples all report AUTO mode 4, ready 1,
models 1/0, with continuing camera frames at 30 FPS and UBox activity. Nineteen
logged rejected inference attempts include nine face detections: four outside ROI
and five without five valid landmarks. The landmark stage ran in some attempts.
Logged failed attempts have zero MBF time; successful inference is not logged by
this diagnostic, so these observations do not prove full embedding/matching or
successful recognition. Owner reports poor enrollment photos. Current observed
ROI/landmark failures occur before template comparison and must be distinguished
from enrollment quality. Greeting/HA identity acceptance remains open. Voice gates
and all image-quality/model thresholds remain unchanged.

## Final installed 0.11.7 receipt

Firmware source: `098ede32577a914bff544fe968cdd9bd418e2cb2`. Application 4,505,248 B,
partition headroom 2,834,784 B, SHA256
`2b66003749fd57daabbcd94fa32ebd3049429bfc27f0ff15e505a3a95bb559dc`.
Fresh private full backup, app-only write at 0x20000, verify_flash and full 16 MiB
readback pass. All other sectors, profiles, NVS, table and both models unchanged.
No recovery erase, re-enrollment, partition, C6 or irreversible provisioning change.

One final-source real worker disable/enable cycle committed generations 13/14,
with stable boot ID, names, IDs, face/voice sample counts and models 1 load/0 releases.
Ready is false while disabled and true after enabling. It uses checked queue
injection while both harts are briefly halted; OpenOCD watchdog and touchscreen
limitations described below still apply. After debugger shutdown, full profiles
readback validates 14/13; generation 14's entire 312,416-byte canonical payload is
byte-identical to pre-test generation 4. A debugger-free normal restart restores
14, saved/clean, and automatically reaches ready AUTO. CSI continues about 30 FPS.
This supersedes the earlier final-source installation/restore placeholders below.

Actual first boot: restore 162,768 us; storage-stage internal lifetime low 285,239 B,
PSRAM low 30,460,512 B. Uptime 3 s: internal free 202,967 B, largest 69,632 B,
lifetime minimum 171,263 B, PSRAM free 18,167,508 B; identity stack watermark 5,920 B.
These are sampled free space/lifetime lows, not isolated operation peaks. Checked
UI frame subtotal remains 2,736 B; persistence snapshots remain 624,832 B PSRAM.

Final firmware CI: all five jobs pass, including host ASan/UBSan and four firmware
configurations: https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/40411959469.
Local 32/32 CTest, 24/24 Python, 3/3 checks, formatting/whitespace, recognition build,
Flash budget and seventeen-function ELF IRAM placement pass. Public final package:
`dist/0.11.7-dev-iteration13-final`, with source metadata, VALIDATION.md, SHA256SUMS
and a separately hashed ZIP. Backups/raw logs remain private and outside Git.

No outstanding installation authorization: the owner authorized these operations.
Visual touchscreen/flicker acceptance, actual restored-member greeting/HA delivery,
and real microphone/speech/model/calibration acceptance remain physical checks.
Speaker weights are provisioned and numeric readiness/failure cleanup reporting is
implemented; this is not a measured successful speaker inference. Voice/BOTH events
remain gated. Next milestone: one combined debugger-free visual/greeting/HA and
bounded silence/speech readiness session before considering 0.12.0-dev. PR stays
draft/unmerged; this is a reproducible development candidate, not product release.

## Verified 0.11.6 hardware transactions and final 0.11.7 UI guard

0.11.6/b7f5da1 was installed app-only with verify and full Flash readback; all other
sectors matched the fresh private backup. All five CI jobs passed (40386086581).
A captured disable/enable pair committed generations 5/6, parked/resumed the same
model set and survived a normal restart restoring generation 6 with ready AUTO.

Three further automated hardware worker cycles produced six verified commits:
generations 7,8,9,10,11,12. Both harts were briefly halted only to inject each
command into the checked empty/unlocked existing queue and to inspect results;
execution used the real identity worker, production transaction engine and active
CSI/DSI. Boot ID stayed constant; names/IDs/sample counts stayed identical and
model loads/releases remained 1/0. This proves the backend risky path without a
new manual reproduction, not touchscreen/visible flicker or normal-watchdog proof
while the debugger was attached.

After debugger shutdown, a private read of the entire profiles partition verifies
valid generations 12 and 11. The latest 312,416-byte canonical payload is byte-for-
byte identical to generation 4 before the tests, proving names, IDs and both
modalities' templates preserved, beyond just matching counts. A debugger-free
normal restart then restores generation 12 and ready AUTO with continuing CSI
frames. No recovery erase or re-enrollment.

Measured transaction times include 5,030,632 and 6,041,538 us. The final UI guard
in 0.11.7-dev does not report a create timeout while storage_saving is true, even
past 5 seconds; actual creation receipts are still handled first. Waiting without
an active save retains the old timeout. Tests cover 5,001/5,031/6,042/30,000 ms
and prevent false failure during a legitimate durable commit. The worker/storage/
IRQ implementation remains that of the verified 0.11.6 fix. Final-source install
and target receipts are recorded when obtained, without inferring acceptance.

## 0.11.5 failure and measured 0.11.6-dev correction

The owner confirmed the dark frame metrics were caused by darkness/device
orientation and that the camera itself operates. However, disabling a profile
reproduced a restart. A live capture confirms HP_SYS_HP_WDT_RESET, with previous
committed generation 4 restored. This supersedes any acceptance implication from
the earlier idle boot/window. Do not run additional manual tests on 0.11.5.

USB JTAG/OpenOCD and the exact installed ELF captured the original exception.
With both harts halted and the existing polling command queue empty/unlocked,
a synthetic disable command plus the normal epoch change was injected into that
queue. It exercises the real worker/Flash/CSI transaction, not the touchscreen.
At panic entry, the saved frame has mepc=0x402dd6be, ra=0x4ff01c7a, mcause=0x19,
hart=0. Address resolution identifies `dw_gdma_channel_config_transfer` called by
`csi_dma_trans_done_callback` at the native CSI transfer restart. SDK 5.5.1 leaves
this function in Flash despite its other IRAM control options. The secondary
watchdog occurs while the cache error is being handled. No model threshold,
partition or stored templates are implicated by this frame.

0.11.6-dev adds a project-local linker fragment for only that missing noflash
mapping, without editing the installed SDK. The ELF guard now checks seventeen
entrypoints/callees, including DW-GDMA control/list helpers, cache synchronization
and the timer. The 0.11.5 binary is rejected at the exact Flash-resident culprit.
Hardware transaction tests under the debugger are diagnostic evidence; OpenOCD
changes watchdog behavior while halted, so final normal-mode/touchscreen/flicker
acceptance still requires a debugger-free session. Source/CI/package/install and
post-fix results are recorded below once actually obtained.

## Historical 0.11.5-dev installation and boot (subsequently withdrawn)

Following the existing owner authorization, installed only the app at 0x20000:
4,505,248 B, SHA256 ccd13d196db6410d4617f0c1d16e951fd36358149cb1aab9c79e0051d984f8e8.
A fresh private 16 MiB backup preceded the update. App verification and full Flash
readback pass; every other sector is unchanged, including all profiles, NVS,
partition table and both models. No recovery or new enrollment was performed.
Public package: dist/0.11.5-dev-iteration13-csi-cache, 54 verified hashes and ZIP.
All five CI jobs pass for firmware source 311dbe1:
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/40374472427.
Local: 32/32 CTest, 24/24 Python, 3/3 checks, sanitizers, target configuration,
eight-function linked ISR placement, Flash and unchanged 2,736 B UI frame subtotal.

Actual 0.11.5 boot confirms version, LVGL, profile bank A generation 4, durable clean
status and automatic ready AUTO (models 1/0). Restore took 164,230 us. CSI controller
and application callbacks register successfully and real frame completion counters
advance at 29.6-29.7 FPS. The first 55-second window has no panic, watchdog, callback
registration error or display underrun. Heap samples at uptime 3 s: internal free
203,423 B, internal lifetime minimum 171,895 B, PSRAM free 18,167,508 B. These are
sampled free space/lifetime minima, not isolated operation peaks.

Frames currently measure below-range luminance 0.0/rejected fraction 1.00; this is
not proof of usable scene pixels. Operator lighting/lens context or a preview is
needed before camera-image/recognition acceptance. Actual disable/enable after
this fix, absence of visible flicker/MBF error, post-change restart, greeting/HA
and microphone/model inference remain pending operator interaction. Do not infer
these from frame rate or ready status. 0.11.4-dev was withdrawn after its CSI boot
failure; its earlier all-green CI is not hardware acceptance.

## Earlier authorized 0.11.3-dev installation

The owner subsequently authorized installation and hardware testing. The exact
0.11.3-dev candidate (8499ffc) and pinned speaker model were installed on COM5.
A private 16 MiB backup was captured outside the repository before any write.
Disabled Secure Boot/Flash Encryption and the unchanged installed table were
verified. The backup contained valid schema-2 banks A generation 2 and B generation 1;
no recovery erase was necessary or performed.

Writes were limited to application 0x20000 (4,503,568 B) and speaker model 0xc20000
(2,844,624 B). esptool verification and full 16 MiB readback passed. Every sector
outside the two selected write/erase ranges was byte-identical to the private
backup, including the entire profiles partition, NVS, face model and partition table.
Application SHA256: 86eabffc2384fbc571b9eaf9eb28c1713bc280ef54c91eac3b858e6e442d05c7.
Speaker SHA256: b503ff049a0cd5c62b4c1ac337cf20b7de1549857a4d5329a1821cfb55e9b3a8.

Actual boot log confirms Home Buddy 0.11.3-dev / IDF v5.5.1, LVGL ready,
PROFILE_BOOT result=0 bank=0 generation=2, PROFILE_STATUS ready=1 saved=1 dirty=0,
and ready AUTO (HEALTH mode=4 ready=1) without manual activation. Restoration took
170,369 us. The first 55-second boot capture has no panic/watchdog/brownout marker.
This proves this boot's restoration/AUTO readiness, not recognition/greeting/HA or
new-transaction acceptance. Touch changes, a post-change restart and microphone/
real model inference remain pending operator interaction. Prior owner-confirmed
retention/AUTO on 0.11.2-dev remains the baseline, superseded only where tested here.

## Profile toggle hardware defect and 0.11.4-dev correction

The operator's first disable/enable test on 0.11.3-dev produced display flicker
and a failed MBF restart. Passive capture confirms both profile commits succeeded:
PROFILE_STATUS result=0 ready=1 saved=1 dirty=0 bank=0 generation=4. AUTO remained
selected but not ready (models=1/1), so persistence acceptance must be separated
from failed recognition availability. No post-change restart or voice test was
claimed; the failing runtime was captured before any further device reset.

The observed internal free heap was 249,939 B but its largest block only 59,392 B,
below the unchanged 65,536-byte cold-start admission. Disabling the last enabled
member had unnecessarily destroyed the already loaded model set. The new policy
parks that same bounded set only for a member-disable pause with AUTO still chosen.
Disabled people cannot produce inference, confirmation, greeting or HA identity.
Re-enabling reuses the existing set; no additional model allocations or relaxed
admission thresholds. Explicit Stop/cancel, deletion and foreground mode changes
clear the administrative pause and retain normal resource release. A genuine cold
start can still fail safely under fragmented memory; this is not a general heap
compactor or a promise that Stop/reload succeeds under arbitrary memory pressure.

The pinned IDF 5.5.1 DSI callback must restart frame DMA on each completion. With
LCD_DSI_ISR_IRAM_SAFE disabled, Flash operations defer that handler. Enable the
supported setting (which selects DW_GDMA_ISR_IRAM_SAFE) and verify effective
configuration in every build profile. The board refresh callback was already IRAM
and uses an internal semaphore. This targets the Flash-associated flicker; physical
confirmation on the same toggle remains required, not inferred from configuration.
Menu/system firmware labels now use the real application descriptor instead of
stale hardcoded versions. A 100-cycle policy regression covers park/resume without
reallocation, explicit release, disabled cold state and foreground enrollment.

## Shared DW-GDMA follow-up, 0.11.5-dev

0.11.4-dev (e9a2f6c) passed all five CI jobs and was installed application-only with
verified readback; all other sectors, including profiles generation 4, were unchanged.
The automatic boot check then caught a CSI regression before an operator retest:
DW-GDMA rejected the camera controller's external-RAM user context. DSI's global
IRAM-safe DMA selection applies to CSI as well. Model-ready AUTO without camera
frames is NOT recognition acceptance. No successful toggle/flicker result is claimed
for 0.11.4-dev; its runtime is superseded by the following correction.

Enable CAM_CTLR_MIPI_CSI_ISR_CACHE_SAFE with DSI IRAM safety, selecting the pinned
DW-GDMA ISR/control/getter/setter IRAM modes and the CSI controller's internal-RAM
allocation. Move the two application callbacks and their bounded frame-pool
reserve/complete/abort/publish callees into IRAM. ISR state stays in static internal
RAM; callbacks never read biometric/PSRAM payloads, allocate, log or call LVGL.
Time division uses the existing ROM libgcc helpers. A new post-link ELF guard
checks the actual eight camera/display entrypoints and frame-pool callees; missing
or Flash-resident functions fail the build. Effective configuration checks cover
the shared DMA contract. Host frame-pool lifetime/lease behavior is unchanged.
The target boot must confirm actual CSI frames as well as profile/AUTO restoration.

## Storage initialization and first save

The production boot engine verifies both entire banks. Only a completely erased
1 MiB partition receives an empty schema-2 generation in upper bank B. Enrollment
then saves to A, retaining that committed empty fallback. No format, partition,
ABI, namespace, household capacity or persistence-policy change is introduced.
A valid pre-existing database restores without this initialization write.
Legacy migration retains both schema-1 banks until its schema-2 commit verifies.

Bootstrap is itself a transaction. If power fails during this empty generation,
there may still be no committed bank. Unknown/torn bytes remain preserved and
writes are blocked; this tradeoff cannot be eliminated by pretending an erased
header proves a blank bank. A failed first user save restores either the empty
fallback or the new valid generation, depending on whether its header committed.
Failed runtime saves retain usable RAM and a retry; durable UI success still waits
for header/payload readback. A commit whose final read fails can be found valid
on reboot; the failed attempt is never presented as confirmed durable in advance.

Flash erase and payload programming use bounded 4 KiB operations, with a scheduler
yield between device operations. Readback and final commit ordering stay unchanged.
The identity worker owns the full transaction and shared compute admission covers
boot, fingerprinting and saves. LVGL callbacks do not perform database writes.
Yields reduce continuous monopolization; they do not prove watchdog or physical
power-failure acceptance. No cancellation can delete the owning worker mid-save.
Canonical fingerprints still exclude recognition timestamps/counters.

## Transaction evidence

Tests exercise production `profile_store_v2.c`, `profile_store.c` and the real Flash
adapter with synthetic records, never household biometrics. Empty bootstrap injects
four cut lengths at all 206 mutation boundaries plus blank scan, payload readback
and final verification failures (836 cases). First user save injects four cuts at
all 209 I/O operations (836 cases) and proves the empty fallback remains unchanged.
Existing database faults cover all 209 I/O operations with eleven cut lengths
(2,299 cases), asserting the active bank is byte-identical and restored names and
IDs match the selected generation. Migration, full capacity, both modalities,
rename/disable, corrupt/unknown banks, locked policy, deletion and scrub remain
covered. The historical 14,368-byte headerless payload failure remains reproduced.
Tests use a deterministic protocol checksum; target firmware uses mbedTLS SHA256.

## Recovery and USB

Recovery defaults to offline inspection and checks backup SHA256, package/table,
exact offsets, entire current partition, security state, approved scope and blank
readback. It rejects ambiguous/duplicate security fields and protected devices.
All USB commands now leave the board in the loader (`--after no_reset`), preventing
application writes between the comparison and erase/readback. Execution leaves the
loader active; a later application start/update is a separate authorized step.
Tests cover checksum/policy/layout/security failure, changed bytes, failed reads,
erase/readback failure and exact 0xba0000/0x80000 erase scope. The private backup
is never overwritten. This old interrupted-first-save recovery is inapplicable
to the currently valid household database.

Application-only USB plans still write only 0x20000, preserving NVS, profiles,
face/speaker models, settings and C6 firmware. Full recovery remains a separate
potentially destructive operation described in USB_FLASHING.md; never default to it.

## UI, restoration and voice

Four product sections, eleven advanced diagnostic entries, static UI status
workspace, touch geometry and target stack guard remain. Save failure no longer
shows a raw result number on the ordinary household screen. Numeric result stays
in advanced diagnostics/logs. Storage damage reports that profile memory needs
repair. Restore precedes AUTO preference processing, independently of Wi-Fi/HA;
model initialization retains its three bounded attempts and preserves templates.
No new GUI layout or extra ordinary navigation confirmation is added.

The existing explicit voice diagnostic now exposes one numeric readiness report:
selected I2S slot, observed rate, accepted sample count, RMS/peak/clipping/suspicious
signal flag, valid normalized 512D output, model load/preprocessing/inference times,
sampled heap lows, and callback/PCM cleanup. It logs no PCM or embedding values.
The old inferred model load/release counters were removed: RAII adapter cleanup
has host fault tests, not a fabricated physical resource-release measurement.
Report validity is tested with success, invalid output and error/cancel paths.
A successful embedding is not proof of speech selection or speaker accuracy.
Voice enrollment/identity calibration gates and FACE-only HA policy stay intact.

## Memory and validation receipt

Exact unchanged snapshot allocations: 2 x 312,416 = 624,832 bytes PSRAM. No extra
persistence buffer or snapshot-sized LVGL object. Voice adds a small fixed numeric
report; its 96,480-byte PCM window remains temporary and scrubbed before release.
Physical boot telemetry: storage-stage lifetime internal low watermark 287,143 B
and PSRAM low watermark 30,460,096 B; at uptime 3 s, internal free 204,667 B,
internal minimum 173,719 B, PSRAM free 18,166,888 B. Identity stack watermark 5,936 B.
These are sampled free space/global lifetime lows, not isolated operation peaks.
Initial restore took 170,369 us. Save/inference latency and longer watchdog behavior
remain pending; no measured microphone/model resource result is claimed yet.

Local validation: 32/32 CTest (including LVGL navigation and visual baselines),
21/21 Python regressions, 3/3 project checks, complete ASan/UBSan script, formatting,
whitespace, effective recognition configuration and enforced Flash budget pass.
Target ESP-IDF 5.5.1 recognition build passes; checked UI frame subtotal is 2,736 B.
Application size: 4,503,568 bytes; 2,836,464 bytes headroom in the 7 MiB partition.
The earlier Windows build invocation hit console encoding U+2265; rerunning with
PYTHONUTF8=1 succeeded. It was a host build-environment error, not a target fault.
Full firmware matrix and final source-bound package receipt are recorded in the
package VALIDATION.md after GitHub Actions completes. No CI success is inferred
from these local checks. Host tests cannot prove complete ESP-IDF boot under
network/model outages or every device-specific LVGL page and touch transition.

## Next milestone and one acceptance session

Application and speaker installation are authorized, completed and verified.
Next: one durable intentional change/restart/restore plus greeting and eligible HA event. Do not delete/re-enroll the current member or run obsolete recovery.
The pinned speaker model is now provisioned under the same approval. Collect
silence/speech/slot response and one readiness report in the same session. 0.12.0-dev remains conditional on target persistence
validation and measured microphone/model readiness, with calibration still required
before public speaker identification. No production release or PR merge.
