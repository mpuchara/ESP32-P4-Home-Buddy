# Iteration 12: interrupted first commit and advanced diagnostics

Date: 2026-10-26. Candidate: **0.11.2-dev**.

## Completed recovery and owner-confirmed restart

0.11.2-dev was installed application-only on COM5, with write/readback verification.
The owner subsequently gave explicit permission to discard the incomplete bank.
The helper verified security, the unchanged table and exact current profile bytes
against the private backup, erased only 0xba0000/0x80000 and read-verified both
banks blank. The private original backup remains outside Git and packages.

The owner then confirmed that a newly enrolled household member remained stored
after a physical restart and recognition remained enabled. This is physical
acceptance of profile retention and recognition-preference restoration for this
session, not a claim of greeting/HA delivery or measured recognition accuracy.
Earlier authorization/pending statements below describe the pre-recovery state.

## Physical evidence

The owner completed the face registration flow on installed 0.11.1-dev without
the previous reboot. The household profile then reported blocked storage. This
does not establish durable enrollment or reboot persistence.

With explicit owner permission to reset and read only, a complete 1 MiB profiles
partition backup was captured on COM5 and hashed outside the repository. No
Flash erase or write was performed during this investigation. The authorized
reset may have lost the newest RAM-only enrollment; do not claim it is durable.

The lower 512 KiB bank is fully erased. The upper bank has an erased 256-byte
header and 14,368 non-erased payload bytes starting immediately after it. Its
remaining region is erased. Neither bank contains a committed schema-2 header.
The observation is consistent with the original LVGL crash interrupting the
first create/save operation. Exact causal timing is inferred; the raw bank state
is directly measured. No validated committed generation can be restored.

The fail-closed boot engine correctly refuses to overwrite this state. It must
not be changed to interpret a blank header alone as a blank database.

## Software corrections

- The create navigation receipt is assigned only after its persistence attempt
  ends. Publishing the saving status no longer announces creation prematurely.
  A failed save still retains usable RAM and reports its failure honestly.
- Storage status preserves the actual result code. Policy, I/O, incompatible
  format/model, capacity and corrupt/interrupted data have separate messages.
  PROFILE_BOOT and periodic PROFILE_STATUS contain numeric metadata only.
- Advanced diagnostics passes eleven rows. The shared renderer previously
  returned without creating anything for more than ten. It now supports up to
  twelve rows with six bounded rows of two buttons, retaining 52-pixel targets.
  Real LVGL tests exercise all eleven labels/actions/bounds over 400 transitions.
  The updated synthetic diagnostic image was inspected before updating its hash;
  main-menu and settings baselines remain unchanged.

## Explicit recovery scope

`tools/recover_profile_storage.py` is offline by default. It accepts the private
1 MiB backup and its exact SHA256, validates the USB package and unchanged
profiles offset/size/type/flags, and produces a narrow erase plan only when:

- The lower bank is entirely erased.
- The upper header is entirely erased, but the upper bank is not blank.
- There are no programmed bytes outside the schema-2 payload region.

This pattern means headerless uncommitted **or unknown** data, not a validated
profile. The helper does not guess its contents or claim it can restore them.
It refuses committed headers, nonblank lower banks, unexpected layout and
unnecessary erases of a fully blank partition.

Execution requires explicit owner permission to discard this uncommitted bank,
`--execute` and `--discard-uncommitted-bank`. It checks actual hardware security,
the installed table and every profile byte against the approved backup before
erasing only **0xba0000 / 0x80000**. A changed database aborts before erase. It
then reads both banks and verifies the partition is blank. Backup remains private.
No full-chip erase, app write, NVS/model write, partition change, C6 or eFuse action
is implemented. The historical table encryption flag is preserved; hardware
Flash Encryption must be disabled. A separate application-only update installs
the corrected candidate.

The initial read-only backup authorization did not authorize erasing data.
Installation followed the owner's standing installation permission; the separate
bank erase followed explicit approval. Neither approval changes the firmware's
fail-closed policy for future interrupted/unknown databases.

## Validation and remaining acceptance

Local: 32/32 rebuilt CTest, 19/19 Python, 3/3 project checks, ASan/UBSan,
formatting and target recognition build pass. The real transaction test injects
the observed 14,368-byte first-payload interruption and verifies boot refusal
without modifying the damaged banks. Recovery tests cover security/table/read
failures, changed profile bytes, erase failure, post-erase verification failure,
and exact erase scope. Target UI frame subtotal remains 2,736 B.

CI result, exact clean-source package hash and final application size are in the
package validation receipt and final task response. Installation, scoped recovery,
profile retention and enabled recognition preference after a physical restart
are now confirmed as described above. Greeting and eligible HA-event delivery
remain unconfirmed in this session; do not repeat enrollment/restart merely to
reconfirm the already observed persistence. Physical peak memory,
storage latency/responsiveness and independent voice acceptance remain open.
