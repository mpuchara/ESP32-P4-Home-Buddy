> Historical original specification. Its MQTT, microSD and OTA proposals are superseded by the owner's 0.6.1-dev continuation: SINGLE_IMAGE_FLASH_LAYOUT.md, HOME_ASSISTANT_HTTP.md and USB_FLASHING.md. Original text below is retained for traceability, not current requirements.

> Additional operator override: temperature uses Open-Meteo with manually selected
> locality; MQTT temperature was removed. UBox token is visible in its edit field.

> Current implementation override, 2026-10-22: the operator explicitly requested
> all Wi-Fi/UBox/MQTT settings without an administrator PIN. The original prompt
> below is preserved as received; current behavior and validation are recorded in
> USER_GUIDE.md and DEVELOPMENT_LOG.md. No biometric workflows are implemented.

# ESP32-P4 Home Buddy

## Master Development Prompt — Full Project Specification

### ROLE

You are a senior embedded software engineer and architect specializing in:

* ESP32-P4 and ESP32-C6
* ESP-IDF and FreeRTOS
* LVGL embedded graphical interfaces
* Edge AI and TinyML
* On-device face recognition
* On-device speaker recognition
* MIPI-CSI cameras
* MIPI-DSI displays
* Home Assistant and MQTT
* Embedded networking, security and OTA updates

Your task is to design, implement, test, document and progressively deliver a complete embedded product called **ESP32-P4 Home Buddy**.

This is not a proof of concept intended to run only once. The final result must be a stable, maintainable, standalone smart-home device suitable for continuous operation.

Development repository:

**GitHub: `mpuchara/ESP32-P4-Home-Buddy`**

The project starts from an almost empty repository containing an Apache 2.0 license.

Create an appropriate project structure, documentation, firmware, build configuration, tests and CI/CD pipelines.

**Do not modify `mpuchara/HA_UBOX_bridge`.**

That repository is an existing, independently maintained project and must only be used through its published HTTP API.

---

# 1. PRODUCT VISION

ESP32-P4 Home Buddy is a compact, intelligent, wall-mounted home assistant with a 4.3-inch touchscreen.

The device combines four principal functions:

1. An elegant, fullscreen Home Assistant camera snapshot dashboard.
2. Fully local facial recognition of household members.
3. Fully local voice-based identification of household members.
4. Native integration with Home Assistant for automations and status monitoring.

The entire identity recognition pipeline must operate directly on the ESP32-P4.

No external computer, Raspberry Pi, cloud API, remote inference service or Home Assistant processing may be required for facial or speaker identification.

Home Assistant acts only as:

* A source of camera snapshots.
* A source of outdoor temperature.
* An MQTT broker integration endpoint.
* A receiver of identity events and device telemetry.

All biometric templates, inference and enrollment must remain on the device.

The system must continue recognizing household members when disconnected from Home Assistant or the internet.

---

# 2. TARGET HARDWARE

Target device:

**GUITION JC4880P443C_I_W_Y**

SKU: `10153003`

Manufacturer: Shenzhen Jingcai Intelligent.

Initial hardware specification:

| Component                 | Specification                                |
| ------------------------- | -------------------------------------------- |
| Main MCU                  | ESP32-P4                                     |
| Wireless coprocessor      | ESP32-C6                                     |
| Display                   | 4.3-inch IPS                                 |
| Native display resolution | 480 × 800                                    |
| Application orientation   | Landscape 800 × 480                          |
| Display interface         | MIPI-DSI                                     |
| LCD controller            | ST7701S                                      |
| Touch controller          | GT911                                        |
| Camera                    | OV02C10 2 MP — verify exact installed sensor |
| Camera interface          | MIPI-CSI                                     |
| Audio                     | Integrated microphone and ES8311 audio codec |
| External RAM              | 32 MB PSRAM                                  |
| Flash                     | 16 MB                                        |
| Model and profile storage | 16 MB onboard Flash; no external storage      |
| USB                       | Two USB-C connectors                         |
| Network                   | Wi-Fi through ESP32-C6 / ESP-Hosted          |

These specifications are starting assumptions.

Before implementing hardware-dependent drivers, confirm them against:

* Actual connected hardware.
* Chip revision.
* Manufacturer documentation.
* Available schematics.
* Hardware initialization logs.
* Compatible driver implementations.

Do not assume that examples for the `JC4880P443C_I_W` without the `_Y` suffix are automatically compatible with every peripheral in our model.

### 2.1 Board support

Investigate and reuse relevant code from:

`ultramcu/guition-jc4880p4-bsp`

and:

`ultramcu/guition-jc4880p443c-i-w`

These projects document:

* ST7701S initialization.
* MIPI-DSI operation.
* GT911 touchscreen handling.
* Landscape framebuffer rotation.
* ESP32-C6 networking.
* Backlight PWM.
* Onboard memory initialization and verified Flash model loading.
* Hardware revision compatibility.

Prefer reusing reliable and properly licensed drivers rather than rewriting hardware support unnecessarily.

### 2.2 Important hardware considerations

The ESP32-P4 does not contain an integrated Wi-Fi radio.

Networking must use the onboard ESP32-C6 through ESP-Hosted over SDIO.

Verify host/slave firmware compatibility before attempting network operations.

Do not automatically overwrite the ESP32-C6 firmware.

If a coprocessor firmware update is required, provide a documented recovery-safe procedure.

Some ESP32-P4 boards contain engineering-sample silicon requiring specific toolchain configurations.

Check the actual chip revision before selecting build flags.

---

# 3. SOFTWARE ARCHITECTURE

Use **ESP-IDF as the primary framework**.

Use:

* ESP-IDF
* FreeRTOS
* LVGL
* ESP-DL
* ESP-WHO components where applicable
* ESP-Hosted networking
* ESP-MQTT
* ESP HTTP Client
* NVS
* SPIFFS or LittleFS where appropriate
* Espressif camera and audio drivers

Use stable, mutually compatible versions.

Pin dependencies in the repository.

Do not mix incompatible versions of ESP-IDF, Arduino, LVGL, ESP-DL or board support components.

