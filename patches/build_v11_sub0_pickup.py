#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, struct

BASE=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v10 CLEAN SPLIT COOP ITEMBOX.exe')
OUT=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v11 SUB0 PICKUP.exe')
BLOB=Path('/mnt/data/rev1_local_coop_research/pickup_sub0_v11.bin')
MAN=Path('/mnt/data/rev1_local_coop_research/local-coop-v11-sub0-pickup-manifest.json')
EXPECTED_BASE='5060269e5115ef8df53aa0ad4e26c09b4b273d4cfeb6628a7a4f902c606bb4e6'
TEXT_VA=0x01B79000
TEXT_RAW=0x400
HELPER_BASE=0x01C95120
CHOOSE=0x01C95120
GATE1=0x01C951D0
GATE2=0x01C951F7

def off(va): return va-TEXT_VA+TEXT_RAW
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def rel32(src,n,dst): return struct.pack('<i',dst-(src+n))

def ranges(a,b):
    out=[]; s=None; n=min(len(a),len(b))
    for i in range(n):
        if a[i]!=b[i] and s is None:s=i
        elif a[i]==b[i] and s is not None:out.append((s,i));s=None
    if s is not None:out.append((s,n))
    if len(a)!=len(b):out.append((n,max(len(a),len(b))))
    return out

if sha(BASE)!=EXPECTED_BASE:
    raise RuntimeError(f'unexpected v10 SHA: {sha(BASE)}')
base=BASE.read_bytes(); d=bytearray(base); patches=[]

def patch_va(va,expected,new,label):
    if len(expected)!=len(new):raise ValueError(label)
    o=off(va); got=bytes(d[o:o+len(expected)])
    if got!=expected:raise RuntimeError(f'{label} @ {va:#x}: expected {expected.hex()} got {got.hex()}')
    d[o:o+len(new)]=new
    patches.append({'label':label,'va':hex(va),'file_offset':hex(o),'old':expected.hex(),'new':new.hex(),'bytes':len(new)})

blob=BLOB.read_bytes()
if len(blob)!=248: raise RuntimeError(f'unexpected helper size {len(blob)}')
patch_va(HELPER_BASE,b'\xCC'*len(blob),blob,'Sub0 pickup helper block')
patch_va(0x0246215E,bytes.fromhex('8945d4837dd400'),b'\xE8'+rel32(0x0246215E,5,CHOOSE)+b'\x90\x90','choose nearest local pickup actor (P1/Sub0)')
patch_va(0x02460671,bytes.fromhex('0f84ed040000'),b'\x0f\x84'+rel32(0x02460671,6,GATE1),'pickup finalization: exact Sub0 fallback after is-pl false')
patch_va(0x02464733,bytes.fromhex('0f8475020000'),b'\x0f\x84'+rel32(0x02464733,6,GATE2),'item delivery: exact Sub0 fallback after is-pl false')

OUT.write_bytes(d)
if len(d)!=len(base): raise RuntimeError('file size changed')
rr=ranges(base,d)
manifest={
  'base':BASE.name,'base_sha256':EXPECTED_BASE,
  'output':OUT.name,'output_sha256':sha(OUT),
  'status':'STATICALLY VERIFIED ONLY - gameplay not executed in this environment',
  'design':{
    'candidate':'keep stock local-player lookup, then compare exact tracked Sub0 by squared 3D distance to the same uItem transform; store the closer actor in stock uItem+0xF48',
    'gate1':'stock pl unchanged; on is-pl false allow only gLocalCoopActive && uItem target == gSub0Npc',
    'gate2':'stock pl unchanged; on is-pl false allow only gLocalCoopActive && pickup actor == gSub0Npc',
    'network':'sNetworkManage global pl-only lookup remains untouched',
    'npc_scope':'no generic np actor is enabled'
  },
  'helper':{'base_va':hex(HELPER_BASE),'size':len(blob),'choose':hex(CHOOSE),'gate1':hex(GATE1),'gate2':hex(GATE2)},
  'patches':patches,
  'diff_vs_v10':{'same_file_size':len(d)==len(base),'different_bytes':sum(e-s for s,e in rr),'ranges':len(rr)},
  'limitations':[
    'Runtime validation is required.',
    'uItem has one local pickup target; v11 chooses the nearer of P1/Sub0 rather than duplicating per-player target state.',
    'If the nearer actor is temporarily invalid while the farther actor is valid, stock validation may reject that frame; runtime testing will show whether eligibility-aware arbitration is needed.',
    'Interaction prompt/HUD duplication is not addressed by this patch.'
  ]
}
MAN.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps(manifest,indent=2))
