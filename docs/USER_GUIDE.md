# Development firmware 0.3.0-dev user guide

## Profile storage in 0.11.3-dev

Profiles and the previously enabled AUTO preference restore locally after restart,
without HA, password or PIN. Wait for confirmed saved status before restarting.
A save error leaves temporary RAM changes for retry; it is not a durable receipt.
If profile memory requires repair, preserve it for diagnosis, never format it.
On genuinely blank storage the device establishes an empty fallback automatically.
Existing profiles do not need re-enrollment for an application-only update.


## Current iteration 12 policy (0.11.0-dev)

The selected product mode is **persistent, unencrypted, local household storage with
automatic restoration**. The recognition build runs the existing schema-2 engine in
two 512 KiB banks inside the unchanged 1 MiB partition. Intentional household changes
save automatically; valid face/voice profiles and the previously enabled AUTO mode
restore locally at startup. No password, PIN, manual unlock, eFuse provisioning or
mandatory encryption. Physical extraction is not prevented. Experimental voice
identification remains gated. Implementation/host tests do not establish physical
reboot persistence; installation and the combined acceptance session remain pending.
See [storage decision](PERSISTENCE_DECISION.md), [database](PROFILE_DATABASE.md) and
[iteration report](ITERATION12_REPORT.md). Older iteration notes below are historical.


Tap the saver to open settings. All pages open directly, without administrator
PIN, as requested. Thirty idle seconds return to the saver; pending saves are
exempt. Touch diagnostics remain open until Return. Brightness has a 26/255 floor
and has a separate persistent configuration; see BRIGHTNESS.md.

1. Wi-Fi: Scan, choose SSID, enter the network password and Save. The network
   password stays masked. Wi-Fi obtains DHCP IP; System shows IP/RSSI.
2. Kamera UBox: enter HA host/IP without URL/path, port (usually8100), token and
   poll seconds. The token is intentionally VISIBLE in this field to make long
   input checkable. It is not shown in System or serial logs. Save starts original
   cached-image polling. No new LTE capture is requested.
3. Pogoda: enter a locality name, tap Szukaj, choose the correct country/region
   result from the dropdown, then Zapisz. ASCII spelling is accepted by geocoding;
   you need not type Polish accents. No default location is assumed. Empty the
   city field and Save to disable weather. Weather requires no MQTT/broker setup. Identity HTTP has a separate Home Assistant menu; see HOME_ASSISTANT_HTTP.md.
4. The device requests Open-Meteo current temperature at2m every10minutes over
   verified HTTPS. These are weather-model data, not a local sensor measurement.
   Data older than1hour become unavailable. Source attribution appears in Pogoda.
5. Internet SNTP establishes time before HTTPS weather searches/data display.
   The clock follows Warsaw CET/CEST. Missing/stale time/data are never fabricated.
6. Test JPEG is an explicit synthetic reference, Test greeting displays Test UI,
   and Test touch checks corner/center/drag coordinates. System reports per-service
   state, heap/largest blocks and time since an image was accepted, not capture time.

Wi-Fi, UBox image, corrected image colors, touch, greeting, brightness and local
clock were physically confirmed by the operator. Weather search/selection/temperature display was also confirmed. Reconnect/cold-boot/fault/soak acceptance is separate.
NVS is not encrypted at rest; private flash backups may contain credentials. No
factory-reset, enrollment, local capture, biometric inference or OTA is exposed.

7. Kamera lokalna: turn preview on/off explicitly. It uses only the onboard camera;
   The portrait thumbnail rotates the fixed module mounting90degrees left. Test
   kolorów shows a synthetic sensor pattern and suspends light measurement.
   Wygaszacz UBox returns to the saved remote image. Capture FPS and visible preview
   FPS are different. No camera images are saved or uploaded.
8. Wyświetlacz: AUTO/MANUAL/TYMCZASOWO, day/night calibration, fallback and response
   settings. [Detailed workflow](BRIGHTNESS.md). Missing calibration uses fallback;
   missing camera data after calibration holds the last safe level.

