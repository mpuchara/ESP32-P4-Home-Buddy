# Privacy and security status

No biometric enrollment, camera capture, audio capture or identity inference is
implemented yet. The current firmware does not transmit biometric data.

Keep private hardware backups, credentials, raw frames/audio and embeddings out
of Git and CI. The local/ directory is ignored. Serial diagnostics omit SSIDs,
passwords and access tokens. NVS is not automatically erased after init failure.
The current reserved profiles/models partitions are not mounted or formatted.

A local PIN foundation protects UBox/MQTT credential editing. Wi-Fi setup is
explicitly exempt at the operator request: local users can select a network and
enter its password, but that save cannot alter protected tokens, broker settings
or the existing PIN. Other credential edits have no default,
salted PBKDF2 hash, bounded unlock and temporary lockout. Credentials persist in a
single NVS record. NVS is NOT encrypted at rest in this development build. PIN is
an interface control, not protection against flash extraction or physical recovery.
Encrypted storage, secure provisioning, PIN recovery and production threat review
remain required. Do not burn irreversible eFuses during bring-up. No biometric
enrollment or deletion is exposed. Face/voice matching is not a liveness proof.

Store embeddings rather than recordings by default, discard enrollment source
buffers after extraction, and remove every template on profile deletion.
Never include embeddings, face images or microphone audio in MQTT.
Physical flash extraction and microSD access are separate threats: encrypted NVS
does not automatically encrypt files on SPIFFS or removable media. Production
security needs verified secure boot/flash encryption provisioning and recovery
design; do not burn irreversible eFuses during bring-up.
