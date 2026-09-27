"""Exact-image effect ownership and root view-mask witnesses. Read-only."""
import argparse,json
from pathlib import Path
from audit_hud_native_ops import Image
from audit_genesis_effect_lifetime import witnesses as lifetime_witnesses

def witnesses():
    yield from lifetime_witnesses()
    for at,data,label in (
        (0x02B29668,'8b45f805f402000050','FilterSet callback is Scanner+2F4'),
        (0x02B2C979,'894804','Callback stores parent at +4'),
        (0x03477537,'83e2f883ca01','cUnit constructor state1'),
        (0x0326A2A7,'0fb6450883e07fc1e003','Scheduler encodes seven-bit group'),
        (0x0326A2B7,'81e207fcffff0bd0','Scheduler preserves non-group flags'),
        (0x01EB6156,'25ff030000c1e010','View mask has ten bits at bit16'),
        (0x01EB6164,'81e2ffff00fc0bd0','View setter preserves all other fields'),
    ):
        yield at,bytes.fromhex(data),label

def audit(data):
    im=Image(data);count=0
    for at,expected,label in witnesses():
        if im.read(at,len(expected))!=expected:raise ValueError(f'{label} at {at:08X}')
        count+=1
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':count,'gameplay_executed':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path)
    print(json.dumps(audit(p.parse_args().image.read_bytes()),indent=2))