If Arduino-based examples provide useful drivers, evaluate whether they can be adapted as ESP-IDF components.

Do not introduce an Arduino dependency solely for convenience if it creates integration problems with ESP-DL or camera drivers.

### 3.1 Proposed component structure

```text
ESP32-P4-Home-Buddy/
│
├── README.md
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── dependencies.lock
│
├── main/
│   ├── main.c
│   └── app_main.cpp
│
├── components/
│   │
│   ├── board/
│   │   ├── board_init
│   │   ├── display
│   │   ├── touch
│   │   ├── camera
│   │   ├── audio
│   │   └── backlight
│   │
│   ├── ui/
│   │   ├── ui_manager
│   │   ├── screensaver
│   │   ├── settings
│   │   ├── enrollment
│   │   ├── notifications
│   │   └── widgets
│   │
│   ├── identity/
│   │   ├── face_detection
│   │   ├── face_recognition
│   │   ├── speaker_recognition
│   │   ├── identity_fusion
│   │   └── identity_events
│   │
│   ├── profiles/
│   │   ├── profile_manager
│   │   ├── biometric_database
│   │   └── secure_storage
│   │
│   ├── connectivity/
│   │   ├── wifi_manager
│   │   ├── mqtt_client
│   │   ├── ha_discovery
│   │   ├── ha_entities
│   │   └── ubox_client
│   │
│   ├── services/
│   │   ├── time_service
│   │   ├── brightness_service
│   │   ├── configuration
│   │   ├── ota_manager
│   │   └── diagnostics
│   │
│   └── system/
│       ├── event_bus
│       ├── task_manager
│       ├── watchdog
│       └── memory_monitor
│
├── assets/
│   ├── fonts/
│   ├── icons/
│   └── images/
│
├── tests/
│
├── tools/
│
├── docs/
│   ├── ARCHITECTURE.md
│   ├── HARDWARE.md
│   ├── DEVELOPMENT_PLAN.md
│   ├── MQTT_API.md
│   ├── USER_GUIDE.md
│   ├── PRIVACY.md
│   └── TROUBLESHOOTING.md
│
└── .github/
    └── workflows/
```

This structure is a proposal, not a rigid requirement.

Adjust it to ESP-IDF conventions and the actual component boundaries.

---

# 4. GRAPHICAL USER INTERFACE

The entire graphical interface must use **LVGL**.

Target logical resolution:

**800 × 480 pixels, landscape.**

All screens must be optimized for a 4.3-inch capacitive touchscreen.

Use a modern, minimal, polished interface.

The UI language must initially be Polish.

Use UTF-8-compatible fonts containing Polish characters.

The interface should remain responsive during camera capture, AI inference, networking and storage operations.

Never execute long-running operations directly inside LVGL event callbacks.

---

# 5. DEFAULT SCREEN — FULLSCREEN CAMERA SCREENSAVER

The default screen is the central user-facing feature.

It must display an image received from the existing UBox Bridge Home Assistant integration.

### 5.1 Fullscreen requirements

The camera image must fill the entire display.

Resolution:

**800 × 480 pixels.**

The image must extend to all four physical display edges.

Do not add:

* Header.
* Footer.
* Status bar.
* Date.
* Title.
* Application name.
* Permanent buttons.
* Frames.
* Black borders.

Use a `cover` image-fitting algorithm.

Preserve the image aspect ratio.

Crop small portions of the image when necessary.

Never stretch or distort the image to change its aspect ratio.

The image should use as much available source detail as possible.

Use efficient JPEG decoding, preferably hardware acceleration where supported and appropriate.

Keep the last successfully decoded image visible while downloading or decoding a replacement.

Do not clear the screen before displaying a new frame.

Avoid visible flickering.

### 5.2 Camera image source

Use the existing repository:

`mpuchara/HA_UBOX_bridge`

Read its documentation and API implementation.

The current interface exposes cached snapshots through HTTP on port 8100.

Relevant endpoint:

`GET /snapshot.jpg`

Authentication:

`X-UBox-Token`

Use the original-resolution JPEG, not the 320 × 240 CYD variant.

Support HTTP caching through `ETag` and `If-None-Match`.

Handle HTTP 304 without downloading the image again.

Polling must not cause the LTE camera to wake up.

By default, query the existing cached image every 30 seconds.

Make the polling interval configurable.

Understand that UBox Bridge provides **cached snapshots, not continuous live video**.

Never display the image as though it were a real-time video feed.

Expose image age and connection diagnostics in the settings menu.

### 5.3 Clock overlay

Overlay the clock directly on the camera image.

Position:

**Bottom-left corner.**

Recommended appearance:

* Large white numerals.
* 24-hour format.
* Example: `09:41`.
* Minimalistic font.
* Soft shadow or subtle local dark gradient.
* Transparent background.
* No separate panel.

Synchronize time through NTP.

Timezone:

`Europe/Warsaw`

Correctly handle daylight-saving-time changes.

Update the clock once per minute without redrawing or downloading the camera image.

If time has not been synchronized, do not invent a valid-looking time.

### 5.4 Outdoor temperature overlay

Overlay outdoor temperature directly on the camera image.

Position:

**Bottom-right corner.**

Example:

`12,5°C`

Temperature must come from a configurable Home Assistant entity.

The user selects the source entity in the settings.

Preferred integration:

MQTT.

Because Home Assistant does not automatically publish all entity states to MQTT, provide a documented Home Assistant automation for publishing the selected temperature entity to a configurable MQTT topic.

Optionally support an authenticated Home Assistant REST API implementation as an alternative.

Do not require both methods simultaneously.

The preferred MQTT approach should avoid storing an unrestricted Home Assistant API token on the device.

Display the last valid temperature while temporarily disconnected, subject to a configurable freshness limit.

If no valid reading exists, show a neutral unavailable representation rather than a fabricated value.

### 5.5 Overlay implementation

Render the image, clock, temperature and notification as separate LVGL layers.

Example hierarchy:

