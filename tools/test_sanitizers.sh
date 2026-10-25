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
