# Iteration 9 validation receipts

## Firmware code receipt

Source: c0d63c02201c2336dd92226595a9874187dd8c9e, version 0.8.0-dev.
[CI run 40189532248](https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/40189532248)
passed host and all four clean firmware profiles: diagnostics, bringup, wifi,
recognition. Pinned dependencies.lock stayed unchanged. CI host also verified
identical synthetic main/settings/diagnostics render hashes on Linux.

Local committed-source recognition build completed with exit 0. Effective config
and enforced Flash budget passed. App: 4,463,232 bytes; SHA256
36ddac1014767acbb1590039adcf796e777161ad649922913fb4a3c0064d0175.
Factory capacity 7,340,032 bytes, margin 2,876,800 bytes. MBF package fits its
4,194,304-byte partition with 671,360 bytes remaining. Runtime RAM is not inferred
from these file sizes. No layout/security/model provisioning was performed.

## Final host additions

23/23 C/LVGL tests (including the added synthetic matching/confirmation ->
independent greeting/HA -> actual JSON/privacy/cooldown/outage policy pipeline);
9/9 Python tests; 3/3 project checks; formatting and whitespace checks passed.
The pipeline fixture uses synthetic embeddings/events, not network transport or
an LVGL/peripheral-worker simulator. GCC AddressSanitizer+UndefinedBehaviorSanitizer
passed identity/guided-enrollment and PCM tests in Ubuntu/WSL; the same script is
included in CI. The subsequent tests/documentation commit does not change firmware
source; its own current CI status must be checked rather than inferred from this run.

## USB candidate

Local directory: dist/0.8.0-dev-iteration9-validated. Built from the firmware code
receipt above; includes dependency/license inventory and iteration-9 operator docs.
All 40 SHA256SUMS entries were verified. The application-only dry run
selected only 0x20000 without opening a serial port. Full mode is a layout
migration and is not an ordinary application update.

Hardware: not performed. The installed baseline and current RAM enrollment remain
untouched. Minimal hardware acceptance is three compact checks in ITERATION9_REPORT.md.
The microphone, secure persistence, voice inference and long soak are not accepted.