```text
LVGL SCREEN
│
├── Fullscreen Camera Image
│   └── 800 × 480
│
├── Clock Overlay
│   └── Bottom Left
│
├── Temperature Overlay
│   └── Bottom Right
│
└── Notification Overlay
    └── Displayed Temporarily
```

Updating the clock must not redraw the source JPEG.

Updating the temperature must not download the source JPEG.

Animations must not modify the underlying JPEG buffer.

---

# 6. IDENTITY RECOGNITION NOTIFICATIONS

When a known household member is recognized, display a subtle notification directly over the screensaver.

Example:

**Cześć Maciej!**

The message must use the display name configured in the person's local profile.

### 6.1 Visual design

Create a translucent, rounded notification bubble.

Recommended characteristics:

* Centered horizontally.
* Positioned near the vertical center, away from clock and temperature.
* Dark translucent background.
* White text.
* Rounded corners.
* Soft shadow if performance allows.
* Smooth appearance and disappearance.

Animations:

* Fade-in: approximately 300 ms.
* Visible: approximately 4 seconds.
* Fade-out: approximately 500 ms.

The notification must never replace the fullscreen camera image.

### 6.2 Notification rules

Notifications must be generated by the local identity engine.

They must not depend on Home Assistant.

Implement configurable cooldowns.

Default greeting cooldown:

5 minutes per person.

Do not continuously repeat greetings while someone remains in view.

Do not greet unknown individuals by an invented identity.

If the face and voice indicate conflicting identities, suppress personalized greetings.

If a confident single-modality identification is available, a greeting may still be displayed.

The greeting cooldown and Home Assistant identity-event cooldown must be independent.

---

# 7. TOUCH INTERACTION AND SETTINGS

Touching the fullscreen screensaver opens the LVGL main menu.

Do not require a physical button.

The first touch opens the menu rather than triggering hidden controls on the camera image.

After 30 seconds of menu inactivity, automatically return to the fullscreen screensaver.

Make the timeout configurable.

Do not interrupt active biometric enrollment or critical configuration operations merely because an inactivity timer expires.

### 7.1 Main menu

Implement a modern tile-based menu.

Required sections:

**Domownicy**

* List registered people.
* Add person.
* Edit display name.
* Register face.
* Register voice.
* Improve existing profile.
* Delete profile.
* Show enrollment status.

**Rozpoznawanie**

* Face recognition enabled/disabled.
* Speaker recognition enabled/disabled.
* Live test mode.
* Detection diagnostics.
* Similarity scores.
* Recognition thresholds.
* Event cooldown configuration.

**Wi-Fi**

* Scan networks.
* Select SSID.
* Enter password using LVGL keyboard.
* Connect/disconnect.
* Show IP address and signal strength.
* Network diagnostics.

**Home Assistant**

* MQTT broker configuration.
* MQTT authentication.
* MQTT connection status.
* Temperature source configuration.
* MQTT Discovery status.
* Test event publishing.

**Kamera UBox**

* Home Assistant host address.
* UBox Bridge port.
* Access token.
* Image refresh interval.
* Snapshot preview.
* Last successful image timestamp.
* Connection test.

**Wyświetlacz**

* Automatic brightness.
* Manual brightness.
* Day calibration.
* Night calibration.
* Brightness diagnostics.
* Screensaver timeout.

**System**

* Firmware version.
* Chip revision.
* Memory usage.
* Uptime.
* CPU load.
* Model status.
* Storage usage.
* OTA update.
* Restart.
* Factory reset.

### 7.2 Security

Biometric enrollment, profile deletion and sensitive configuration changes must be protected by a local administrator PIN.

The PIN should not be hardcoded.

Do not expose the PIN or credentials in logs.

Provide a safe first-run setup procedure.

---

# 8. AUTOMATIC DISPLAY BRIGHTNESS

The display must automatically adjust its backlight to the ambient lighting conditions.

The entire algorithm must run locally on ESP32-P4.

### 8.1 Light sensor selection

First, confirm whether the exact board revision includes a dedicated ambient light sensor.

If present and functional, use it.

If no dedicated sensor exists, use the onboard MIPI-CSI camera.

Do not use the remote UBox snapshot as a brightness measurement source.

The local camera is the environmental light sensor.

### 8.2 Camera-based ambient brightness

Periodically estimate ambient illumination from camera measurements.

Suggested interval:

5–10 seconds.

Do not rely only on average JPEG brightness.

Account for:

* Automatic exposure.
* Analog gain.
* Exposure duration.
* Image statistics.
* Saturated pixels.
* Local shadows.
* Temporary occlusion by people.

Use camera exposure/gain metadata where available.

If necessary, use a controlled sampling mode that does not destabilize the face-recognition pipeline.

The objective is not necessarily to estimate calibrated lux.

A stable, repeatable relative illumination metric is sufficient for automatic brightness control.

### 8.3 Day/night calibration

Create two calibration workflows in LVGL.

**Kalibracja — dzień**

The user adjusts screen brightness using a slider while the system measures the current ambient light.

Save:

* Ambient light measurement.
* Selected backlight brightness.

**Kalibracja — noc**

Repeat the same procedure under normal nighttime lighting.

Save both calibration anchors locally.

The user must be able to recalibrate at any time.

### 8.4 Brightness algorithm

Interpolate between calibrated day and night brightness levels.

Apply:

* Low-pass filtering.
* Outlier rejection.
* Hysteresis.
* Configurable response speed.
* Gradual PWM transitions.
* Minimum and maximum brightness limits.

Target response time:

Approximately 20–60 seconds for normal lighting changes.

Prevent rapid brightness changes caused by:

* Someone walking past the camera.
* Brief shadows.
* A momentary flash.
* Small changes in image exposure.

Support a temporary manual override that automatically expires.

If the camera is temporarily unavailable, preserve the last valid brightness setting.

Do not cause the display to become permanently black due to a measurement error.

Verify the PWM pin and actual backlight control range against the hardware.

Use the ESP-IDF LEDC driver where compatible.

---

# 9. LOCAL FACE RECOGNITION

Implement fully local facial recognition using ESP-DL or compatible Espressif components.

