# Onboard profile database, schema 1

The single identity worker owns 8 bounded household slots, each with up to 15
512-float MBF templates. Enrollment still requires 8 diverse samples and a new
verification frame. Names, enabled flags, 64-bit IDs, templates and next-ID counter
are persisted together. Rename and additional samples preserve the ID. Deleting
an ID never makes it available for reassignment. Blank profiles may exist before
enrollment. The face-model namespace is checked on every restore and mutation.
Voice enrollment is not implemented; voice_enrolled is always false.

## Secure gate

The adapter opens only when HB_IDENTITY_PERSISTENCE is opted in, hardware Flash
encryption and Secure Boot are active, and profiles is the dedicated encrypted
0x41 data partition of the correct size. There is no key generation, eFuse write,
format-on-error or plaintext fallback. Unprovisioned development devices use
explicitly labeled RAM-only mode. No personal data is uploaded anywhere.

## Transaction format

The 1 MiB partition uses its first 512 KiB for two raw 256 KiB erase-aligned banks. Each bank holds a 128-byte
header and a 246,640-byte P4 ABI snapshot. Header fields include magic, schema,
payload size/dimension, generation, payload SHA256, exact model namespace and a
second SHA256 protecting header metadata. The schema pins the little-endian P4
ABI and dimensions; it is NOT a portable interchange/export format. Incompatible
model, schema or ABI is rejected rather than silently reusing templates. A future
schema must perform an explicit migration. Capture sequences/times and greeting
cooldowns are cleared before storage because uptime is not meaningful after boot.

Commit erases the inactive bank, writes payload, reads/hash-verifies it, writes the
header LAST, then reads/hash-validates the committed bank. The previous bank is
untouched until a later transaction. Reboot selects the highest valid generation;
a failed/torn write recovers the prior committed copy. With no valid copy, corruption
and model mismatch block persistent writes rather than formatting data. The worker
reports failures and reloads the last valid state. An all-blank header is an
uncommitted bank, including a power loss before the final header write.

Deletion commits the remaining database first, then erases the obsolete bank.
A scrub failure is reported, never acknowledged as fully removed. Power loss
between commit and scrub preserves logical deletion in the new generation but
may leave encrypted obsolete blocks until scrub/reuse. This is not cryptographic
erasure against a holder of encryption keys; replay of an old whole-device image
is not prevented by this schema. Prior fallback generations may contain pre-rename
state, which is the documented tradeoff of recovery after corruption.

## Capacity and wear

Each bank leaves 15,376 bytes after header/snapshot. At most 120 face vectors
(245,760 bytes) plus metadata fit. Do not claim this also fits voice templates.
The store alternates banks and writes only on explicit create/rename/save/delete,
never every frame, greeting or HTTP publish. This spreads erase cycles over two
banks but is NOT a general wear-levelled filesystem; Flash endurance still limits
write frequency. Capacity is checked before any erase. PSRAM scratch storage is
bounded to one extra database snapshot and is zeroed after each transaction.

Host fault injection covers erase/write/read boundaries and torn writes, capacity,
corruption fallback, create/rename/delete, maximum templates, stable IDs and reboot
restoration. Its injected checksum tests the transaction protocol; the device uses
mbedTLS SHA256. Host tests do not establish encryption/provisioning or physical
power-loss acceptance. See FLASH_MIGRATION.md and ITERATION7_REPORT.md.

The remaining 512 KiB is untouched and reserved for a future independently
versioned voice-template database. No voice format/dimensions are selected yet.
For two 256 KiB voice banks, each proposed serialized snapshot plus header must
fit 262,144 bytes, including eight people, all templates and metadata. Measure
that actual format before activation. Face data uses 245,760 feature bytes plus
880 ABI metadata bytes; each bank leaves 15,376 bytes. Raw encrypted Flash uses
same-size blocks, not filesystem per-record overhead; header/payload writes are
16-byte aligned. Security key storage is separate in nvs_keys. Larger voice
requirements need an explicit schema/layout migration, not unbounded allocation.
