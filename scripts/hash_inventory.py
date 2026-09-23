#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib
from pathlib import Path

def sha256(path: Path) -> str:
    h=hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda:f.read(1024*1024),b""):
            h.update(chunk)
    return h.hexdigest()

p=argparse.ArgumentParser()
p.add_argument("files",nargs="+",type=Path)
a=p.parse_args()
print("| File | Size | SHA-256 |")
print("|---|---:|---|")
for f in a.files:
    print(f"| {f.name} | {f.stat().st_size:,} | `{sha256(f)}` |")
