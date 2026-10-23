# Privacy and security, 0.6.1-dev

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