Temporary brightness (0.3.1-dev): explicitly select TYMCZASOWO to begin five
minutes. Editing sliders/options or saving does not extend that interval. After
expiry the service/form returns to AUTO; only pressing TYMCZASOWO again starts a
new interval. A later ordinary slider adjustment selects MANUAL. Controls ignore
new edits while the save worker is committing the displayed configuration.

Night calibration in0.3.3-dev: leave the camera uncovered in normal night lighting,
wait for ciemno (prog) or a valid numeric reading, select Kalibruj noc, move the
slider to a comfortable level and Zapisz. Do not shine a flashlight to manufacture
the night point. Negative light values are normal relative logarithmic values.
Brak aktualnego pomiaru means no usable fresh sample; it no longer displays0.000.

Local face test (0.4.2-dev): open Kamera lokalna, provide normal good lighting,
press Test twarzy and face the onboard camera. The button displays count/time;
green boxes are diagnostic detections. Press again or leave the page to stop.
The menu still times out after30s without touch. No identity is assigned and no
photo is saved/sent. AI: brak pamieci means admission refused; normal camera/UBox
remain available. Restore normal lighting after testing; no flashlight needed.


0.4.3 presence test: enable Test twarzy and remain facing the camera until
Obecnosc: TAK (two consecutive results). A brief missed face should retain TAK.
Move fully outside the camera view for more than5s of continued analysis: after
at least four negative results expect NIE. Return and expect TAK after two
positive analyses. If analysis becomes unavailable for more than3s, expect
nieustalona rather than NIE. Closing the page ends the diagnostic; reentry starts
without saved presence. The30s menu inactivity timeout remains; touch an unused
area to keep the page open during a longer test. No HA action is triggered.


0.4.4 background trial: open Obecnosc w tle -> Uruchom10minut, then Wygaszacz UBox.
Return to the same page to inspect countdown and presence. Zatrzymaj ends the
background trial; simply returning to the menu does not. The trial expires after
10min and is off after reboot. A new explicit start renews10min; page visits do not.
A concurrently enabled foreground Test twarzy can continue after the lease ends,
and closing that page keeps a still-live background trial. Test kolorow cancels
both. No local image, greeting or HA action is added to the saver.


0.4.5 quality trial: open Jakosc twarzy -> Uruchom pomiar and face the camera.
The largest detected face supplies dimensions, brightness, contrast and detail.
Move closer to increase the face's input size; change actual illumination to
observe processed-face brightness. Hints are provisional and do not reject a face.
Detail also changes with beard/glasses/noise/scale, so it is not a focus diagnosis.
Without a fresh face the page shows unavailable/wait text. Stop or leave to end
foreground measurement. An active10min background lease still follows its deadline.


## 0.5.0-dev local identity experiment

Current 0.6.1-dev workflow: Domownicy -> enter a name -> keyboard checkmark ->
Potwierdź dodanie -> Odśwież listę -> select the profile -> Rejestruj próbki.
Collect eight accepted samples with small pose/distance changes and 2-3 second
spacing. Wait three seconds, face the camera, Weryfikuj, then Potwierdź zapis.
Refresh the household list, select the profile and open Test rozpoznawania.
KNOWN plus potwierdzony requires fresh-frame confirmations. No profile survives
a reboot on the current unprovisioned RAM-only device.

Foreground testing does not publish HA identity events. For the explicit
background trial on the installed household-keyboard firmware, press Włącz tło
na 10 min, wait twenty seconds for the queued mode change, then Wygaszacz UBox.
The prepared navigation-fix candidate instead shows “Tło przyjęte. Możesz
otworzyć wygaszacz.” after queue acceptance, and permits immediate saver entry.
That candidate is not yet installed or physically accepted. Queue rejection
still displays an error; Stop and Test revoke background navigation intent.
The operator has accepted the personalized
greeting and a real recognized/face/confirmed HA event. Greeting cooldown is five
minutes; HA cooldown is configured separately (default 60 seconds). To validate
another encounter, leave the camera view for 15 seconds and return after the HA
cooldown has elapsed. Inspect all received events: the last event after leaving
can be presence_leave rather than the earlier recognized event.

