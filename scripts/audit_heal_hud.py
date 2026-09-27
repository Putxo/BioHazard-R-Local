#!/usr/bin/env python3
"""Read-only January heal-event ownership and native activation witnesses."""
import argparse, json
from pathlib import Path
from audit_hud_lifetime import Image

WITNESSES = [
    (0x0279900E, "e859fb4eff", "Original heal activation call"),
    (0x0279907A, "e8edfa4eff", "Original heal activation call"),
    (0x027999CF, "e898f14eff", "Original heal activation call"),
    (0x027ACE72, "e8f5bc4dff", "Original heal activation call"),
    (0x027B211C, "e84b6a4dff", "Original heal activation call"),
    (0x027B21E9, "e87e694dff", "Original heal activation call"),
    (0x02798F20, "894df8", "Caller this is EBP-8"),
    (0x02799890, "894df8", "Caller this is EBP-8"),
    (0x027ACDD0, "894df8", "Caller this is EBP-8"),
    (0x027B2090, "894df8", "Caller this is EBP-8"),
    (0x027F16A3, "8b45f883b8400e000000750432c0eb0d8b45f883b8400e0000030f95c05f5e5b", "Gate admits Pad (rejects ThinkMode0 and3)"),
    (0x02799163, "8b45f88b40445f5e5b8be55dc3", "Heal getter returns cockpit slot44"),
    (0x02B48674, "6a1068b0020000e842db0fff", "Heal allocation size alignment and allocator"),
    (0x02B48698, "e8ef200aff", "Heal constructor call"),
    (0x02B486B8, "894844", "Heal stock cockpit slot"),
    (0x02B1E4C3, "8b45f88b88a00200008b55f88b82a00200008b118bf48bc88b421cffd0", "Activation restarts owned animation"),
    (0x02B1E4E7, "6a018b4df8e8703d0dff6a018b4df8e8a29415ff", "Activation enables update and visibility"),
    (0x02B1E50E, "c3", "Activation takes no stack arguments"),
    (0x02B1E37B, "8b45f88b88a0020000894dec8b4dece8", "Phase8 checks own animation time"),
    (0x02B1E3BF, "6a008b4df8e8d49515ff6a008b4df8e88e3e0dff", "Phase8 hides completed effect"),
]

def audit(data):
    im=Image(data)
    for a,h,label in WITNESSES:
        b=bytes.fromhex(h)
        if im.read(a,len(b))!=b:raise ValueError(label)
    return {"status":"PASS_STATIC_EVIDENCE_ONLY","checks":len(WITNESSES),
            "gameplay_executed":False}

if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("image",type=Path);a=p.parse_args()
    print(json.dumps(audit(a.image.read_bytes()),indent=2))
