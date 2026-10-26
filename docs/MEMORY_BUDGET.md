# Memory and flash budget

## Current iteration 12 policy (0.11.0-dev)

The selected product mode is **persistent, unencrypted, local household storage with
automatic restoration**. The recognition build runs the existing schema-2 engine in
two 512 KiB banks inside the unchanged 1 MiB partition. Intentional household changes
save automatically; valid face/voice profiles and the previously enabled AUTO mode
restore locally at startup. No password, PIN, manual unlock, eFuse provisioning or
mandatory encryption. Physical extraction is not prevented. Experimental voice
identification remains gated. Implementation/host tests do not establish physical
reboot persistence; installation and the combined acceptance session remain pending.
See [storage decision](PERSISTENCE_DECISION.md), [database](PROFILE_DATABASE.md) and
[iteration report](ITERATION12_REPORT.md). Older iteration notes below are historical.


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
All model assets must use onboard Flash; pin hashes, licenses and model format/version.
Never fetch a model from a server as a runtime requirement. Signed OTA, rollback,
model compatibility and partition migration are not implemented.

## PSRAM: measured 32 MiB, provisional simultaneous budget

| Allocation | Bytes |
|---|---:|
| Two native RGB565 framebuffers | 1,536,000 |
| One logical RGB565 LVGL framebuffer | 768,000 |
| Two display snapshots during replacement | 1,536,000 |
| Capped JPEG staging | 2,097,152 |
| Example two 640×480 RGB888 capture frames | 1,843,200 |
| Example six seconds 16 kHz mono PCM | 192,000 |

The first four rows are implemented. Decoder also allocates aligned JPEG input
(up to another 2 MiB) and padded raw RGB565 (up to 8 MiB), then frees both.
UI queue holds at most four completed frames. Download-worker peak and coexistence
with a synthetic test must be measured, not inferred from aggregate free bytes. LVGL's internal pool is
128 KiB; task stacks, TLS, networking, JPEG workspace and model arenas are additional.
Internal DMA-capable memory and largest contiguous blocks must be measured separately.
Capture resolution and audio/model windows above are engineering estimates.
Do not claim that remaining aggregate bytes guarantee simultaneous AI operation.

## Iteration-2 boot measurement

0.2.0-dev with Wi-Fi worker and no configured bridge: internal free 328135 bytes,
largest block 245760, minimum 328135; PSRAM free 31244760 at uptime 3 seconds.
Logical framebuffer is 768000 bytes. JPEG logs report aligned input/raw allocation,
decode+cover time and free/largest blocks during a successful decode. These boot
numbers are not a JPEG peak or leak/soak result.

## Synthetic JPEG on the physical P4

1280x720 input, decode+cover156ms. During successful decode: internal free316471,
largest237568; PSRAM free28575716, largest28311552; aligned input51743, padded
raw1843200, output768000 bytes. After cleanup the idle PSRAM sample was30474708
with the displayed frame retained. This is one synthetic decode sample, not an
HTTP two-MiB peak or a repeated-swap leak/fragmentation test.

## Iteration 3 initial RGB565 experiment (superseded)

Three stable1288x728 RGB565 slots plus one IDF discard buffer total7,501,312 bytes;
explicit diagnostic preview adds165,888 bytes, released on close. Real boot with
CSI active: PSRAM23,743,048, largest23,592,960. After640x360 UBox decode and retained
saver:22,972,996 free. During that simultaneous decode:20,317,260 free,
largest19,922,944; decode+cover140ms. Internal DMA after network+capture:265,631 free;
steady pre-JPEG sample268,903, largest229,376. These are short-run samples, not
full4MP peak, full-resolution2304x1296 acceptance or leak/72h proof.
The earlier illustrative640x480 capture row is superseded by these actual buffers.
Future face weights/arenas and audio still require independent measured admission.

## Iteration 3 accepted RAW8 route

Three1288x728 RAW8 slots plus one discard use3,750,656bytes. Upright180x318
RGB565 preview adds114,480bytes and is released on close. After initial RAW8
startup, free PSRAM27,493,704, largest27,262,976 and internal DMA271,271bytes
were observed before UBox decode. These short samples do not establish a
2304x1296 JPEG peak, AI/audio capacity, fragmentation behavior or72-hour stability.
The retained buffers remain bounded through recovery; uncertain driver cleanup
quarantines camera allocations until reboot.

Final upright short sample at158..178seconds: PSRAM26,723,652bytes and largest
22,544,384 unchanged between samples; DMA265,431 then265,219bytes. Preview toggle
allocation/release and full-resolution JPEG peak are not inferred from these
idle samples. No AI/audio admission is implied.

## Natural2304x1296 UBox with RAW8 capture, 0.3.2-dev

Original cached image746,465bytes was completely downloaded and decoded/covered
in218ms on P4 while the local camera continued around30fps. Decoded RGB565 buffer
5,971,968bytes, final output768,000. At the image-worker allocation log: internal
free290,651/largest180,224; PSRAM free17,897,292/largest17,825,792. After cleanup,
PSRAM returned to26,723,652; largest block was17,825,792. Aggregate free return
is not proof of repeated-swap fragmentation safety. This is a natural3MP sample,
not worst-case2MiB compressed/4MP decoded admission or future AI/audio capacity.