Investigate:

* ESP-WHO
* ESP-DL
* Human face detection examples
* Human face recognition examples
* Available recognition databases

Prefer existing quantized models optimized for ESP32-P4.

### 9.1 Required pipeline

```text
LOCAL CAMERA
    │
    ▼
FRAME CAPTURE
    │
    ▼
FACE DETECTION
    │
    ▼
FACE ALIGNMENT
    │
    ▼
FEATURE EXTRACTION
    │
    ▼
LOCAL FACE DATABASE
    │
    ▼
IDENTITY MATCHING
    │
    ▼
IDENTITY EVENT
```

### 9.2 On-device enrollment

Users must register their faces directly through the device touchscreen.

No external training scripts.

No PC-based enrollment.

No cloud processing.

Enrollment workflow:

1. Open Domownicy.
2. Select Dodaj domownika.
3. Enter a display name.
4. Select Rejestruj twarz.
5. Show camera preview.
6. Guide the person through several poses.
7. Capture multiple good-quality face samples.
8. Extract embeddings locally.
9. Save the profile locally.
10. Verify recognition against new camera frames.

Recommended enrollment poses:

* Looking straight ahead.
* Slightly turned left.
* Slightly turned right.
* Different facial expressions.
* More than one lighting condition where feasible.

Reject unsuitable samples.

Do not save blurred or incorrectly detected faces.

### 9.3 Recognition behavior

Support multiple registered household members.

The system should distinguish:

* Known person.
* Unknown person.
* No face.
* Ambiguous identification.
* Recognition temporarily unavailable.

Apply confidence thresholds and temporal stabilization.

A single low-quality frame must not trigger a confirmed identity.

Use repeated consistent detections when appropriate.

Make thresholds configurable through advanced settings.

Do not interpret cosine similarity directly as a percentage probability of correctness.

### 9.4 Updating an existing profile

Allow users to improve recognition by adding new samples to an existing profile.

Do not automatically enroll every face seen by the camera.

Additional enrollment must require an explicit, authenticated action.

### 9.5 Privacy

Prefer storing feature embeddings rather than full facial photographs.

Do not retain raw frames by default.

Make deleting a person remove their stored biometric templates.

Do not upload embeddings to Home Assistant.

---

# 10. LOCAL SPEAKER RECOGNITION

Implement local identification of household members by voice.

This means **speaker identification**, not speech transcription.

The device does not need to understand what someone says.

It must attempt to identify who is speaking.

Use an available ESP-DL speaker-verification model supporting ESP32-P4.

Investigate Espressif's `speaker_verification` example and its enrollment database.

### 10.1 Audio pipeline

```text
MICROPHONE
    │
    ▼
AUDIO CAPTURE
    │
    ▼
VOICE ACTIVITY DETECTION
    │
    ▼
AUDIO PREPROCESSING
    │
    ▼
SPEAKER FEATURE EXTRACTION
    │
    ▼
LOCAL SPEAKER DATABASE
    │
    ▼
SPEAKER MATCHING
    │
    ▼
IDENTITY EVENT
```

Use an appropriate supported sample rate, initially targeting 16 kHz mono PCM where required by the model.

Verify the exact microphone-to-codec connection on this board.

Do not assume that an example using a different ESP32-P4 board has identical audio routing.

### 10.2 On-device voice enrollment

Voice enrollment must be completed entirely on the device.

Proposed workflow:

1. Select a household member.
2. Select Rejestruj głos.
3. Show a recording interface.
4. Ask the person to speak naturally.
5. Capture several voice samples.
6. Validate sample duration and speech activity.
7. Generate speaker embeddings locally.
8. Save the embeddings.
9. Run a verification test.

Allow several samples in different acoustic conditions.

Use approximately 3–6-second useful speech windows, subject to the selected model's actual requirements.

Do not require fixed phrases unless necessary for a particular implementation.

Do not require speech recognition.

### 10.3 Speaker identification

Support open-set identification.

Compare an incoming embedding against enrolled speakers.

Return:

* Known speaker.
* Unknown speaker.
* Ambiguous result.
* Insufficient speech.
* Recognition error.

Do not force the closest match to be a known household member.

Unknown speakers must remain unknown when similarity thresholds are not satisfied.

### 10.4 Audio privacy

Do not save continuous audio recordings.

Do not transmit microphone audio to HA.

Do not use cloud speech APIs.

Raw enrollment audio should be discarded after extracting suitable templates unless explicitly required for a user-approved diagnostic function.

Speaker models must run locally on ESP32-P4.

---

# 11. IDENTITY FUSION ENGINE

Create a dedicated identity-fusion module that combines facial and speaker recognition.

Both recognition systems must operate independently.

### 11.1 Examples

**Face recognized, no speech**

Return the facial identity if the recognition result is sufficiently reliable.

**Voice recognized, no face**

Return the speaker identity if the result satisfies its threshold.

**Face and voice agree**

Return the identity with status `confirmed`.

**Face and voice disagree**

Return `conflict`.

Do not silently select one identity.

**Unknown face, known voice**

Preserve the information from both modalities.

Do not automatically assume that the voice belongs to the person visible in the image.

**Several people in view**

Track faces independently where feasible.

Do not associate a voice with a particular face without sufficient evidence.

### 11.2 Event stability

Implement:

* Temporal smoothing.
* Duplicate suppression.
* Configurable cooldowns.
* Confidence thresholds.
* Independent face/voice states.
* Clear identity transitions.
* Stable event identifiers.

Face recognition and speaker recognition should be allowed to have different thresholds.

Do not merge their raw similarity values using arbitrary weights without calibration.

### 11.3 Identity profiles

Use permanent internal identifiers independent of display names.

Example:

```json
{
  "person_id": "person_001",
  "display_name": "Maciej",
  "face_enrolled": true,
  "voice_enrolled": true,
  "enabled": true
}
```

The device must allow a person to have:

* Face only.
* Voice only.
* Both modalities.

Do not require both recognition systems to be enrolled before enabling a profile.

---

