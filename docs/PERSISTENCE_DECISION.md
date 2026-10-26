# Selected persistence policy, iteration 12

## Iteration 13 update

LOCAL_UNENCRYPTED and automatic restore remain selected. Only two fully erased
banks permit a committed empty schema-2 fallback at startup. Unknown/torn/legacy
bytes never trigger formatting. See ITERATION13_REPORT.md for the bootstrap tradeoff.


The owner selected **persistent, unencrypted, local household storage with automatic
restoration**. This supersedes iteration 11's owner-unlocked AEAD proposal and the
previous recommendation to remain RAM-only. Firmware candidate: 0.11.0-dev.

No password, PIN, unlock key, external storage, cloud backup, eFuse change, Secure
Boot requirement or Flash Encryption requirement. No secret is embedded in firmware
or NVS. The owner accepts extraction of names and biometric templates by an attacker
with sufficient physical access. SHA256 detects accidental corruption; it does not
authenticate a database against a malicious writer. This mode is not encrypted.

`hb_profile_storage_mode` expresses RAM_ONLY, LOCAL_UNENCRYPTED and
HARDWARE_PROTECTED independently of checksums and transaction validity. The local
adapter refuses a secured device, rather than silently downgrading it. The protected
adapter still requires encrypted profiles, active hardware Flash encryption and
Secure Boot; it never provisions them. Hardware protection remains a future option
requiring its own USB recovery procedure and installation decision.

The recognition build enables persistence and LOCAL_UNENCRYPTED explicitly. The
existing partition table is unchanged. ESP-IDF 5.5.1 `is_partition_encrypted()` makes
the partition's effective encrypted property false when hardware Flash encryption
is off, despite the existing table flag. Reads/writes use ordinary esp_partition APIs.
No partition flag, offset, model allocation or C6 firmware is changed.

On boot the identity worker restores schema 2 before selecting AUTO. Valid legacy
schema 1 migrates only into a completely blank upper bank. Neither a failed read,
an unknown schema nor a torn initial transaction permits formatting. Both invalid
banks block writes, preserving bytes for private offline diagnosis. Approval to
install this candidate is also the decision to run its documented migration on the
device; no physical installation or migration has been performed during development.

First local enrollment requires one on-screen acknowledgment, remembered by ordinary
NVS with commit/readback. It is a privacy notice, not an unlock prompt. Updates and
recognition never require a password. Existing templates are restored without a new
acknowledgment or model/network/HA dependency.

See PROFILE_DATABASE.md for transaction/deletion/wear details and ITERATION12_REPORT.md
for evidence and remaining physical acceptance.
