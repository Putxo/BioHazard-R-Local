#!/usr/bin/env python3
"""Read-only January SubEquip witnesses; never loads or executes engine code."""
import argparse
import json
from pathlib import Path
import struct
from audit_hud_lifetime import Image

def audit(data):
    image=Image(data);checks=[]
    def exact(va,expected,label):
        if image.read(va,len(expected))!=expected:raise ValueError(label)
        checks.append({'va':hex(va),'label':label})
    def rel(va,target,label,op=0xE8):
        exact(va,bytes([op])+struct.pack('<i',target-va-5),label)
    vt=0x04DE57B4
    col=image.u32(vt-4);td=image.u32(col+12)
    exact(td+8,b'.?AVuGUI_SubEquipWin@gui@game@app@@\0','SubEquip RTTI')
    chd=image.u32(col+16);count=image.u32(chd+8);array=image.u32(chd+12)
    if not 1<=count<=16:raise ValueError('base count')
    bases=[image.cstr(image.u32(image.u32(array+4*i))+8) for i in range(count)]
    if '.?AVuBioCockpitGUI@gui@game@app@@' not in bases:raise ValueError('cockpit inheritance')
    for slot,thunk,body in ((0,0x01C32956,0x02B42BA0),(5,0x01C7FD6E,0x02B42C80),
            (8,0x01BA1E2E,0x02B42F60),(9,0x01BF295F,0x02CD27E0),(11,0x01C8373E,0x02B43110)):
        exact(vt+4*slot,struct.pack('<I',thunk),'vslot '+str(slot))
        rel(thunk,body,'virtual implementation '+str(slot),0xE9)
    exact(0x02B4899C,bytes.fromhex('6a106870030000'),'allocation alignment and size')
    rel(0x02B489A3,0x01B8BCE6,'allocator')
    rel(0x02B489C0,0x01C39C88,'constructor call')
    rel(0x01C39C88,0x02B42A90,'constructor thunk',0xE9)
    exact(0x02B42AC0,bytes.fromhex('c700b457de04'),'constructor vtable')
    exact(0x02B489E0,bytes.fromhex('894864'),'Cockpit +64 owner slot')
    exact(0x02B42F80,bytes.fromhex('894df8'),'widget at caller EBP-8')
    rel(0x02B42FA6,0x01C2C7F4,'Self actor finder')
    exact(0x02B42FAB,bytes.fromhex('8945ec837dec007505e9ea000000'),'null actor skips update')
    rel(0x02B42FBC,0x01BCC327,'actor pack')
    exact(0x02B42FD5,bytes.fromhex('8b4df88881a0020000'),'state stored in widget instance')
    rel(0x02B4309E,0x01C30737,'forward actor into subweapon update')
    exact(0x02B43189,bytes.fromhex('c20400'),'draw consumes context')
    return {'status':'PASS','witnesses':checks,'bases':bases,'gameplay_executed':False}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path);a=p.parse_args()
    print(json.dumps(audit(a.image.read_bytes()),indent=2))
