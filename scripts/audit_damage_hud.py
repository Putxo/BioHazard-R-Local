#!/usr/bin/env python3
"""Read-only, SHA-pinned evidence for the separate ordinary-damage widget."""
import argparse, json
from pathlib import Path
from audit_hud_lifetime import Image

WITNESSES = [
    (0x02B4859D, "6a1068b0020000e8a95704ff", "Damage allocation size alignment and allocator"),
    (0x02B485C1, "e81ced06ff", "Damage constructor"),
    (0x02B485E1, "89483c", "Cockpit damage slot"),
    (0x02B1C618, "e8582a0dff", "Damage cockpit-GUI base constructor"),
    (0x02B1C620, "c7007c35de04", "Damage vtable"),
    (0x02B7973C, "c7809002000000000000", "Base detached link"),
    (0x02B79749, "c6809402000000", "Base force-skip default"),
    (0x02B49584, "e86b320eff", "Original manager reads Self"),
    (0x02B495C3, "e8610c09ff", "Manager actor-state wrapper"),
    (0x02B495CA, "e884fd0bff83f8047546", "State comparison selects choking branch"),
    (0x02B495D4, "6a008b45f88b483ce8bce312ff", "State4 hides ordinary damage"),
    (0x02B4961A, "6a018b45f88b483ce876e312ff", "Other states enable ordinary damage"),
    (0x02B49640, "e8894903ffd9e8dee1d80dc4fccb04", "Health ratio complemented and scaled"),
    (0x02B4964F, "d9bdd2feffff0fb785d2feffff0d000c00008985ccfeffffd9adccfeffffdfbdc4feffffd9add2feffff8b85c4feffff", "Native truncating x87 conversion with CW restore"),
    (0x02B49683, "8b493ce8a87814ff", "Native damage setter"),
    (0x01D46393, "8b45f88b88280f00008b5508890a8b45085f5e5b", "Actor-state return wrapper contains one word"),
    (0x027A07A3, "8b4df881c1a4180000e8929c45ff8bc8e8e6dc40ff", "Health component and ratio getter"),
    (0x027A0803, "8b45f8d94018", "Normalized health ratio return"),
    (0x02B1CAE6, "e8b13b07ff3945087439", "Percent setter skips unchanged value"),
    (0x02B1CB03, "dfad2cffffff51d91c248b4df88b91a0020000528b4df8e82cd914ff8b45f8", "Percent drives widget node and dirty flag"),
    (0x02B1C923, "8b45f80fb688a402000083f90175128b45f8c680a4020000008b4df8e81b600dff", "Damage phase9 consumes dirty flag"),
    (0x02B1C9C2, "e833d60fff", "Damage draw invokes base GUI"),
    (0x02B1C9DA, "c20400", "Draw consumes context argument"),
    (0x04DE357C, "29cac201", "Damage virtual slot0"),
    (0x04DE3590, "2e8db901", "Damage virtual slot5"),
    (0x04DE359C, "91e5c601", "Damage virtual slot8"),
    (0x04DE35A0, "abefbb01", "Damage virtual slot9"),
    (0x04DE35A8, "759ebb01", "Damage virtual slot11"),
    (0x01B8DD52, "e929e5f800", "Pinned native thunk"),
    (0x01BB72E2, "e90953f600", "Pinned native thunk"),
    (0x01C2CA29, "e982fcee00", "Pinned native thunk"),
    (0x01B98D2E, "e94d3af800", "Pinned native thunk"),
    (0x01C6E591, "e91ae3ea00", "Pinned native thunk"),
    (0x01BBEFAB, "e950d9f500", "Pinned native thunk"),
    (0x01BB9E75, "e9f62af600", "Pinned native thunk"),
    (0x01BDA229, "e942c11600", "Pinned native thunk"),
    (0x01C09353, "e9a8d81300", "Pinned native thunk"),
    (0x01B7DFCE, "e9ad27c200", "Pinned native thunk"),
    (0x01C90F33, "e988bbe800", "Pinned native thunk"),
    (0x01C7799D, "e9be3aea00", "Pinned native thunk"),
]

def audit(data):
    im=Image(data)
    for a,h,label in WITNESSES:
        b=bytes.fromhex(h)
        if im.read(a,len(b))!=b:raise ValueError(label)
    return {"status":"PASS_STATIC_EVIDENCE_ONLY","checks":len(WITNESSES),
            "gameplay_executed":False,"ordinary_damage_only":True}

if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("image",type=Path);a=p.parse_args()
    print(json.dumps(audit(a.image.read_bytes()),indent=2))
