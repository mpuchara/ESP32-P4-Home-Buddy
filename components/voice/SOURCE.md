# Voice numerical source attribution

The checked filterbank initialization and preprocessing sequence adapt Espressif
ESP-DL speaker_verification 0.3.1 at eb72563e6a4e61d2bbc0d46e4c79f2c9f816c3c6,
models/speaker_verification/speaker_verification.cpp and ESP-DL 3.3.14
(audio/common/dl_audio_common.cpp, audio/speech_features/dl_fbank.cpp).
Upstream numerical code/weights: MIT; retained notice in
../face/vendor/ESP-DL-MIT.txt and included in USB packages as licenses/ESP-DL-MIT.txt.
Application ownership, safety gates, synthetic tests and RAM provider are Apache-2.0.
The original convenience Fbank constructor is not used because some allocations
are unchecked; the local adaptation validates each scratch/mel allocation.
The pinned dl_fft implementation supplies FFT primitives without modification.
See docs/VOICE_RECOGNITION.md for exact source links and acceptance boundaries.
