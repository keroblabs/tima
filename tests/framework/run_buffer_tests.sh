#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/tima-fwk-buffer.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM

cc -std=c99 -Wall -Wextra -Werror \
    -I"$repo_root/tests/framework/stubs" \
    -I"$repo_root/framework" \
    "$repo_root/framework/buffer.c" \
    "$repo_root/tests/framework/test_buffer.c" \
    -o "$test_binary"
"$test_binary"
printf 'buffer tests passed\n'