See [face recognition implementation and limitations](FACE_RECOGNITION.md),
[onboard Flash layout and migration proposal](IDENTITY_FLASH_LAYOUT.md), and
[actual build/test versus pending hardware acceptance](IDENTITY_VALIDATION.md).
The experimental MBF worker remains opt-in. The 0.6.1-dev candidate uses onboard
Flash for model storage, gated encrypted profile persistence and native identity
HTTP. Iteration 12 selects LOCAL_UNENCRYPTED independently of hardware provisioning. See ITERATION7_REPORT.md and
FLASH_MIGRATION.md before installation. UBox/Open-Meteo/clock and camera leases remain.

## 0.8.0-dev product navigation

Touch the photo to open Domownicy / Rozpoznawanie / Ustawienia / Informacje.
Wróć do zdjęcia returns to UBox. Ordinary inactive menus close after 30 seconds;
a visible keyboard, enrollment, deletion confirmation or pending save protects
the current operation. Settings group Wi-Fi (including scan), UBox, Open-Meteo,
display and Home Assistant. Wstecz returns to the containing section.

Domownicy lists real stable-ID profiles. Add a name, choose Dalej, keep one face
in the preview and vary head position gently. The worker collects eight valid,
nonduplicate samples at least one second apart and verifies a fresh frame.
Potwierdź zapis profilu performs the only biometric commit. The list refreshes
automatically. A repeat enrollment stages fresh templates while preserving old
committed templates until success. Rename and confirmed deletion are on the
person page. Profiles remain temporary on the installed unprovisioned unit.

Rozpoznawanie has explicit automatic on/off and a direct household link. It does
not start biometric recognition merely by opening the page. AUTO has no ten-minute
limit; the diagnostic TEST keeps that bound. Advanced tools, including microphone,
recognition evaluation, JPEG/touch/camera and HA connection tests, are under
Informacje > Diagnostyka zaawansowana. Microphone Start is always explicit.


## Iteration 10, 2026-10-25

Domownicy/Rozpoznawanie/Ustawienia/Informacje stay the four main sections. Informacje > Diagnostyka zaawansowana > Mikrofon contains normal 60-second diagnostics and explicit experimental Model 3 s L/R tests. First validate silence/speech and the usable slot. Model tests need an intentionally provisioned optional model and never identify/enroll a household member. Stop/back cancels the test. Real voice registration remains unavailable.


## Iteration 11, 2026-10-25

Domownicy > selected person shows a disabled voice-enrollment entry while microphone/model/speech-selection/resource/calibration acceptance is pending. Recognition has a separate disabled voice control. The prepared enrollment page collects four distinct normal utterances, verifies with a fifth, and requires explicit RAM-save confirmation; it does not recognize words or require a password. It is unavailable in the delivered candidate. The four main categories and UBox screensaver remain. Restart still loses profiles. See ITERATION11_ACCEPTANCE.md.

## Persistent household profiles

Before the first enrollment, read and acknowledge the one-time notice that face and
voice profiles are stored locally without encryption. The acknowledgment is remembered
in ordinary configuration; no unlock is requested on reboot. Add/enroll/rename/remove
as usual. Wait for Profile zapisane na urządzeniu. If saving fails, the new data
remains in RAM: use Ponów zapis before switching off. Do not assume RAM-only changes
will survive power loss. Recognition resumes only if AUTO was previously enabled
and valid enabled face profiles exist. Voice stays unavailable until separately accepted.
Advanced bank/generation/status information is under Informacje -> Diagnostyka
zaawansowana -> Pamięć profili; it contains no embeddings.
