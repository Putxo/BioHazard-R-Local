#!/usr/bin/env python3
"""Hash-pinned read-only evidence; does not install or execute any game code."""
import argparse
import json
from pathlib import Path
import struct
from audit_hud_lifetime import Image

def audit(data):
    image=Image(data);records=[]
    def exact(va,expected,label):
        if image.read(va,len(expected))!=expected:raise ValueError(label)
        records.append({'va':hex(va),'label':label})
    def branch(va,target,label):
        exact(va,b'\xe8'+struct.pack('<i',target-va-5),label)
    for vt,name in ((0x04CDCA9C,b'.?AVuActionIcon2D@action_command@game@app@@'),
                    (0x04DE28CC,b'.?AVuGUI_ActionIcon@gui@game@app@@')):
        td=image.u32(image.u32(vt-4)+12)
        exact(td+8,name+b'\0','RTTI '+name.decode())
    for va,h,label in (
        (0x01EB6F7E,'c7009ccacd04','producer vtable'),
        (0x01EB6F84,'8b45f88b4d08894840','constructor argument stored at +40'),
        (0x01EB6FA2,'8b4df883c150','embedded ActionIcon at +50'),
        (0x01EB6FAD,'8b4df881c1f0020000','second embedded widget at +2F0'),
        (0x01EB6FF7,'c20400','producer constructor ret 4'),
        (0x01EB705F,'c7404000000000','destructor clears owner'),
        (0x01EB740D,'6a00','first arbitration context index 0'),
        (0x01EB7452,'6a00','second arbitration context index 0'),
        (0x01EB746E,'6a016a00','claim flag and context index 0'),
        (0x01EB75D6,'8b8070010000','arbitration +170 getter'),
        (0x01EB7666,'8a8074010000','arbitration +174 getter'),
        (0x01EB7619,'888874010000','arbitration +174 setter'),
        (0x01EB7744,'0fb645cb508b4dec51','show accepts icon ID and boolean'),
        (0x02B14247,'c20800','show ret 8'),
        (0x02B13C89,'c78090020000ffffffff','GUI +290 is icon state, not Cockpit next'),
    ):exact(va,bytes.fromhex(h),label)
    for va,target,label in (
        (0x01EB6FA8,0x01C220F6,'construct embedded GUI'),
        (0x01EB707A,0x01B9F8C7,'destroy embedded GUI'),
        (0x01EB740F,0x01C54A3D,'first arbitration lookup'),
        (0x01EB7454,0x01C54A3D,'second arbitration lookup'),
        (0x01EB7472,0x01C54A3D,'claim arbitration lookup'),
        (0x01EB7753,0x01C10D2E,'show embedded icon'),
    ):branch(va,target,label)
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':len(records),'records':records,
            'action_icon_patched':False,'gameplay_executed':False}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path);a=p.parse_args()
    print(json.dumps(audit(a.image.read_bytes()),indent=2))
