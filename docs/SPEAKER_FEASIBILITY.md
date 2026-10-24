# Local speaker model feasibility, 2026-10-24

Research only. No voice model is flashed/run, no voice identity or BOTH event.
Physical microphone acceptance precedes any speaker benchmark.

Primary source inspected: Espressif esp-dl
[eb72563e6a4e61d2bbc0d46e4c79f2c9f816c3c6](https://github.com/espressif/esp-dl/tree/eb72563e6a4e61d2bbc0d46e4c79f2c9f816c3c6/models/speaker_verification),
component 0.3.1. It declares esp-dl ~3.3.14; locked 3.3.14 contains dl_audio_wav
and dl_fbank. This is source feasibility, not P4 revision-1.0 runtime acceptance.
No production-only toolchain/dependency upgrade is introduced.

Raw 3s P4 asset: 2,844,528 bytes. Prior single-model package assessment:
2,844,624 bytes; sv_model_3s capacity 3,145,728 bytes; margin 301,104 bytes.
Storage fit is not operator compatibility or measured heap. Upstream flash defaults
include BOTH 3s and 6s, incompatible with our layout. Select only 3s, pin digest,
validate package and use USB preflight before a future authorized write.

PCM API: mono int16, 16000 Hz; 80-bin log filterbanks, Hamming 25 ms windows,
10 ms shift, low frequency 20 Hz, per-bin mean subtraction and int16 quantization
using tensor exponent. Frame count and output dimension are read from tensors.
For 300 feature frames: (300-1)*160+400 = 48,240 samples (~3.015 s), not 48,000;
short input is padded, long input center-cropped. Derived float audio scratch:
192,960 bytes; feature scratch: 96,000 bytes. Add PCM, model/runtime/FFT/output.
These array sizes are not measured peak PSRAM/internal RAM. Record actual model
input/output dtypes, shapes and embedding dimension before defining a voice schema.
Audit allocation/tensor failures before using the upstream convenience class.

The API computes embeddings and pairwise cosine similarity, not validated open-set
household identification. A provider needs separate voice templates per stable ID,
explicit fresh enrollment, diverse utterances, versioned namespace, calibrated
acceptance/different-person margin, UNKNOWN/AMBIGUOUS and cancellation. Do not reuse
face thresholds. Future UI contract: Domownicy > Rejestruj głos; unavailable until
these gates. No household-specific off-device training.

After microphone acceptance, a separate diagnostic may load only sv_model_3s under
the compute mutex. Measure construction/unload cycles, free/largest/minimum RAM,
arena peak, preprocessing/inference/stack/CPU and simultaneous face/JPEG/audio.
Start resource tests with non-personal signal; real voice enrollment needs consent.
Bound windows, free returned embeddings, reject wrong rates/empty input and test
cancellation/allocation failure. Upstream latency/EER uses different clocks/PSRAM
and a verification dataset, not this board or multi-person accuracy.

Fusion is architecture only in hb_identity_future.h: independent FACE/VOICE
confirmation; same ID may yield BOTH only after both providers are validated;
distinct IDs yield CONFLICT. Preserve a sole reliable modality, otherwise return
UNKNOWN/AMBIGUOUS. Never weight incomparable raw scores or assume the visible face
owns a concurrent voice. Event bus still admits FACE only.

## Host asset inspection receipt

The pinned P4 3s asset was downloaded only to ignored local/iteration8, not to the
board. Raw SHA256: 736ca24bc3430db65b063ce4af7341904d1970d8505aa2eaab0d7be9e760c42e.
Read-only FlatBuffer inspection against the locked espdl.fbs schema gives input
`input`: int16 [1,300,80], output `embedding`: int8 [1,512]. Thus this asset has a
512-dimensional output; equal face/voice dimension does not imply comparable spaces.
The locked packer produces 2,844,624-byte PDL3; full-file SHA256
b503ff049a0cd5c62b4c1ac337cf20b7de1549857a4d5329a1821cfb55e9b3a8; margin
301,104 bytes. The packer's printed package_sha256 is its internal integrity field,
not the full-file digest. No runtime loading/operator or resource acceptance follows
from parsing/packing; the future SV partition remains unchanged.
