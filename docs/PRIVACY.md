# Privacy and security status

No biometric enrollment, camera capture, audio capture or identity inference is
implemented yet. The current firmware does not transmit biometric data.

Keep private hardware backups, credentials, raw frames/audio and embeddings out
of Git and CI. The local/ directory is ignored. Serial diagnostics omit SSIDs,
passwords and access tokens. NVS is not automatically erased after init failure.
The current reserved profiles/models partitions are not mounted or formatted.

The operator explicitly requested no administrator PIN anywhere in settings.
Wi-Fi/UBox/weather forms open directly and all PIN gates/hash operations were removed.
Wi-Fi passwords remain masked. The operator explicitly requested the UBox token
visible in its edit field; general diagnostics/logs still omit it. Weather has no
secret or broker credentials and only sends the selected locality/coordinates
to Open-Meteo through verified HTTPS. Anyone with touchscreen
access can edit settings. Schema-1 settings remain compatible; obsolete PIN bytes
are ignored and cleared on the next successful configuration save.
NVS is NOT encrypted at rest in this development build. Encrypted storage and
production provisioning/recovery remain incomplete. No biometric enrollment or
deletion is exposed. Face/voice matching is not a liveness proof.

Store embeddings rather than recordings by default, discard enrollment source
buffers after extraction, and remove every template on profile deletion.
Never include embeddings, face images or microphone audio in MQTT.
Physical flash extraction and microSD access are separate threats: encrypted NVS
does not automatically encrypt files on SPIFFS or removable media. Production
security needs verified secure boot/flash encryption provisioning and recovery
design; do not burn irreversible eFuses during bring-up.


## 0.5.0-dev local identity experiment

See [face recognition implementation and limitations](FACE_RECOGNITION.md),
[unchanged flash layout and migration proposal](IDENTITY_FLASH_LAYOUT.md), and
[actual build/test versus pending hardware acceptance](IDENTITY_VALIDATION.md).
The experimental MBF worker remains opt-in. The 0.6.0-dev candidate uses onboard
Flash for model storage, gated encrypted profile persistence and native identity
MQTT. Unprovisioned hardware remains RAM-only. See ITERATION6_REPORT.md and
FLASH_MIGRATION.md before installation. UBox/Open-Meteo/clock and camera leases remain.
