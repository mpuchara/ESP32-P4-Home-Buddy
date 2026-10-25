# Onboard household database, active schema 2

Selected mode: **LOCAL_UNENCRYPTED**, persistent local profiles with automatic restore.
Confidentiality is disabled by explicit owner decision. Integrity validation remains
mandatory. SHA256 is not authentication against malicious modification. The separate
HARDWARE_PROTECTED adapter retains its original security checks without downgrade.

## Layout and ownership

The unchanged 1 MiB profiles partition is at 0xb20000. Bank A is relative offset 0,
bank B 0x80000, each 524,288 bytes. Each bank holds a 256-byte header and 312,416-byte
P4 little-endian ABI snapshot: face 246,640 + voice 65,768 + zero padding 8 bytes.
Headroom per bank: 211,616 bytes. Capacity: eight stable household IDs, fifteen
512-float face templates per person and four independent 512-float voice templates.
Names and enabled states belong to the face household identity. Voice records refer
to that stable ID. Rename/face replacement preserve voice, voice replacement preserves
face, deletion reconciles both modalities before committing. Disabled people cannot
match. Voice model/schema validation is independent from face model validation.

No raw face image, PCM recording, network embedding endpoint or personal database
is packaged. Generic model weights are separate from household templates.

## Startup

The existing asynchronous identity worker opens Flash, allocates one bounded PSRAM
scratch snapshot, validates both banks and chooses the highest valid committed
generation. It restores both modalities before reading the ordinary NVS AUTO
preference. Dashboard startup does not wait for model loading. Local restore needs
neither Wi-Fi nor HA. Missing/incompatible model assets leave profiles and AUTO
preference intact; at most three model loads are attempted per activation, spaced
by 30 seconds after failure. Explicit AUTO activation permits a bounded new attempt.
Voice templates do not bypass the experimental voice acceptance gates.

Only two fully erased banks mean EMPTY. A header erased by a torn first transaction
with dirty payload is CORRUPT. Unknown schemas/namespaces, bad lengths, invalid
records/checksums or read errors never initialize an empty database. A valid older
bank remains usable. If neither is valid, RAM starts safely empty and storage writes
are blocked; original Flash bytes remain untouched for private offline diagnosis.

Schema-1 migration validates legacy face records, initializes an independent empty
voice namespace and commits first to the completely blank upper half. Both 256 KiB
legacy banks remain intact through verification. A dirty upper region blocks a new
migration; offline reviewed recovery is required. Schema-2 markers prevent fallback
reinterpretation as legacy. Physical migration only follows an approved installation.

## Saving and recovery

Only successful intentional household mutations request a save in the identity
worker, outside LVGL callbacks. A canonical fingerprint excludes greeting/sample
counters and suppresses identical saves. Detection, scores, greetings, HA events and
diagnostics never request profile writes. Face/voice staging remains atomic in RAM.

Commit validates the new snapshot, erases the inactive bank, writes its payload,
reads it back and checks SHA256, then writes the final header with generation,
model namespaces and independent header checksum. It verifies the committed bank
before updating the durable revision. The old bank remains untouched until this
succeeds. First initialization also refuses a nonblank target. Power loss yields
either the previous generation or a fully valid new generation, never a partial
logical database. Unknown/torn first writes require diagnosis rather than auto-erase.

Saving, saved and failed states are published. Enrollment completion receipt is
emitted only after a successful durable commit. A failed write keeps the current
usable RAM state and pending enrollment receipt; Ponów zapis retries explicitly.
It does not reload old Flash over newly captured embeddings. Durable event metadata
is false while RAM changes remain unsaved. Storage status and retry are also under
Informacje -> Diagnostyka zaawansowana -> Pamięć profili.

Deletion commits both modality removals before scrubbing the obsolete 512 KiB bank.
A scrub failure is reported separately while the new logical deletion remains durable;
retry attempts cleanup without another identical commit. This is best-effort obsolete
bank cleanup, not a promise of secure physical erasure of remapped cells, historical
Flash remnants or private external backups. No unnecessary biometric backup is made.

## Flash endurance and amplification

A normal commit erases 524,288 bytes and programs 312,672 bytes (payload plus header),
about 1.68 erased bytes per snapshot byte. A one-byte logical rename still writes the
full snapshot: this is deliberate transactional simplicity, not per-record wear
leveling. Banks alternate, so each bank's sectors normally see one erase per two
commits. Deletion adds an obsolete-bank erase. Repeated identical saves are deduplicated;
no recognition-cycle writes exist.

Representative SPI NOR data sheets specify about 100,000 program/erase cycles per
sector, for example the manufacturer-authored [W25Q128JV data sheet](https://www.winbond-electronics.com/pdf-7d/w25q128jvsjq.pdf).
This is an example, NOT identification or a lifetime guarantee for this board's Flash.
Using even a deliberately conservative 10,000 cycles as a planning assumption,
ten normal commits/day would consume about five cycles/day per alternating bank;
calendar life also depends on chip grade, temperature, retention, deletion and failures.
The exact installed chip's endurance remains unmeasured/unidentified in this iteration.

## Memory and evidence limits

Live unified snapshot plus scratch = 624,832 PSRAM bytes. Both are explicit heap
allocations, never 312 KiB stack objects. Serialization/restore/migration reuse the
same scratch; fingerprinting reuses it too. Small header/read buffers are bounded
(256-byte arrays, plus digests). Allocation failure blocks persistence without
formatting. These sizes are not measured board peaks. Physical peak PSRAM/internal
heap/stack, save latency, responsiveness and reboot persistence remain pending the
approved combined session. Host tests use synthetic vectors and a protocol checksum;
firmware uses pinned mbedTLS SHA256.
