# Profile persistence decision, iteration 11

Decision proposed for the owner: keep **C (RAM-only)** on this board now. Develop
and independently review **B (owner-unlocked storage)** next. Choose **A** for an
unattended appliance only after security provisioning and failed-app USB recovery
are demonstrated on matching spare P4 silicon. None is activated by this document.

| | A: hardware-protected | B: owner-unlocked AEAD | C: current RAM-only |
|---|---|---|---|
| Protection | Flash encryption plus authenticated boot/firmware; keys in protected hardware | Ciphertext at rest; a high-entropy owner secret is absent from Flash | No durable biometric database |
| Startup | Automatic profile restore after verified boot | Locked until owner enters the secret locally | Re-enrollment after restart |
| Unattended use | Best once provisioned and recovered safely | Good until a reboot; owner must unlock again | Enrollment lost on power failure |
| USB maintenance | Signed app, per-device offset-correct encryption and proved ROM service path | Existing plain app updates can preserve ciphertext; unlock after reboot | Current straightforward USB workflow |
| Lost key / bad update | Lost keys or disabled download can make recovery impossible | Lost owner secret means biometric data loss; failed app recoverable with existing USB path | Restore firmware, then enroll again |
| Physical attacker | Device-level protection with silicon/debug/boot-policy limitations | Can replace unverified firmware and steal a later unlock secret or live plaintext | Can inspect live RAM or replace firmware, but no saved templates to extract |
| Implementation | Separate secure updater, spare-board proof, irreversible provisioning and key custody | Reviewed format/KDF/AEAD/unlock/lock UI, secret handling, migration and tests | Implemented and current default |

## Concrete B design to implement and review

Use the existing 1 MiB partition and two 512 KiB banks. Encrypt the entire unified
snapshot with AES-256-GCM through pinned mbedTLS. Each bank gets a fresh 96-bit
nonce, 128-bit authentication tag and versioned associated metadata covering
schema, generation, lengths and both model namespaces. Nonce generation needs
reviewed device RNG readiness and power-loss tests. No plaintext names, IDs or
vectors in the header. Integrity checksums in the prepared transaction engine
are **not authentication** and cannot substitute for AEAD.

Prefer an owner-held random 128-bit unlock/recovery key (26 base32 symbols).
Derive a per-database 256-bit AES key with HKDF-SHA256, a random salt and a
versioned domain string; the 128-bit secret is not used directly as an AES-256 key.
If a passphrase is offered, derive the key with PBKDF2-HMAC-SHA256, a random
per-database salt and versioned work factor benchmarked on the actual P4.
A short numeric PIN does not provide strong offline protection. UI throttling
cannot stop an attacker who copied Flash. Never store an automatic-unlock secret
on the same device. Do not add a convenient fallback key in NVS or firmware.

Unlock must be explicit on the device. Keep key and decrypted profiles only in
bounded RAM, erase temporary buffers on failure/lock, reject a wrong tag before
publishing identities, and provide an explicit lock command. Live-RAM protection,
secure zeroization and malicious-firmware resistance require separate review;
without Secure Boot, B cannot stop a physical attacker modifying the next firmware
to capture the owner's secret. There is no claim that B equals A.

USB app-only writes already preserve partition contents. Test an actual update
and subsequent unlock/ID restoration with synthetic profiles before accepting this
path. Secret backups contain keys, not biometric exports. Key entry, AEAD storage,
unlock UI and a secure update route for A are **not implemented** in this candidate.
Schema 2 below is transaction engineering, not a bypass of the active secure gate.
The encrypted envelope needs a distinct magic/version and authenticated reader;
never put ciphertext under the prepared raw `HBP2` payload format.

## Prepared schema 2 and migration recovery

Face payload: 246,640 B. Voice database: 65,768 B, including 65,536 B vectors.
Explicit padding: 8 B. Unified payload: 312,416 B. Header: 256 B. Each 524,288 B
bank leaves 211,616 B. ABI, dimensions and independent namespaces are pinned;
incompatible models are rejected. No partition table change is needed.

`hb_profile_v2_*` is compiled but has no runtime caller. It requires protected
storage and an explicit authorization argument for commit/migration/scrub. The
current `profile_flash` adapter and schema-1 persistence gate are unchanged.
The first migration scans the entire upper half for blank bytes, restores the
latest valid schema-1 snapshot, initializes an empty voice namespace, then commits
schema 2 at offset 0x80000 within profiles. Both old 256 KiB banks survive until
the new payload and final header have been read back and verified. Unknown schemas,
dirty upper-half data and incompatible payloads are refused without formatting.

Interrupted migration before a valid header leaves schema 1 readable. Do not
automatically retry over a dirty upper bank: preserve a private copy of the full
profiles partition, inspect it offline, verify both legacy generations, then seek
authorization for a targeted upper-bank recovery. An interruption after a valid
header may leave schema 2 recoverable; restore checks that before attempting migration.

Subsequent schema-2 alternation eventually replaces the legacy lower half. **Do not
boot schema-1 firmware after switching to schema 2**: it can restore an older profile
or report incompatibility, and it does not understand voice deletion. Rollback needs
an explicitly chosen compatible firmware/database pair, never reinterpretation.
Deletion commits first and then scrubs the obsolete bank. A scrub failure is reported;
it is not cryptographic erasure or anti-replay protection. A pre-migration copy may
still contain deleted templates and must remain private under the same custody policy.

Synthetic tests cover migration preservation, refused retry after torn migration,
both modality capacities, stable IDs, torn transaction writes/reads/erase boundaries,
corrupt-bank fallback, namespace/orphan rejection, logical deletion and scrub.
They do not prove physical power-loss behavior, encryption or USB preservation.

Version-specific references: [IDF 5.5.1 P4 Flash encryption](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32p4/security/flash-encryption.html)
and [Secure Boot v2](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32p4/security/secure-boot-v2.html).
The existing revision-specific provisioning/recovery analysis remains in
[SECURITY_PROVISIONING.md](SECURITY_PROVISIONING.md).