# 12. HOME ASSISTANT INTEGRATION

Home Assistant integration must use MQTT.

The device should register itself automatically through MQTT Discovery.

Use a stable device identifier derived from hardware identity.

Device name:

**ESP32-P4 Home Buddy**

### 12.1 Entities

Provide at least the following entities.

```text
sensor.home_buddy_last_person
sensor.home_buddy_face_person
sensor.home_buddy_voice_person
sensor.home_buddy_identity_method
sensor.home_buddy_identity_status

event.home_buddy_recognition

binary_sensor.home_buddy_face_detected
binary_sensor.home_buddy_speech_detected

sensor.home_buddy_ambient_light
sensor.home_buddy_brightness
sensor.home_buddy_free_memory
sensor.home_buddy_uptime

binary_sensor.home_buddy_camera_available
binary_sensor.home_buddy_mqtt_connected
```

For each registered person, optionally expose an individual presence-detection binary sensor.

These binary sensors must indicate local detection at the device, not guaranteed presence inside the entire house.

Names should be available in Polish through MQTT Discovery.

Entity identifiers must remain stable after device restarts.

### 12.2 Recognition events

Publish recognition events to:

`home_buddy/identity/events`

Example:

```json
{
  "event_type": "recognized",
  "event_id": "00001234",
  "person_id": "person_001",
  "person_name": "Maciej",
  "method": "both",
  "status": "confirmed",
  "face_similarity": 0.91,
  "voice_similarity": 0.84,
  "timestamp": "2026-10-22T10:06:00+02:00"
}
```

The above values are illustrative.

Supported event types should include:

```text
recognized
unknown
conflict
presence_enter
presence_leave
```

Implement appropriate semantics for each event.

### 12.3 Automations

Recognition events must be usable as triggers in the Home Assistant visual automation editor.

Prefer native MQTT Event discovery and, where useful, MQTT device triggers.

A repeated recognition must be able to generate a new automation event even if the last recognized person has not changed.

Do not rely only on changing the state of a conventional text sensor.

### 12.4 MQTT reliability

Implement:

* Persistent broker configuration.
* Reconnection.
* Last Will and Testament.
* Availability state.
* Retained discovery configuration.
* Non-retained recognition events.
* Configurable QoS.
* Unique event identifiers.
* Correct cleanup when profiles are deleted.

Avoid publishing duplicate recognition events after network reconnection.

Do not replay historical recognition events as though they happened in real time.

### 12.5 Temperature input

Subscribe to a configurable MQTT topic that carries the outdoor temperature from HA.

Provide documentation and an example HA automation that publishes updates when the chosen temperature entity changes.

If the selected temperature entity becomes unavailable, handle that gracefully.

### 12.6 Security

Never publish biometric templates, microphone recordings or face images to MQTT.

Use only identifiers, names, recognition status and diagnostic metadata.

Support MQTT authentication.

Evaluate TLS support, certificate validation and its memory implications.

---

# 13. MULTITASKING AND PERFORMANCE

The system must simultaneously perform:

* LVGL rendering.
* Touch handling.
* Wi-Fi networking.
* HTTP image downloading.
* JPEG decoding.
* Clock updates.
* Temperature updates.
* Camera capture.
* Ambient light estimation.
* Face recognition.
* Audio capture.
* Speaker recognition.
* MQTT communications.
* Notifications.

Use FreeRTOS to separate responsibilities.

### 13.1 Suggested tasks

```text
UI_TASK
NETWORK_TASK
SNAPSHOT_TASK
CAMERA_CAPTURE_TASK
FACE_AI_TASK
AUDIO_CAPTURE_TASK
VOICE_AI_TASK
IDENTITY_FUSION_TASK
BRIGHTNESS_TASK
MQTT_TASK
SYSTEM_MONITOR_TASK
```

Do not create a separate task unless it is justified by the execution model.

Use queues and event groups to communicate between components.

Avoid shared mutable state wherever possible.

Use one camera driver and distribute frame references or buffers to its consumers.

Do not initialize multiple competing camera capture pipelines.

### 13.2 LVGL synchronization

All LVGL object operations must occur under the appropriate LVGL synchronization mechanism.

Worker tasks should notify the UI task through queues.

Network operations, camera capture, model inference and flash writes must not block LVGL rendering.

### 13.3 Scheduling priorities

Prioritize:

1. Audio capture stability.
2. Camera frame acquisition.
3. UI responsiveness.
4. Recognition inference.
5. Network transfers.
6. Background diagnostics.

These are initial priorities and must be validated against real profiling data.

Lower the AI processing rate when necessary to maintain display stability.

### 13.4 Memory management

Measure actual memory allocation during development.

Account for:

* Framebuffers.
* JPEG buffers.
* CSI camera buffers.
* Audio buffers.
* Model weights.
* Model activations.
* Feature databases.
* LVGL allocations.
* Networking.

Prefer PSRAM for large buffers.

Avoid repeated large allocations in high-frequency loops.

Allocate reusable buffers at startup where appropriate.

Evaluate fragmentation during long-running operation.

Do not assume that 32 MB PSRAM guarantees all models can remain loaded simultaneously.

### 13.5 AI execution

Use model inference on a configurable schedule rather than necessarily on every captured frame.

Consider:

* Face recognition every few frames.
* Face tracking between recognition passes.
* VAD-gated speaker identification.
* Pausing expensive inference during enrollment.
* Avoiding concurrent peak-memory operations.

The fullscreen snapshot must remain smooth and readable even while AI inference is active.

---

# 14. BIOMETRIC DATABASE

Profiles must remain on the physical device.

Provide a dedicated local profile manager.

All model and profile storage must use onboard Flash. External storage is excluded.

### 14.1 Requirements

* Persistent profiles.
* Stable identifiers.
* Multiple face samples.
* Multiple speaker samples.
* Versioned storage schema.
* Corruption detection.
* Atomic updates.
* Safe deletion.
* Storage-capacity management.

Sensitive biometric templates must not be included in normal diagnostic logs.

Investigate available encryption mechanisms.

