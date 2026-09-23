#!/usr/bin/env python3
from __future__ import annotations
import argparse
from pathlib import Path

def num(s:str)->int: return int(s,0)

p=argparse.ArgumentParser()
p.add_argument("binary",type=Path)
p.add_argument("markers",nargs="+")
p.add_argument("--raw-base",type=num)
p.add_argument("--va-base",type=num)
a=p.parse_args()
if (a.raw_base is None)!=(a.va_base is None):
    p.error("--raw-base and --va-base must be supplied together")
data=a.binary.read_bytes()
for marker in a.markers:
    needle=marker.encode("ascii")
    pos=0; found=False
    while True:
        pos=data.find(needle,pos)
        if pos<0: break
        found=True
        tail=""
        if a.raw_base is not None and pos>=a.raw_base:
            va=a.va_base+(pos-a.raw_base)
            tail=f" VA=0x{va:08X}"
        print(f"{marker!r}: raw=0x{pos:08X}{tail}")
        pos+=1
    if not found:
        print(f"{marker!r}: NOT FOUND")
