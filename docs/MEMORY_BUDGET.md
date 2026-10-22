# Memory and flash budget

These are provisional allocations, not measured AI capacity.

## 16 MiB flash

| Region | Offset | Size |
|---|---:|---:|
| NVS | 0x9000 | 24 KiB |
| OTA data | 0xf000 | 8 KiB |
| PHY | 0x11000 | 4 KiB |
| OTA0 | 0x20000 | 6 MiB |
| OTA1 | 0x620000 | 6 MiB |
| Profiles (reserved, not mounted) | 0xc20000 | 2 MiB |
| Models (reserved, not mounted) | 0xe20000 | 1920 KiB |

Final byte is exactly 0x1000000. Bootloader and partition-table space are excluded
from app slots. Gaps/alignment are deliberate. The static checker rejects overlap,
misalignment, unequal OTA slots and overflow.

Two app slots fit the current small firmware. **This does not prove both biometric
model sets fit.** Only 1920 KiB is reserved separately for models. Before adding
models, measure exact quantized weights, code growth and inference arena demand.
Use local microSD assets if required; pin hashes, licenses and model format/version.
Never fetch a model from a server as a runtime requirement. Signed OTA, rollback,
model compatibility and partition migration are not implemented.

## PSRAM hypothesis: 32 MiB

| Allocation | Bytes |
|---|---:|
| Two native RGB565 framebuffers | 1,536,000 |
| One logical RGB565 LVGL framebuffer | 768,000 |
| Planned two decoded display snapshots | 1,536,000 |
| Planned capped JPEG staging | 2,097,152 |
| Example two 640×480 RGB888 capture frames | 1,843,200 |
| Example six seconds 16 kHz mono PCM | 192,000 |

Only the first two framebuffer rows are implemented. LVGL's internal pool is
128 KiB; task stacks, TLS, networking, JPEG workspace and model arenas are additional.
Internal DMA-capable memory and largest contiguous blocks must be measured separately.
Capture resolution and audio/model windows above are engineering estimates.
Do not claim that remaining aggregate bytes guarantee simultaneous AI operation.
