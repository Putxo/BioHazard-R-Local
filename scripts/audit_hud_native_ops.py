#!/usr/bin/env python3
"""Hash-pinned, read-only January HUD/native-call witnesses. Never patches/executes the game."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
SHA = '9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
SIZE = 60_748_800
TYPES = (
 ('Reticle',0x04DE52C4,0x2E0,0x01C11706,0x02B3FDD0,0x01C19CCB,0x02B40140,
  (0x01C40C04,0x01B94A7B,0x01C7272C,0x01C7E437,0x01C6EF7D),
  (0x02B40220,0x02B402F0,0x02CD26C0,0x02B40480,0x02B40740),0x02B48F0F),
 ('MainEquipWin',0x04DE4C3C,0x370,0x01C15775,0x02B3AD10,0x01B7D1F0,0x02B3B080,
  (0x01C0C74C,0x01B96D58,0x01C65A9A,0x01BF295F,0x01C6445B),
  (0x02B3B190,0x02B3B280,0x02B3B670,0x02CD27E0,0x02B3BBA0),0x02B488D2),
 ('MapBaseAndHerb',0x04DE810C,0x2C0,0x01C7855A,0x02B61520,0x01C6392A,0x02B61890,
  (0x01BAB4F6,0x01C33D4C,0x01C28CAD,0x01BF295F,0x01C67381),
  (0x02B619A0,0x02B61A70,0x02B61D60,0x02CD27E0,0x02B624E0),0x02B68183),
)
class Image:
    def __init__(self,data: bytes):
        if len(data)!=SIZE or hashlib.sha256(data).hexdigest()!=SHA:
            raise ValueError('requires the exact original January executable')
        self.data=data;pe=struct.unpack_from('<I',data,0x3C)[0]
        if data[pe:pe+4]!=b'PE\0\0' or struct.unpack_from('<H',data,pe+4)[0]!=0x14C:
            raise ValueError('not PE32 i386')
        opt=pe+24;self.base=struct.unpack_from('<I',data,opt+28)[0]
        n=struct.unpack_from('<H',data,pe+6)[0];size=struct.unpack_from('<H',data,pe+20)[0]
        self.sections=[]
        for i in range(n):
            _,rva,raw_size,raw=struct.unpack_from('<4I',data,opt+size+i*40+8)
            self.sections.append((self.base+rva,raw_size,raw))
    def read(self,va,n):
        for start,size,raw in self.sections:
            if raw and n>=0 and start<=va and va+n<=start+size:
                return self.data[raw+va-start:raw+va-start+n]
        raise ValueError(f'no raw range at {va:#x}')

def audit(data):
    pe=Image(data);records=[]
    def exact(va,b,label):
        if pe.read(va,len(b))!=b:raise ValueError(f'{label} at {va:#x}')
        records.append(dict(va=hex(va),expected=b.hex(),label=label))
    def branch(va,to,jump=False,label='call'):
        exact(va,bytes([0xE9 if jump else 0xE8])+struct.pack('<i',to-va-5),label)
    for name,vt,size,alloc,alloc_impl,ctor,ctor_impl,slots,impls,site in TYPES:
        exact(site,b'\x6a\x10\x68'+struct.pack('<I',size),name+' allocation size/alignment')
        branch(site+7,alloc,label=name+' cdecl allocator call')
        exact(site+12,b'\x83\xC4\x08',name+' allocator caller cleanup')
        branch(alloc,alloc_impl,True,name+' allocator thunk')
        branch(ctor,ctor_impl,True,name+' constructor thunk')
        for slot,thunk,impl in zip((0,5,8,9,11),slots,impls):
            exact(vt+slot*4,struct.pack('<I',thunk),name+f' virtual {slot}')
            branch(thunk,impl,True,name+f' virtual {slot} implementation')
    for va,to in ((0x01C8C27B,0x01DC75E0),(0x01BF2BC6,0x0326A0D0),
                  (0x01C326DB,0x01CB25D0),(0x01BD52B5,0x01EB7280),(0x01C3E7A6,0x01EB72C0)):
        branch(va,to,True,'native helper thunk')
    for va,h,label in (
        (0x03477525,'81caf8030000','cUnit detached moveline sentinel'),
        (0x0347757E,'c7421400000000','cUnit next null'),
        (0x03477588,'c7401800000000','cUnit prev null'),
        (0x01CB25F6,'8b480cc1e90a83e13f83e1100f95c0','Active bit is 0x4000'),
        (0x0326AAF6,'8b45f03b45087506c645fb00','sUnit membership pointer comparison'),
        (0x0326AB1B,'0fb645fbf7d81bc083c001','membership true iff found'),
        (0x0326AB33,'c20400','membership thiscall one argument'),
        (0x01DC7616,'c3','sUnit singleton no stack arguments'),
        (0x02B49827,'8b45c88b108bf48b4dc88b4220ffd03bf4','cockpit phase8 no argument'),
        (0x02B49CA5,'8b45ec8b108bf48b4dec8b4224ffd03bf4','cockpit phase9 no argument'),
        (0x02B49E3C,'8b4508508b4de08b118b4de08b422cffd03bf4','draw one context argument'),
        (0x01EB72EB,'f30f11401c','unit time scale write'),
        (0x01EB72F6,'c20400','time scale setter one stack float'),
        (0x02B49A46,'8a8094020000','cockpit ForceSkip byte (not MapHerb)'),
        (0x02B401BA,'c3','Reticle constructor zero stack args'),
        (0x02B3B122,'c3','MainEquipment constructor zero stack args'),
        (0x02B6192B,'c3','MapHerb constructor zero stack args'),
    ): exact(va,bytes.fromhex(h),label)
    branch(0x02B4C61B,0x01BF2BC6,label='separate native registration call')
    exact(0x04EC4BAC,b'sUnit::addBottom\0','native registration literal')
    return dict(status='PASS_STATIC_EVIDENCE_ONLY',input_sha256=SHA,input_size=len(data),
        checks=len(records),records=records,gameplay_executed=False,game_image_written=False,
        limits=['Exact instruction witnesses, not full transitive side-effect proof',
                'Does not establish a live driver safe point or callback ownership'])

def main(argv=None):
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path)
    p.add_argument('--report',required=True,type=Path);a=p.parse_args(argv)
    try:
        if a.report.resolve()==a.original.resolve() or os.path.lexists(a.report):
            raise ValueError('report destination exists or aliases original')
        report=audit(a.original.read_bytes())
        with a.report.open('x',encoding='utf-8') as f:json.dump(report,f,indent=2);f.write('\n')
        print(json.dumps({k:v for k,v in report.items() if k!='records'}));return 0
    except (OSError,ValueError,struct.error) as e:
        print('ERROR: '+str(e),file=sys.stderr);return 2
if __name__=='__main__':raise SystemExit(main())
