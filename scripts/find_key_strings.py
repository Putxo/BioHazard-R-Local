#!/usr/bin/env python3
"""Reconnaissance scanner. A string hit is a clue, not proof of semantics."""
from __future__ import annotations
import argparse
import re
from pathlib import Path

PATTERNS = [
    rb"CreateRaidPlayer", rb"ForceTwoPlatoon", rb"NotSendPad",
    rb"PickingCoopLock", rb"UseOtomo", rb"mSelfID", rb"mPartnerID",
    rb"mPadNo", rb"mPadViewportNo", rb"mStartPadNo", rb"mCameraIdx",
    rb"switchCamera", rb"Player 2", rb"Player2", rb"for2P", rb"Pad2",
    rb"uPcsPlayerMain", rb"uPcsPlayerSub0", rb"uPcsPlayerSub1",
    rb"mMovePcs", rb"mMoveSubPcs", rb"VIEW_[0-7]",
    rb"REGION_(?:FULLSCREEN|TOP|BOTTOM|LEFT|RIGHT|TOPLEFT|TOPRIGHT|BOTTOMLEFT|BOTTOMRIGHT)",
]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("binary", type=Path)
    args = ap.parse_args()
    data = args.binary.read_bytes()
    rx = re.compile(b"|".join(b"(?:"+p+b")" for p in PATTERNS), re.I)
    for m in rx.finditer(data):
        start = m.start()
        end = data.find(b"\x00", start, min(start+256, len(data)))
        if end < 0:
            end = min(start+256, len(data))
        print(f"0x{start:08X} {data[start:end].decode('ascii', errors='replace')}")

if __name__ == "__main__":
    main()
