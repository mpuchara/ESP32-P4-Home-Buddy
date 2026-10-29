# One combined physical session, after authorized USB installation

This candidate was not flashed. Restart loses the currently installed RAM-only
household enrollment. Installation and optional speaker-model provisioning must
be chosen deliberately before this checklist. Do not run a security bootloader,
migration, chip erase or C6 update.

1. Open the four-section menu, return to the UBox screensaver, and confirm touch,
   clock, weather and brightness remain responsive.
2. In advanced microphone diagnostics, Start once: brief silence, normal speech,
   louder speech and a short pause, then Stop and Start/Stop once more. Retain the
   bounded numeric serial receipt. Confirm codec success, useful selected-slot PCM,
   low silence activity, no persistent zeros/full scale/duplicate slots, sensible
   sample cadence and no DMA/read errors. Touch must work throughout. Choose the
   useful slot from evidence; a successful read alone is insufficient.
3. If the separately verified SV model was intentionally provisioned and capture
   passed, run one 3-second model diagnostic on that slot. Record load/preprocess/
   inference times, internal/PSRAM free and largest blocks, sampled lows, stack and
   release counts. Confirm a finite normalized embedding and subsequent face/UBox
   responsiveness. This is model execution acceptance, not speaker accuracy.
4. Enroll one face only if needed after the planned profile loss. Confirm one
   existing FACE greeting and one eligible Home Assistant notification. No repeated
   legacy camera/display/network test matrix is needed for unchanged components.

Voice enrollment/identification and its switch remain unavailable. A later focused
calibration session needs consenting speakers, separate enrollment/verification,
unknowns, distances/noise/TV/music and overlapping speech. Do not infer accuracy
from the single embedding diagnostic. This evidence cannot be synthesized off-device.

For passive reliability capture use the pinned ESP-IDF Python with pyserial:
`python tools/monitor_soak.py --port COM5 --hours 72 --output local/iteration11-soak.jsonl`.
It does not reset the unit, stores only numeric allowlisted telemetry and bounds
output to 64 MiB. Check coverage/gaps and uptime regressions before calling it a
continuous soak. No 72-hour run or long-term hardware result is claimed here.
