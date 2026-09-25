#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
as --32 "$here/selector.S" -o "$work/selector.o"
objcopy -O binary -j .text "$work/selector.o" "$work/selector.bin"
echo '500a82d8071f5c40ece76397fee1bac97888768a3e9029d52222d4d933eb7896  '"$work/selector.bin" | sha256sum -c -
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
