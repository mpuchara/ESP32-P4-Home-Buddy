# Iteration 12 implementation and validation

## Superseding development candidate

Iteration 13 continues from the owner-confirmed 0.11.2-dev retention/AUTO evidence
recorded below. See ITERATION13_REPORT.md for 0.11.3-dev changes and validation.
No new physical operation occurs in iteration 13.


## Physical persistence acceptance update, 2026-10-26

Installed firmware: **0.11.2-dev**, source
`88cfe34b714244928d087ad319db083cc9958c40`. Application-only COM5 flashing and
readback verification completed. The owner explicitly approved the separate
upper-bank recovery; current bytes matched the secured private backup, only
0xba0000/0x80000 was erased, and both blank banks were read-verified.

After new enrollment and a physical restart, the owner confirmed the household
member remained stored and recognition remained enabled. This accepts profile
retention and recognition-preference restoration in this single physical session.
It does not independently verify stable numeric IDs, recognition accuracy,
greeting/HA delivery, power-failure scenarios or measured peak resource usage.
Those evidence limits remain explicit. No further restart is required to repeat
this already completed test. Earlier pending status below is historical.

## Current storage recovery candidate, 2026-10-26

**0.11.2-dev** supersedes the candidate status below. The owner completed the
0.11.1-dev face flow, but storage was blocked. An explicitly authorized read-only
USB snapshot found a blank lower bank and an upper bank with only a partial payload
and no committed header. No erase or repair was authorized or performed. The new
candidate fixes the empty advanced menu, delays the create receipt until its save
attempt ends, and distinguishes storage result codes. Explicit guarded recovery,
installation and physical persistence acceptance remain pending. See
ITERATION12_STORAGE_RECOVERY.md for measured evidence, scope and validation.

## Current candidate status, 2026-10-26

The current corrected candidate is **0.11.1-dev**. The 0.11.0-dev installation
was completed on 2026-10-25 using application-only USB flashing with readback
verification and a matching partition table. Enrollment then reproduced a physical
LVGL stack protection fault before face registration could complete. Persistence
across a physical restart is therefore still unaccepted. The implementation and
host transaction results below remain applicable, but the historical installation
status below is superseded by this paragraph and ITERATION12_STACK_FIX.md.

The corrected candidate moves nested UI status snapshots to a bounded static
workspace and enforces target-compiled stack-frame limits. Local revalidation:
32/32 CTest, 16/16 Python and 3/3 project checks pass. No corrected candidate has
been installed during this continuation. Next hardware acceptance is the same
single combined enrollment/saved/AUTO/restart/greeting/eligible HA-event session,
after installation is authorized; do not restart the currently installed board
for an acceptance test before the enrollment fault is corrected there.

Date: 2026-10-25. Firmware: **0.11.0-dev**. Branch: `codex/local-face-recognition`.
Initial implementation commit: `b941f201811559f9d7b25befca74b13cdf2c620d`;
final resource-admission follow-up is in the branch history and package source hash.
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
| Flash | Existing 1 MiB profiles at 0xb20000; unchanged offsets/table/model partitions. Exact final application bytes/headroom are recorded in usb-package.json. Face PDL3 3,522,944 B in 4 MiB partition |
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
