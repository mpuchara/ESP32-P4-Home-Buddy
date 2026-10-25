# Privacy and security, 0.6.1-dev

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


Local opt-in face enrollment/recognition uses onboard camera, ESPDet, MNP and MBF.
Raw captures are transient; personal templates stay in RAM on unprovisioned boards.
The secure transactional Flash adapter is implemented but refuses plaintext
biometric writes. Voice recognition is not implemented. No microSD or external
inference is used. Face matching is not a liveness proof or sufficient door security.

HTTP identity events contain bounded metadata only, never images/audio/embeddings.
Publishing and names are separately opt-in. RAM IDs are never advertised as
persistent. Webhook IDs and REST tokens are bearer secrets: visible in HA settings
at the owner's request for checking input,
omitted from logs/exports and never reused from UBox. LAN HTTP is unencrypted;
HTTPS validates CA and hostname without bypass. Development NVS settings and
credentials are NOT encrypted at rest. Protect physical access and private backups.

No mandatory administrator PIN is added. Anyone with touchscreen access can edit
settings. The existing owner-requested visible UBox token field is unchanged;
HA credentials are also visible at the owner's request. Open-Meteo receives selected location through
verified HTTPS. Keep full Flash backups, keys, credentials and personal samples
outside Git, artifacts and CI. Do not automatically erase NVS on initialization
failure. See PROFILE_DATABASE.md for deletion/recovery limits and FLASH_MIGRATION.md
for the separately authorized security provisioning requirements.


## Iteration 8 continuation

AUTO requires deliberate opt-in; ordinary NVS stores its boolean only. The iteration-12 candidate restores valid local profiles automatically. Microphone test starts manually, expires after 60 seconds, clears PCM each block and sends/saves no audio. Local unencrypted persistence is explicitly selected; experimental voice identification remains gated. See AUDIO_DIAGNOSTICS.md and SECURITY_PROVISIONING.md.


## Iteration 10, 2026-10-25

Microphone/model tests require explicit local Start, stop at one window or 60 s, and cancel on Stop/navigation. One bounded RAM window and temporary features are local; no PCM, embedding or voice identity is sent to HA. Speaker embeddings from the diagnostic are discarded. The passive soak collector writes only numeric whitelisted telemetry, not raw serial logs. Real voice enrollment remains gated.
