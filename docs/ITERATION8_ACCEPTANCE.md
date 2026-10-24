# Iteration 8 physical acceptance protocol

0.7.0-dev candidate is not installed. Preserve installed one-person RAM enrollment
and prior receipts. Application-only USB update resets RAM: schedule its loss and
consenting re-enrollment before installation. No C6, partition or security writes.

## Lifecycle

Record candidate hash/version/reset reason and passive USB timestamps, excluding
personal data/credentials. For background TEST record acceptance time and
SESSION_BACKGROUND session/until_ms, stop/expiry/model-release receipts. Return
to saver immediately, after 1 s and 20 s; open/close settings 20 times. Session and
deadline remain unchanged; inference/camera/UBox continue. Stop immediately and
during inference: cancelled results produce no later eligible greeting/event.
Already-dispatched HTTP cannot be recalled. TEST rejects inference completing at
or after ten-minute deadline and releases models under compute mutex. Repeated
starts are explicit new sessions; navigation never starts one. In an instrumented
build saturate the command queue: rejection must not grant saver ownership or
cancel the active epoch. Do not claim touchscreen/queue acceptance from pure tests.

Initially OFF computes no MBF embeddings. With usable enrollment explicitly select
AUTO, return to saver and stay >10 min: same session, resident model, no periodic
reload. Ordinary settings navigation is passive. Stop cancels immediately and
asynchronously saves OFF; failed save visibly warns AUTO could return on reboot.
AUTO with zero templates waits without loading. RAM-only reboot shows lost profiles
and waiting, never a restored identity. Enroll and explicitly select AUTO again.
Secure restored profiles may start AUTO at boot only with preference and usable
templates. Diagnostics/enrollment replace the session; resume AUTO explicitly.
TEST selection disables saved AUTO if NVS commit succeeds.

## Encounters and HA failures

Keep previous real REST/notification receipts as regression baselines. One eligible
event per person/encounter; none for UNKNOWN, AMBIGUOUS, unconfirmed or no-face.
Short detector dropout cannot reset an encounter. Genuine absence requires fresh
negatives through existing presence policy. Leave/re-enter with independent greeting
(5 min) and configured HA cooldowns. Changed person requires three fresh confirmations.

Test Wi-Fi loss, HA down/restart, invalid token (401/403), 4xx/5xx rejection, timeout,
then reconnect. Local greeting/recognition/UI continue; bounded ring may overflow;
old events never replay. Count harmless recognized-only notifications and HA traces
separately. Connection tests are diagnostic. Failed events drop; no durable retry,
at-least-once or exactly-once guarantee. Distinguish cancellation before send from
an already in-flight request.

## Local accuracy trials

Select expected person in Domownicy; diagnostic offers Wybrana osoba and Gość.
Start TEST, choose expected identity/guest, establish condition, tap Oceń próbę.
One explicit trial uses three NEW observations, not three independent trials.
No enrollment frame is evaluated; no guest is enrolled. Leave/reposition/change
condition between trials, rather than rapidly tapping correlated frames.

For each consenting identity and consenting unknown participant evaluate front,
left/right turn, tilt, shadow, low/bright light, glasses, near/far, partial occlusion,
two faces, covered camera and empty scene. Record condition, expected identity/guest,
trial count/outcome locally; never commit names, images, credentials or embeddings.
Without participants only synthetic policy tests are possible.

known_trials/unknown_trials include unavailable and ambiguous. Known outcomes:
TA, wrong registered ID, FR (non-ambiguous non-accept), known AMB, known unavailable.
Unknown outcomes: FA, CR (non-ambiguous reject), unknown AMB, unknown unavailable.
Use TA/known_trials, FR/known_trials, confusion/known_trials, FA/unknown_trials and
CR/unknown_trials; disclose abstentions, counts and conditions. Never pool into an
unexplained accuracy percentage. Raw counters remain local. Latency arrays collect
final-observation total_us, not encounter-to-notification time; D/L/MBF remains
visible. Physical distributions have not been collected.

## Persistence, audio and soak

Follow SECURITY_PROVISIONING.md gates and synthetic torn-write tests before real
persistent enrollment. AUDIO_DIAGNOSTICS.md defines microphone acceptance. Speaker
benchmark needs working microphone and compatible model, with no voice HA event.

Run uninterrupted 72 h on actual candidate: AUTO, regular UBox/weather/HA, daily
encounters. Record hash, start/end uptime, resets, periodic free/largest/minimum
PSRAM/internal RAM, stack, model cycles, camera drops, queue overflow/staleness,
network errors, watchdogs and brightness. Include menus/enrollment, full-size JPEG,
temporary camera/model failure, Wi-Fi/HA recovery and manual-brightness expiry.
A reset or capture gap restarts the uninterrupted timer. No host test or historical
session is a completed soak.
