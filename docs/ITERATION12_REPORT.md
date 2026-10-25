# Iteration 12 implementation and validation

Date: 2026-10-25. Firmware: **0.11.0-dev**. Branch: `codex/local-face-recognition`.
Implementation commit: `b941f201811559f9d7b25befca74b13cdf2c620d`.
The final documentation commit and exact package source are recorded by Git and the
package's `usb-package.json`; no personal database is part of the package.

| Required item | Delivered behavior / evidence |
|---|---|
| Latest commit/version | 0.11.0-dev; exact clean source hash in usb-package.json, current branch history |
| Storage policy | Explicit RAM_ONLY / LOCAL_UNENCRYPTED / HARDWARE_PROTECTED; recognition build selects LOCAL_UNENCRYPTED. No encryption, passwords, PINs, keys or eFuse provisioning |
| Schema-2 integration | Runtime worker uses the existing 312,416-byte unified face/voice snapshot; all CRUD/enrollment serialization shares the same household ID namespace |
| Save mechanism | Successful intentional mutations commit automatically through inactive-bank erase, payload write/readback, final header and committed-bank verification. Canonical dirty fingerprint suppresses identical saves; failed writes retain RAM and allow explicit retry |
| Automatic restore | Local startup validates both banks, restores newest valid generation, preserves names/IDs/flags/both modalities. Empty only when fully erased; errors never format |
| AUTO | NVS preference read after restore; starts asynchronously only with enabled enrolled face profiles, independently of network/HA. Model failure keeps preference/templates, with three bounded load attempts and 30-second spacing |
| Capacity | Eight people, fifteen face templates and four voice templates per person. Snapshot 312,416 B + header 256 B; 211,616 B headroom per 512 KiB bank |
| Flash | Existing 1 MiB profiles at 0xb20000; unchanged offsets/table/model partitions. Local application size 4,501,904 B; 2,838,128 B factory headroom. Face PDL3 3,522,944 B in 4 MiB partition |
| Memory | Bounded live + scratch snapshots: 624,832 B PSRAM. No snapshot-sized stack objects; same scratch for serialization, fingerprint, restore and migration. Actual on-board peak is NOT measured. Numeric restore/commit telemetry and global heap low watermarks prepared |
| Corruption/power loss | SHA256 payload/header checks plus structural/model validation; older valid generation recovers after injected erase/write/payload/header/readback failures. Torn first initialization and dirty migration fail closed, preserving data |
| USB | Application-only plan writes 0x20000 only, preserving NVS/profiles/models. Selected model plans also exclude profiles. Layout mismatch/full recovery safeguards retained; no chip erase or C6 write |
| UI | Four main sections retained. Automatic saving/saved/failure status, retry, stable enabled/disabled member state, advanced storage diagnostics, one-time NVS privacy acknowledgment. Durable enrollment receipt only after verified save |
| Automated tests | Local 32/32 CTest including LVGL/navigation/menu visual regression; 13/13 Python; 3/3 project checks. ASan/UBSan passed identity/audio/voice/household/schema-2 and both real Flash-adapter policies. Formatting/whitespace/effective config/Flash budget passed |
| GitHub Actions | Pending push of final documentation commit at report creation. Read the final response / PR checks for the actual run ID and result; local tests are not a CI claim |
| Physical acceptance | NOT INSTALLED or reboot-tested on board. No serial operation, physical migration, reset, security provisioning or Flash erase was performed |
| Exact remaining blockers | Approval to install; one combined enrollment/saved/restart/automatic AUTO/greeting/eligible HA-event session; physical PSRAM/internal/stack peaks, save latency and responsiveness. Voice remains gated pending separate pre-existing microphone/model/calibration acceptance |

## Scope of automated evidence

The tests run the actual profile transaction/boot engines and actual partition
adapter, with synthetic embeddings and simulated Flash/platform failure injection.
They cover blank startup, stable ID/name/enabled and both modalities through restore,
full capacity, bank alternation/generations, corrupt namespace/schema/checksum,
partial erases/writes, torn initial state, legacy migration interruption and deletion
scrub. Ordinary NVS tests cover AUTO ON/OFF, readback/commit failure and persistent
one-time notice. Wiring guards check restore-before-AUTO ordering, deletion
reconciliation-before-save and durable enrollment receipts. These do not execute a
complete ESP-IDF peripheral boot under simulated network/HA outage, nor do the shared
menu renders constitute a full screenshot test of every newly changed device page.
The local engine has no network/HA dependency. Full runtime behavior remains part of
the single physical acceptance, not a claimed host result.

Protocol fault tests use a deterministic test checksum; production firmware supplies
pinned mbedTLS SHA256. This demonstrates transaction handling and is not a separate
cryptographic implementation validation. Checksums do not authenticate malicious edits.

Global `internal_low` / `psram_low` telemetry is a lifetime heap low watermark, not
an isolated storage-stage peak measurement. Fixed workspace sizes are exact ABI sizes,
not inferred physical free heap. See MEMORY_BUDGET.md and PROFILE_DATABASE.md.

## Minimal approved installation acceptance

The first update from the installed RAM-only firmware loses its current RAM enrollment
when the board restarts. No installation has been done. After approval, use the safe
application-only USB helper on the verified unchanged layout. Enroll one member,
confirm Profile zapisane na urządzeniu, enable AUTO, restart once, verify the same
member/ID and AUTO return without interaction, then one greeting and one eligible HA
metadata event. Numeric telemetry can be captured during this same session; do not
request repeated enrollment or a long manual fault matrix.

No merges or production release. No changes to HA_UBOX_bridge, OTA, microSD, MQTT,
cloud inference, biometric network endpoints or mandatory security provisioning.