Second natural2304x1296 frame:739,606bytes,295ms; at allocation log PSRAM
17,127,240/largest11,796,480, internal289,555/largest180,224. After replacing the
old displayed image, free PSRAM returned to26,723,652/largest23,592,960. Two full
frames/one replacement are observed, not a sustained repeated-swap/AI admission test.

##0.4.2 diagnostic detection budget

ESPDet416 weights are578,160bytes before pack/alignment; input194,688bytes.
Build and physical acceptance receipts are in DEVELOPMENT_LOG.md.
Model arenas/parameters and temporary candidates are additional. An8KiB worker
stack is created only after explicit activation. Model/input free on exit; the
worker remains. Loading/inference use conservative aggregate/largest-block
admission and serialize with JPEG decode. Heap-delta guards are not a hard private
allocator budget. Actual combined hardware allocation/latency/release receipts
follow separately; do not infer future recognition/audio capacity from free bytes.


ESPDet416 observed load delta on P4: 1,862,120 bytes PSRAM including194,688-byte
input, and29,328 (0.4.1) /29,784 (0.4.2) internal bytes. Shared heap deltas include
concurrent activity. In the0.4.1 diagnostic sample free PSRAM was24,742,992 with
largest17,825,792; a later idle sample returned to26,723,652/largest23,592,960.
This is a short observed return, not a repeated combined allocation/leak soak.


0.4.3 temporal presence adds only fixed scalar tracker/status fields and no
additional image/model/history allocation. Existing worker stack, lazy resources,
JPEG serialization and all admission/delta guards are retained. Runtime overhead
and combined long-run fragmentation remain to be measured, not inferred from
host policy test speed.


0.4.4 adds only scalar demand/countdown fields and one diagnostic LVGL page. The
background test reuses the existing model/input/task without a second arena or
frame. Lazy model/input stay allocated while either owner is active; page close
frees preview widgets/buffer while retaining a live monitor's model. Existing
admission and load-delta guards are unchanged. Combined natural JPEG allocations,
expiry release and repeated requests need measured receipts.


0.4.5 quality uses only scalar accumulators and up to four scalar result structs;
no extra ROI image, histogram or heap allocation. Existing234x416 input is reused.
64-bit sum products bound arithmetic at maximum generic800x480 ROI. Hardware
quality_us overhead remains to be measured; no speed or recognition claim.


## 0.5.0-dev local identity experiment

See [face recognition implementation and limitations](FACE_RECOGNITION.md),
[onboard Flash layout and migration proposal](IDENTITY_FLASH_LAYOUT.md), and
[actual build/test versus pending hardware acceptance](IDENTITY_VALIDATION.md).
The experimental MBF worker remains opt-in. The 0.6.1-dev candidate uses onboard
Flash for model storage, gated encrypted profile persistence and native identity
HTTP. Iteration 12 selects LOCAL_UNENCRYPTED independently of hardware provisioning. See ITERATION7_REPORT.md and
FLASH_MIGRATION.md before installation. UBox/Open-Meteo/clock and camera leases remain.


## Iteration 10, 2026-10-25

Derived additional arrays: 96,480 B mono PCM window and 96,000 B filterbanks; 1,920 B capture stack PCM arrays; 5,120 B DMA; FFT/window/mel/model and 8 KiB voice task stack add to these. No 192,960 B full float-audio copy. The lazy SV provider requires >=128 KiB free internal, >=64 KiB largest internal and >=8 MiB largest PSRAM before construction. Those provisional admission floors are not measured usage/peaks. Metrics are sampled and include stack/load/preprocess/inference timing; physical peaks/coexistence remain pending.


## Iteration 11, 2026-10-25

Voice DB inside the live unified snapshot:65,768 B; staging holds one four-template person plus policy/session metadata, allocated lazily in PSRAM only after acceptance and an explicit Begin. Capture result is a single approximately2 KiB RAM mailbox with one worker-side consume slot. Schema2 uses312,416 B per snapshot; a transaction needs a separate same-size PSRAM scratch snapshot. The iteration-12 runtime allocates 312,416 B live plus 312,416 B scratch = 624,832 B in bounded PSRAM. These are structure/array sizes, not measured board free memory or peak usage. Actual internal/PSRAM/stack/load/inference/coexistence measurements remain pending.

## 0.11.1-dev: bounded UI status workspace

Captured LVGL stack protection fault during the create receipt -> enrollment page
transition. Moved full hb_id_status values out of nested UI frames into three
UI-task-owned page/refresh/preview slots: 4,872 B static internal RAM on P4, capped
at 6 KiB. The 8 KiB LVGL stack is unchanged. Real target compiler frames now have
an update/identity_update/identity_page subtotal of 2,736 B versus 7,360 B before;
this excludes LVGL/libc/logging and is not a measured full-task peak. Each firmware
build enforces the critical frame and nested subtotal limits. See ITERATION12_STACK_FIX.md.

## 0.11.2-dev storage diagnostics

One storage result scalar increases the target UI workspace from 4,872 to 4,896 B
(ELF symbol size 0x1320), below the unchanged 6 KiB cap. Target UI nested frame
subtotal remains 2,736 B; physical complete-task peaks remain unmeasured. No new
snapshot, model, persistent partition or task stack is allocated.
