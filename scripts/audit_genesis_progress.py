#!/usr/bin/env python3
"""January scanner progress/actor boundaries; read-only original-image audit."""
import argparse,json
from pathlib import Path
from audit_hud_lifetime import Image
WITNESSES = [
    (0x02B1FDAA, "e8b32506ff", "Read progress call"),
    (0x02B20DF8, "e8651506ff", "Read progress call"),
    (0x02B21038, "e8251306ff", "Read progress call"),
    (0x02B211EF, "e86e1106ff", "Read progress call"),
    (0x02B212B8, "e8a51006ff", "Read progress call"),
    (0x02B216DC, "e8810c06ff", "Read progress call"),
    (0x02B217BC, "e8a10b06ff", "Read progress call"),
    (0x02B218DC, "e8810a06ff", "Read progress call"),
    (0x02B21F91, "e8cc0306ff", "Read progress call"),
    (0x02B21FC4, "e8990306ff", "Read progress call"),
    (0x02B22360, "e8fdff05ff", "Read progress call"),
    (0x02B260B1, "e8acc205ff", "Read progress call"),
    (0x02B269A9, "e8b4b905ff", "Read progress call"),
    (0x02B2788C, "e8d1aa05ff", "Read progress call"),
    (0x02B27A2F, "e82ea905ff", "Read progress call"),
    (0x02B27AEE, "e86fa805ff", "Read progress call"),
    (0x02B27C06, "e857a705ff", "Read progress call"),
    (0x02B27C72, "e8eba605ff", "Read progress call"),
    (0x02B28C41, "e81c9705ff", "Read progress call"),
    (0x02B212A7, "e85b910dff", "Set progress call"),
    (0x02B27C6A, "e898270dff", "Set progress call"),
    (0x02B28C39, "e8c9170dff", "Set progress call"),
    (0x02B268DB, "e824bb08ff", "Add progress call"),
    (0x02B2699E, "e861ba08ff", "Add progress call"),
    (0x02B26A42, "e8bdb908ff", "Add progress call"),
    (0x02B26A79, "e886b908ff", "Add progress call"),
    (0x02A94F53, "8b45f88b40745f5e5b8be55d", "Getter manager+74"),
    (0x02EB3933, "837d0864770b8b4508898530ffffffeb0ac78530ffffff640000008b4df88b9530ffffff895174", "Setter unsigned clamp"),
    (0x02EB39A3, "8b45f88b4874034d088b55f8894a748b45f883787464760a8b45f8c7407464000000", "Add wraps then clamps"),
    (0x02EB39CB, "c20400", "Add ret4"),
    (0x02EB3960, "c20400", "Set ret4"),
    (0x02B1FD22, "894df8", "Scanner this stored at EBP-8"),
    (0x02B20D90, "894df8", "Scanner this stored at EBP-8"),
    (0x02B211E0, "894df8", "Scanner this stored at EBP-8"),
    (0x02B21630, "894df8", "Scanner this stored at EBP-8"),
    (0x02B259D0, "894df8", "Scanner this stored at EBP-8"),
    (0x02B276E0, "894df8", "Scanner this stored at EBP-8"),
    (0x02B28B20, "894df8", "Scanner this stored at EBP-8"),
    (0x02B214B8, "e837b310ff", "Additional Self dependency"),
    (0x02B2203D, "e8b2a710ff", "Additional Self dependency"),
    (0x02B2246B, "e884a310ff", "Additional Self dependency"),
    (0x02B225BB, "e834a210ff", "Additional Self dependency"),
    (0x02B27905, "e8ea4e10ff", "Additional Self dependency"),
    (0x02B27C3D, "e8b24b10ff", "Additional Self dependency"),
    (0x02B21003, "e839630eff", "Selected-character actor dependency"),
    (0x02B21244, "e8f8600eff", "Selected-character actor dependency"),
    (0x02B2178B, "e8b15b0eff", "Selected-character actor dependency"),
    (0x02B218AB, "e8915a0eff", "Selected-character actor dependency"),
    (0x02B224A5, "e8974e0eff", "Selected-character actor dependency"),
    (0x02B28C88, "e8b4e60dff", "Selected-character actor dependency"),
 ]
def audit(data):
    im=Image(data)
    for address,expected,label in WITNESSES:
        b=bytes.fromhex(expected)
        if im.read(address,len(b))!=b:raise ValueError(label)
    return {"status":"PASS_STATIC_EVIDENCE_ONLY","checks":len(WITNESSES),"gameplay_executed":False}
if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("image",type=Path);a=p.parse_args()
    print(json.dumps(audit(a.image.read_bytes()),indent=2))
