#!/usr/bin/env python3
"""
Apply the Native Input v3 experiment to the exact 30-Jan-2013 FullDebug EXE.

No proprietary bytes are stored beyond the tiny expected/patched instruction
sequences. The script refuses to patch an unexpected source SHA or unexpected
original bytes.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path

SOURCE_SHA256 = "9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69"
OUTPUT_SHA256 = "9de444e104f2846aa166e1460f33a110ffac5b880f374edfd31b94aef3e5c649"

PATCHES = [
    (
        0x00C280E4,
        bytes.fromhex("83f8010f8593030000"),
        bytes.fromhex("e92f06000090909090"),
        "ThinkMode dispatch hook"
    ),
    (
        0x00C28718,
        bytes.fromhex("cc"*51),
        bytes.fromhex(
            "83f801742483f8030f85200000008b4df851e8631e41ff84c0"
            "0f850f0000006a018b4df8e81f7841ffe9a7f9ffffe935fdffff"
        ),
        "Partner Cpu->Pad native routing cave"
    ),
    (
        0x00C29BB0,
        bytes.fromhex("558bec81eccc00000053565751"),
        bytes.fromhex("51e8dc0941ff0fb6c083f001c3"),
        "Self=pad0 / Partner=pad1 selector"
    ),
]

def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()

    data = bytearray(args.source.read_bytes())
    source_hash = sha256_bytes(data)
    if source_hash != SOURCE_SHA256:
        raise SystemExit(
            f"Refusing to patch: source SHA-256 {source_hash} != expected {SOURCE_SHA256}"
        )

    for offset, expected, replacement, label in PATCHES:
        got = bytes(data[offset:offset+len(expected)])
        if got != expected:
            raise SystemExit(
                f"Refusing to patch {label}: bytes at 0x{offset:08X} differ from expected"
            )
        if len(expected) != len(replacement):
            raise SystemExit(f"Internal patch-length mismatch: {label}")
        data[offset:offset+len(replacement)] = replacement

    output_hash = sha256_bytes(data)
    if output_hash != OUTPUT_SHA256:
        raise SystemExit(
            f"Patched SHA-256 {output_hash} != expected {OUTPUT_SHA256}"
        )

    args.output.write_bytes(data)
    print(f"Wrote {args.output}")
    print(f"SHA-256 {output_hash}")

if __name__ == "__main__":
    main()
