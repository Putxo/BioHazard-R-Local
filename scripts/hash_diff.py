#!/usr/bin/env python3
"""Print SHA-256 and contiguous byte-difference ranges without modifying files."""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

def diff_ranges(a: bytes, b: bytes):
    n = min(len(a), len(b))
    start = None
    for i in range(n):
        if a[i] != b[i]:
            if start is None:
                start = i
        elif start is not None:
            yield start, i
            start = None
    if start is not None:
        yield start, n
    if len(a) != len(b):
        yield n, max(len(a), len(b))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("base", type=Path)
    ap.add_argument("other", type=Path, nargs="?")
    args = ap.parse_args()
    print(f"{args.base}: {sha256(args.base)}")
    if args.other is None:
        return
    print(f"{args.other}: {sha256(args.other)}")
    a, b = args.base.read_bytes(), args.other.read_bytes()
    ranges = list(diff_ranges(a, b))
    print(f"size base={len(a)} other={len(b)} delta={len(b)-len(a)}")
    print(f"ranges={len(ranges)}")
    print(f"different_bytes={sum(e-s for s,e in ranges)}")
    for s, e in ranges:
        print(f"0x{s:08X}-0x{e-1:08X} ({e-s} bytes)")

if __name__ == "__main__":
    main()