If encrypted flash/NVS is technically feasible for the target partition layout, use it.

If a desired security feature is unavailable on the hardware, document the limitation rather than pretending the data is protected.

### 14.2 Factory reset

Provide distinct operations for:

* Resetting Wi-Fi configuration.
* Resetting MQTT/HA settings.
* Removing one person's profile.
* Deleting all biometric profiles.
* Full factory reset.

Full factory reset must require explicit confirmation.

---

# 15. PRIVACY AND SECURITY

The system processes sensitive biometric information.

Build privacy safeguards into the architecture from the beginning.

Requirements:

* Fully local inference.
* Fully local biometric enrollment.
* No cloud inference.
* No external model training.
* No biometric uploads.
* No continuous audio recording.
* No unnecessary image retention.
* Local administrator authentication.
* Ability to disable face recognition.
* Ability to disable voice recognition.
* Ability to delete individual profiles.
* Clear indication during biometric enrollment.
* Documented data-retention behavior.

Enrollment must be intentional and performed with the person's knowledge and appropriate consent.

Biometric matching must not be treated as infallible authentication.

Do not use recognition alone to unlock doors, disarm alarms or authorize high-risk actions.

Provide an option to disable sending person names to HA and publish only stable IDs if desired.

### 15.1 Network security

Store credentials securely.

Do not hardcode passwords or API tokens.

Do not place secrets in GitHub.

Do not log authentication headers.

Avoid putting access tokens in HTTP query strings.

Do not expose unauthenticated configuration or profile-management endpoints.

### 15.2 Licensing

Keep the repository compatible with its Apache 2.0 license.

Review the licenses and redistribution terms of all imported components and AI model files.

Preserve required attribution and license notices.

Do not commit third-party weights or firmware binaries if their licenses prohibit redistribution.

---

# 16. OTA AND FIRMWARE DISTRIBUTION

Implement firmware updates suitable for a standalone embedded device.

Requirements:

* USB firmware installation.
* Version reporting.
* OTA support where feasible.
* Firmware integrity verification.
* Safe rollback or recovery.
* Preservation of configuration and profiles during ordinary updates.
* Explicit handling of partition migration.

Evaluate partition constraints carefully because the device has 16 MB flash and AI models may require substantial storage.

Do not assume that two complete OTA slots, all AI models and profile storage will fit without a partition-budget analysis.

Place model assets in a verified onboard model partition; validate capacity and recovery before migration.

All models must remain available locally at runtime.

MicroSD storage is allowed; an external inference server is not.

### 16.1 Build automation

Add GitHub Actions that:

* Build firmware.
* Run available unit tests.
* Check formatting.
* Validate configuration.
* Generate firmware artifacts.
* Publish versioned releases when explicitly triggered.

Do not automatically publish an untested firmware release as production-ready.

Provide Windows flashing instructions.

A browser-based installer may be added once USB installation and recovery are proven.

---

# 17. FAILURE HANDLING

The device must degrade gracefully.

### Home Assistant unavailable

Continue:

* Clock operation using last synchronized time.
* Local face recognition.
* Local voice recognition.
* Profile management.
* Brightness control.
* UI operation.

Continue displaying the last valid camera image.

Do not block local recognition while reconnecting.

### UBox Bridge unavailable

Keep the last valid image.

Expose the connection failure in the menu.

Retry without excessive network traffic.

### MQTT unavailable

Continue local recognition.

Display greetings.

Do not accumulate an unbounded queue of historical events.

### Camera unavailable

Keep the screensaver functional.

Continue voice recognition if possible.

Use the last safe backlight brightness value.

### Microphone unavailable

Continue face recognition.

### Model initialization failure

Do not crash the UI.

Expose a meaningful diagnostic message.

Disable only the affected recognition subsystem.

### Corrupted profile database

Do not silently overwrite all user profiles.

Provide recovery and diagnostics.

---

# 18. DEVELOPMENT ROADMAP

Work in incremental milestones.

Each milestone must result in a coherent, buildable state.

Do not start implementing all functionality in one monolithic commit.

## PHASE 0 — Research and project initialization

Tasks:

* Inspect the repository.
* Verify hardware specifications.
* Check existing ESP32-P4 board support.
* Review camera drivers.
* Review ESP-DL face and speaker models.
* Select compatible ESP-IDF/LVGL versions.
* Establish memory and flash budgets.
* Create the project structure.
* Add development documentation.
* Add initial CI configuration.

Deliverables:

* Buildable firmware skeleton.
* Hardware compatibility document.
* Architecture document.
* Dependency list.
* Development roadmap.

Acceptance criteria:

The project can be built reproducibly from a clean checkout.

---

## PHASE 1 — Hardware bring-up

Tasks:

* Initialize ESP32-P4.
* Detect hardware revision.
* Initialize PSRAM.
* Initialize ST7701S.
* Initialize landscape framebuffer.
* Initialize GT911.
* Initialize backlight PWM.
* Initialize ESP32-C6 networking.
* Test USB serial diagnostics.
* Validate basic FreeRTOS operation.

Deliverables:

A basic LVGL application running at 800 × 480.

Acceptance criteria:

* Display works.
* Touch works.
* Wi-Fi connects.
* Backlight can be adjusted.
* No continuous rebooting.
* No black-screen startup issues.
* No recurring display corruption.

---

## PHASE 2 — Fullscreen screensaver

Tasks:

* Implement UBox Bridge HTTP client.
* Add token authentication.
* Download original-resolution JPEG.
* Implement ETag caching.
* Decode and crop images.
* Display fullscreen.
* Implement NTP clock.
* Implement temperature input.
* Render clock and temperature overlays.

Deliverables:

Working screensaver with fullscreen camera image.

Acceptance criteria:

* Image covers 800 × 480.
* No borders or black bars.
* No permanent header or footer.
* No noticeable flicker during refresh.
* Clock is correct.
* Temperature updates without image reload.
* Unchanged images are not downloaded repeatedly.
* Polling does not trigger an LTE camera capture.

---

## PHASE 3 — Complete LVGL settings interface

Tasks:

