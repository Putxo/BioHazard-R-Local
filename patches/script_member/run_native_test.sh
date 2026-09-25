#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
as --32 "$here/selector.S" -o "$work/selector.o"
objcopy -O binary -j .text "$work/selector.o" "$work/selector.bin"
echo '5fa7f66b5524bd35bb297745bbd32854db8a43f48d17d52b12de2d2b54531433  '"$work/selector.bin" | sha256sum -c -
g++ -m32 -std=c++17 -Os -ffreestanding -fno-exceptions -fno-rtti -fno-pie -fno-pic \
    -fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables \
    -fno-builtin -mno-sse -mno-mmx -mpreferred-stack-boundary=2 \
    -mincoming-stack-boundary=2 -c "$here/native_test.cpp" -o "$work/test.o"
as --32 "$here/native_bridge.S" -o "$work/bridge.o"
ld -m elf_i386 -T "$here/native_link.ld" -o "$work/native_test" \
    "$work/test.o" "$work/bridge.o" "$work/selector.o"
if [[ "${1:-}" == --compile-only ]]; then
    echo 'COMPILED ONLY: i386 test executable was not run'
else
    "$work/native_test"
fi
