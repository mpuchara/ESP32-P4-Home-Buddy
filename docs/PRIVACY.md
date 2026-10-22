# Privacy and security status

No biometric enrollment, camera capture, audio capture or identity inference is
implemented yet. The current firmware does not transmit biometric data.

Keep private hardware backups, credentials, raw frames/audio and embeddings out
of Git and CI. The local/ directory is ignored. Serial diagnostics omit SSIDs,
passwords and access tokens. NVS is not automatically erased after init failure.
The current reserved profiles/models partitions are not mounted or formatted.

A local administrator PIN, lockout/rate limiting, transactional profile storage,
credential encryption and secure provisioning remain required work. Until those
are implemented this firmware deliberately exposes no credential editor or
biometric enrollment/deletion operation. Do not deploy it as a secure identity or
access-control device. Face and voice matching alone are not liveness proofs.

Store embeddings rather than recordings by default, discard enrollment source
buffers after extraction, and remove every template on profile deletion.
Never include embeddings, face images or microphone audio in MQTT.
Physical flash extraction and microSD access are separate threats: encrypted NVS
does not automatically encrypt files on SPIFFS or removable media. Production
security needs verified secure boot/flash encryption provisioning and recovery
design; do not burn irreversible eFuses during bring-up.