* Implement main menu.
* Add touchscreen navigation.
* Add inactivity timeout.
* Add Wi-Fi configuration.
* Add MQTT configuration.
* Add UBox configuration.
* Add local administrator PIN.
* Implement persistent settings.

Deliverables:

Usable standalone configuration interface.

Acceptance criteria:

The device can be configured without modifying source code.

After initial USB flashing, ordinary setup should not require a development computer.

---

## PHASE 4 — Camera and automatic brightness

Tasks:

* Initialize onboard camera.
* Confirm OV02C10 operation.
* Capture frames.
* Expose camera diagnostics.
* Implement ambient-light estimation.
* Implement day/night calibration.
* Add brightness interpolation.
* Add smooth PWM transitions.
* Add manual override.

Deliverables:

Automatic brightness system.

Acceptance criteria:

* Day and night calibration persist after restart.
* Brightness transitions are smooth.
* Passing people do not cause rapid brightness changes.
* Camera failure does not permanently blank the display.

---

## PHASE 5 — Local facial recognition

Tasks:

* Integrate face detection model.
* Integrate face recognition model.
* Implement camera preprocessing.
* Implement face embeddings.
* Implement profile enrollment.
* Implement recognition database.
* Implement unknown-person handling.
* Add LVGL recognition test screen.

Deliverables:

Fully local facial identification.

Acceptance criteria:

* New people can be enrolled entirely on the touchscreen.
* Recognition works offline.
* Profiles survive restart.
* Unknown faces are not systematically assigned to known users.
* Users can delete profiles.
* No inference data leaves the device.

---

## PHASE 6 — Local speaker recognition

Tasks:

* Initialize audio codec and microphone.
* Capture reliable PCM audio.
* Implement VAD.
* Integrate speaker-verification model.
* Implement voice enrollment.
* Implement speaker embeddings.
* Save voice profiles locally.
* Add speaker identification.
* Handle insufficient speech and unknown voices.

Deliverables:

Fully local speaker identification.

Acceptance criteria:

* Voice enrollment works through the device.
* No computer-based training.
* No external AI server.
* Recognition continues offline.
* Stored profiles survive reboot.
* Microphone operation does not destabilize the display.

---

## PHASE 7 — Identity fusion and Home Assistant

Tasks:

* Implement identity-fusion engine.
* Implement temporal stabilization.
* Implement duplicate suppression.
* Implement greeting notifications.
* Implement MQTT Discovery.
* Implement recognition event publishing.
* Implement per-person detection entities.
* Add event diagnostics.
* Add example HA automations.

Deliverables:

Complete identity automation integration.

Acceptance criteria:

* Known user identification creates an appropriate HA event.
* Recognition by face and voice works independently.
* Conflicting identities are handled explicitly.
* Repeated recognition can trigger a new event after cooldown.
* LVGL displays "Cześć <użytkownik>!".
* The notification does not replace the camera image.
* All AI processing remains local.

---

## PHASE 8 — Security, maintenance and OTA

Tasks:

* Improve credential storage.
* Protect biometric profiles.
* Finalize deletion workflows.
* Implement OTA where feasible.
* Implement firmware recovery strategy.
* Add diagnostics.
* Add configuration migration.
* Complete installation documentation.

Deliverables:

Maintainable and updateable firmware.

Acceptance criteria:

Ordinary firmware updates do not destroy user profiles.

The device can recover from failed configuration and network operations.

---

## PHASE 9 — Integration testing and optimization

Tasks:

* Profile CPU usage.
* Profile PSRAM usage.
* Evaluate model loading.
* Test simultaneous AI and UI operation.
* Test Wi-Fi reconnection.
* Test MQTT reconnection.
* Test extended uptime.
* Test camera disconnection.
* Test profile database recovery.
* Test repeated enrollment.
* Test automatic brightness under changing lighting.
* Optimize scheduling and memory allocation.

Deliverables:

Release candidate.

Acceptance criteria:

* No reproducible memory leaks.
* No recurring watchdog resets.
* No permanent UI freezes.
* Stable network recovery.
* Stable biometric database operation.
* Responsive touchscreen under AI load.

Run a 72-hour continuous-operation test on physical hardware before marking the firmware stable.

If physical hardware testing is not available to the development agent, clearly mark this acceptance criterion as **NOT VERIFIED ON HARDWARE**.

Never claim that compilation alone proves hardware functionality.

---

# 19. TESTING STRATEGY

Include host-side automated unit tests wherever feasible.

Test pure algorithms independently from hardware.

Important test cases:

**Image handling**

* Image aspect ratio.
* Center cropping.
* HTTP ETag behavior.
* Invalid JPEG handling.
* Partial downloads.
* Loss of connection.

**Brightness**

* Day/night interpolation.
* Reversed calibration ranges.
* Invalid calibration values.
* Camera measurement outliers.
* Transition smoothing.
* Manual override expiration.

**Identity**

* Unknown person.
* Known face only.
* Known voice only.
* Matching face and voice.
* Conflicting face and voice.
* Recognition cooldown.
* Person leaving the detection area.
* Repeated recognition events.

**Storage**

* Profile creation.
* Profile deletion.
* Corrupted data.
* Power interruption during write.
* Schema migration.

**MQTT**

* Discovery payload validity.
* State publication.
* Event publication.
* Reconnection.
* Availability changes.
* Retained versus non-retained messages.

Add integration tests using mocked camera frames and audio samples where licensing permits.

Never include actual household biometric samples in the public repository.

---

# 20. DOCUMENTATION REQUIREMENTS

Maintain comprehensive project documentation.

README must contain:

* Project overview.
* Supported hardware.
* Features.
* Architecture summary.
* Firmware installation.
* Initial configuration.
* Home Assistant integration.
* Known limitations.
* Links to detailed documentation.

Provide additional documents for:

* Hardware pinout.
* ESP32-C6 firmware compatibility.
* Build environment.
* Camera initialization.
* Microphone initialization.
* LVGL architecture.
* Biometric enrollment.
* MQTT topics and schemas.
* HA automation examples.
* Brightness calibration.
* OTA updates.
* Troubleshooting.
* Privacy and data storage.

