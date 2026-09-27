#!/usr/bin/env python3
"""Read-only January Genesis ownership boundaries; no runtime modifications."""
import argparse, json
from pathlib import Path
from audit_hud_lifetime import Image

WITNESSES = [
    (0x02B486D9, "6a106800040000e8a5f504ff", "Scanner allocation 0x400 alignment16"),
    (0x02B486FD, "e8c67b13ff", "Scanner constructor call"),
    (0x02B4871D, "894848", "Cockpit scanner slot48"),
    (0x01F134F3, "8b45f88b40485f5e5b8be55dc3", "Getter returns cockpit slot48"),
    (0x0279CFAD, "e893d74aff8bc8e864a44bff3bf07405e9580100", "Player visibility compares selected main player identity"),
    (0x0279D0BD, "8b45ec506a008b4de0e8a8913eff8bc8e89c1740ff", "Scanner activation receives weapon and zero argument"),
    (0x0279D0FD, "8b4de0e86e913eff8bc8e8131d4dff", "Scanner close path"),
    (0x02B20D93, "8b45f88b4d0c8988fc020000", "Activation stores weapon argument at scanner+2fc"),
    (0x02B21490, "8b45f8c780fc02000000000000", "Close clears bound weapon"),
    (0x02B214B8, "e837b310ff", "Close queries global Self"),
    (0x02B214C6, "6a008b4da4e877e508ff", "Close mutates the resolved actor"),
    (0x01D5B4D2, "837d0801722a", "Scan manager accessor expects index below1"),
    (0x01D5B50B, "8b45088d048550305605", "Scan manager singleton table"),
    (0x02A94F53, "8b45f88b40745f5e5b8be55dc3", "Progress getter reads manager+74"),
    (0x02B211E3, "6a00e8b29b05ff83c4048bc8e86e1106ff83f864", "Close tests singleton progress equals100"),
    (0x02B21230, "e8304407ff8bc8e8264716ff50e8234407ff8bc8e8f8600eff", "Reward obtains globally selected character actor"),
    (0x02B2127A, "6a018b4dece8a3b00aff8bc8e8d7d905ff", "Reward calls resolved actor pack with one"),
    (0x02B21299, "6a006a00e8fa9a05ff83c4048bc8e85b910dff", "Close resets singleton progress"),
    (0x02B1F7E6, "6a018b4df8e8058410ff8b4df8898180030000", "Scanner caches native GUI element"),
    (0x02B1F8F0, "6a018b4df8e8e4b206ff8b4df88981b8030000", "Scanner caches native GUI animation"),
    (0x01F13310, "e85e2fc7ff", "Verified scanner getter caller"),
    (0x02658129, "e845e152ff", "Verified scanner getter caller"),
    (0x0279D0C6, "e8a8913eff", "Verified scanner getter caller"),
    (0x0279D0EA, "e884913eff", "Verified scanner getter caller"),
    (0x0279D100, "e86e913eff", "Verified scanner getter caller"),
    (0x0281F0CA, "e8a47136ff", "Verified scanner getter caller"),
    (0x028206F5, "e8795b36ff", "Verified scanner getter caller"),
    (0x02820738, "e8365b36ff", "Verified scanner getter caller"),
    (0x02823A8B, "e8e32736ff", "Verified scanner getter caller"),
    (0x02B07432, "e83cee07ff", "Verified scanner getter caller"),
    (0x01B86273, "e958d23800", "Native thunk"),
    (0x01B7AD9C, "e91f061e00", "Native thunk"),
    (0x01B883CF, "e9dc301d00", "Native thunk"),
    (0x01B82362, "e9c92bf100", "Native thunk"),
    (0x01C07341, "e9fac41901", "Native thunk"),
    (0x01BAFA47, "e90454c000", "Native thunk"),
]

def audit(data):
    im=Image(data)
    for a,h,label in WITNESSES:
        b=bytes.fromhex(h)
        if im.read(a,len(b))!=b:raise ValueError(label)
    return {"status":"PASS_STATIC_EVIDENCE_ONLY","checks":len(WITNESSES),
            "gameplay_executed":False,"genesis_owner_routing_implemented":False}

if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("image",type=Path);a=p.parse_args()
    print(json.dumps(audit(a.image.read_bytes()),indent=2))
