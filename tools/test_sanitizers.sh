#!/usr/bin/env bash
set -eu
build_dir="${RUNNER_TEMP:-/tmp}/home-buddy-sanitizer-tests"
mkdir -p "$build_dir"
cc -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
 -Icomponents/identity/include tests/test_identity.c components/identity/identity_core.c \
 components/identity/enrollment_flow.c -lm -o "$build_dir/identity"
"$build_dir/identity"
cc -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
 -Icomponents/audio/include -Itests/ha_platform tests/test_audio.c components/audio/audio_pcm.c \
 -lm -o "$build_dir/audio"
"$build_dir/audio"

cc -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
 -Icomponents/voice/include -Icomponents/identity/include tests/test_voice.c components/voice/voice_core.c \
 components/identity/identity_fusion.c -lm -o "$build_dir/voice"
"$build_dir/voice"

cc -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icomponents/voice/include \
 -c components/voice/voice_core.c -o "$build_dir/voice_core.o"
c++ -std=c++17 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
 -DHB_VOICE_HOST_TEST -Icomponents/voice/include -Itests -Itests/ha_platform \
 tests/test_voice_adapter.cpp components/voice/voice_embedding.cpp "$build_dir/voice_core.o" -lm -o "$build_dir/voice_adapter"
"$build_dir/voice_adapter"

cc -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
 -DHB_AUDIO_HOST_TEST -Icomponents/audio/include -Itests -Itests/ha_platform \
 tests/test_audio_worker.c components/audio/audio_pcm.c -lm -o "$build_dir/audio_worker"
"$build_dir/audio_worker"