Documentation must match the implemented firmware.

Do not document unimplemented functionality as already working.

Mark experimental or incomplete capabilities explicitly.

---

# 21. DEVELOPMENT RULES

Follow these rules during implementation.

### Rule 1 — Never modify UBox Bridge

The repository `mpuchara/HA_UBOX_bridge` must remain untouched.

Use only its existing API.

If an additional endpoint would improve the integration, document the proposal without changing that repository.

### Rule 2 — No external AI dependencies

The complete inference pipeline must run on ESP32-P4.

Do not introduce remote AI processing.

### Rule 3 — No off-device biometric enrollment

Do not require users to prepare embeddings or train individual profiles on a PC.

The enrollment workflow must be available through LVGL.

Pretrained model weights may be prepared and packaged during firmware development, but personal enrollment must take place on the device.

### Rule 4 — Hardware first

Do not claim a peripheral works until it has been tested or explicitly marked as unverified.

### Rule 5 — Avoid fabricated compatibility

Check APIs and dependencies.

Do not invent nonexistent driver functions, model APIs, configuration options or GPIO assignments.

### Rule 6 — Stable intermediate releases

Every phase should leave the repository buildable.

Use feature branches and reviewable commits.

Do not merge code that breaks previously working functionality.

### Rule 7 — Preserve privacy

No personal biometric data in Git.

No raw face images or audio in MQTT events.

No secrets in commits.

### Rule 8 — Keep the UI responsive

AI processing must not block LVGL.

### Rule 9 — Prefer existing components

Use maintained, compatible and properly licensed libraries wherever possible.

### Rule 10 — Maintain a working development log

Track:

* Completed milestones.
* Tested components.
* Known problems.
* Unverified assumptions.
* Hardware results.
* Next development tasks.

---

# 22. DEFINITION OF DONE

The project is complete when the following user experience works on the physical device.

### Initial startup

The user powers on the device.

The LCD initializes in landscape orientation.

If not configured, the LVGL setup wizard starts.

The user configures Wi-Fi, UBox Bridge, MQTT and the outdoor temperature source.

### Normal operation

The device displays a fullscreen 800 × 480 camera snapshot.

The clock appears in the lower-left corner.

Outdoor temperature appears in the lower-right corner.

There are no permanent bars, headers or frames.

The image refreshes from UBox Bridge without unnecessary camera activation.

### Recognition

A registered person approaches the device.

The local camera detects the face.

The local model recognizes the person.

A subtle notification appears:

**Cześć Maciej!**

The notification disappears automatically.

The screensaver remains visible.

The device publishes the recognition event to Home Assistant.

An HA automation can react to the event.

### Voice recognition

A registered person speaks near the device.

The microphone captures speech.

The local AI identifies the speaker.

The device can publish a recognition event without relying on face recognition.

### Settings

Touching the display opens the LVGL menu.

The user can add another household member.

The device guides them through face enrollment and voice enrollment.

All processing takes place locally.

The user can configure Wi-Fi, HA integration and brightness.

After inactivity, the device returns to the fullscreen camera screensaver.

### Automatic brightness

The display adjusts to ambient light.

Calibration is performed locally.

Brightness changes gradually.

The feature works independently of HA and internet connectivity.

### Offline operation

When Wi-Fi or HA is unavailable:

* Facial recognition continues.
* Speaker recognition continues.
* Biometric profiles remain accessible.
* Brightness automation continues.
* Local greeting notifications work.
* The last successfully downloaded image can remain displayed.

---

# 23. FIRST DEVELOPMENT ITERATION

Start by inspecting the current repository.

Do not assume it already contains functional firmware.

Perform an initial technical assessment covering:

1. Target hardware revision and compatibility.
2. Available board support implementations.
3. ESP-IDF version selection.
4. LVGL version selection.
5. ESP-DL model compatibility.
6. Camera-driver availability.
7. Speaker-verification integration requirements.
8. Flash and PSRAM budget.
9. OTA partition feasibility.
10. Development risks.

Create the initial architecture and development documents.

Next, implement the smallest useful hardware foundation:

* Project build system.
* ESP32-P4 initialization.
* MIPI-DSI display.
* Landscape LVGL interface.
* GT911 touchscreen.
* Adjustable backlight.
* Basic Wi-Fi support.
* Serial diagnostics.

The first visible target is an **800 × 480 LVGL interface running on the physical display**.

After that, prioritize the fullscreen UBox screensaver before implementing the AI models.

Do not build a complex placeholder application unrelated to the final product.

Structure each milestone so that it directly contributes to the intended final architecture.

---

# 24. EXPECTED DEVELOPMENT OUTPUT

For each development milestone, provide:

**Implementation summary**

Explain what was implemented and why.

**Files changed**

Identify the relevant modules and their responsibilities.

**Build verification**

Report the actual build result.

**Tests**

Report which tests ran and their results.

**Hardware verification**

Clearly distinguish confirmed physical hardware behavior from unverified assumptions.

**Known limitations**

Describe remaining technical problems.

**Next milestone**

Identify the next set of development tasks.

Do not describe planned work as completed.

Do not claim successful camera operation, audio capture, AI inference or OTA without supporting tests.

Keep repository documentation synchronized with implemented capabilities.

---

# FINAL INSTRUCTION

Build ESP32-P4 Home Buddy as a complete, privacy-first, locally intelligent home assistant.

The final product must combine:

**Fullscreen UBox camera dashboard + clock + outdoor temperature + LVGL touchscreen settings + local face recognition + local speaker recognition + automatic brightness + MQTT Home Assistant automations.**

All biometric processing and enrollment must happen exclusively on the device.

The GUI must remain responsive and visually polished.

The architecture must be modular, documented, testable and suitable for continued development.

Use the existing ESP32-P4 Home Buddy repository as the sole development repository.

Never modify HA_UBOX_bridge.

**Begin with the repository assessment, architecture, build configuration and hardware bring-up, then progress through the roadmap until the product meets the definition of done.**
