"""Pinned January Genesis camera and producer boundaries; never executes input."""
from pathlib import Path
import argparse,json
from audit_genesis_progress import Image
WITNESSES=[
    (0x02823A53,"8b45f8c7007085da04","Target destructor revocation before owner clear"),
    (0x02B25D8B,"e88d1613ff","Scanner camera call"),
    (0x02B25A42,"894df8","Aligned scanner method saves widget at EBP-8"),
    (0x02B25D90,"8945a4837da4000f8459010000","Caller null check"),
    (0x01CA5DDE,"a13c9d7905","Camera singleton"),
    (0x01F10786,"8b80e00c0000","Primary camera slot"),
    (0x01F107C6,"8b80e40c0000","Secondary camera slot"),
    (0x0205F2DE,"c7001c2bcf04","CameraManage vtable"),
    (0x02066AE4,"8b45f88b4d08894874","CameraManage actor target"),
    (0x0281FB8A,"e88e7843ff","Global detector still selects primary camera"),
    (0x02820716,"e80f513eff","Candidate feed"),
    (0x028207B9,"e878fc44ff","Detector status feed"),
    (0x0281F539,"894824","Per-target focus state write"),
    (0x0281F586,"8b4024","Per-target focus state read"),
    (0x0281B429,"894820","Per-target persistent state write"),
    (0x028232A6,"8b4020","Per-target persistent state read"),
    (0x0281DE07,"8b4df883c110e80ba33bff","Per-target position update"),
    (0x0281ADCE,"c7007085da04","Target constructor"),
    (0x0281AE26,"8b45f85f5e5b81c4cc0000003bece8837e46ff8be55d","Target constructor epilogue"),
    (0x02823AA8,"e808df46ff","Unconditional target base cleanup"),
    (0x02823AA0,"e8d40539ff","Stock scanner target removal"),
    (0x02B2A14F,"c20400","Native removal ret4"),
    (0x028207AC,"8b8584feffff508b8dd4fcffff","Status integer argument from EBP-17C"),
 ]
def audit(data):
    im=Image(data)
    for a,h,label in WITNESSES:
        b=bytes.fromhex(h)
        if im.read(a,len(b))!=b:raise ValueError(label)
    return {"status":"PASS_STATIC_EVIDENCE_ONLY","checks":len(WITNESSES),"gameplay_executed":False}
if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("image",type=Path);a=p.parse_args()
    print(json.dumps(audit(a.image.read_bytes()),indent=2))
