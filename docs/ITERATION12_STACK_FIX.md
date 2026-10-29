# Iteration 12: enrollment transition stack fault

Date: 2026-10-26. Corrected candidate: **0.11.1-dev**.

## Captured physical evidence

The owner reproduced an immediate reboot after entering a household name and
pressing Dalej: rejestracja twarzy. A passive COM5 capture recorded Core 0
`Stack protection fault` in task `lvgl`. SP was 0x4ff623f0, below the lower bound
0x4ff62684 of the unchanged 8 KiB task stack. This is a confirmed stack overflow,
not a confirmed Flash watchdog or a face-model failure.

The installed ELF SHA256 starts `576394fb6`; the preserved matching ELF has full
SHA256 `576394fb6e7905ebcefbf1f8b42afb668b919e8885f9ba5c3b5f184ecf4e73ed`.
MEPC 0x402c3f82 resolves to `_vfprintf_r`, return address 0x4ff11f86 to `esp_log`.
The receipt path calls identity_page from identity_update; its opening diagnostic
log is reached with nested status snapshots still occupying the LVGL stack.
Disassembly before the fix showed:

| Function | Stack frame before | Target compiler frame after |
|---|---:|---:|
| profile_page | 3,360 B | 112 B |
| identity_page | 1,824 B | 192 B |
| identity_update | 4,464 B | 1,472 B |
| update | 1,072 B | 1,072 B |
| household_page | not measured before | 128 B |

The previous update -> identity_update -> identity_page frames alone consumed
7,360 B before LVGL/logging/libc overhead. The corrected subtotal is 2,736 B.
This subtotal is compiler evidence, not a measured peak of the complete task.
Raw capture and crash memory remain private under ignored local/iteration12;
no household names, templates or raw stack dump are published with this report.

## Change and regression protection

Three snapshots live in a fixed, LVGL-task-owned BSS workspace, with independent
page, refresh and preview slots. A refresh can open a page without overwriting its
own snapshot. The workspace occupies 4,872 B on P4 and has a 6 KiB compile-time cap.
No dynamic allocations, larger task stack, partition changes, schema change,
threshold changes, Flash erasure or weaker storage integrity are introduced.

The real ui.c target build emits GCC -fstack-usage data. An ALL build target runs
check_ui_stack.py and rejects oversized/missing/unbounded critical frames or an
update/refresh/page subtotal above 3,584 B. The same check runs in all CI firmware
profiles. Python regression tests reject the captured old frame sizes and cover
bounded/unbounded/missing records and nested-call limits. Existing UI wiring
checks retain their behavioral guards after status access becomes pointer-based.

## Validation and acceptance status

Local Python: 16/16. Existing CTest: 32/32, including LVGL menu/navigation renders.
Target frame guard passed with the values above. Firmware build, final package,
GitHub Actions and corrected physical acceptance are reported in the final task
response and private installation receipt when available.

The original 0.11.0-dev is physically installed and its reboot is reproduced.
Do not claim persistence or corrected enrollment is physically accepted until the
owner reaches enrollment, confirms saved status, then completes the single planned
restart/AUTO/greeting/eligible HA-event check with the corrected candidate.
