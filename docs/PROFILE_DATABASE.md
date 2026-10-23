# RAM-only profile database

The worker owns a bounded database of 8 people, each with 8-15 committed
512-float MBF templates. Each profile has an enabled flag, display name, stable
64-bit ID and templates. Future voice enrollment has no fabricated implementation.
ID = random boot namespace plus monotonically increasing counter. It is stable
through rename/delete of other entries during this RAM database's lifetime;
profiles themselves do not survive reboot. Deleted IDs are not reassigned when
an array slot is reused. A model namespace is checked before matching/mutation.

Pending templates live in separate staging. Additional-sample sessions copy the
committed set into staging before appending, up to 15 templates total. Verification plus explicit save commits
the complete set together in the single owner worker. Cancel/failed verification
leaves committed vectors unchanged. Delete clears the whole person's entry.

Upstream database is intentionally bypassed: its query returns sequential result
indices rather than stored feature IDs, and enrollment changes RAM before file
writes complete. It lacks the required household metadata/transaction contract.

No plaintext or encrypted biometric persistence exists. There are no implemented
on-flash schema, corruption recovery, failed-write rollback or reboot migration
claims. These are deferred until encryption/provisioning and storage layout are
settled. NVS/settings and the existing profiles partition are untouched.
[Privacy and storage details](FACE_RECOGNITION.md), [migration proposal](IDENTITY_FLASH_LAYOUT.md).
