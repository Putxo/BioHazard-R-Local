#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, struct

BASE=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v10 CLEAN SPLIT COOP ITEMBOX.exe')
OUT=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v11 SUB0 PICKUP.exe')
MAN=Path('/mnt/data/rev1_local_coop_research/local-coop-v11-sub0-pickup-manifest.json')

EXPECTED_BASE='5060269e5115ef8df53aa0ad4e26c09b4b273d4cfeb6628a7a4f902c606bb4e6'
EXPECTED_OUT='66b8a6ac440a2b676ce687860073a500617ff86de71d588464eeedc38532b415'
TEXT_VA=0x01B79000
TEXT_RAW=0x400
G_SUB0=0x057D9184
G_LOCAL=0x057D9188
STOCK_FALSE_JZ=0x02464733
HELPER=0x01C95120
STOCK_EXIT=0x024649AE
STOCK_ACCEPT=0x02464739

def off(va): return va-TEXT_VA+TEXT_RAW
def rel32(src,n,dst): return struct.pack('<i',dst-(src+n))
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()

def ranges(a,b):
    out=[]; s=None
    for i,(x,y) in enumerate(zip(a,b)):
        if x!=y and s is None:s=i
        elif x==y and s is not None:out.append((s,i));s=None
    if s is not None:out.append((s,min(len(a),len(b))))
    if len(a)!=len(b):out.append((min(len(a),len(b)),max(len(a),len(b))))
    return out

if sha(BASE)!=EXPECTED_BASE:
    raise RuntimeError(f'unexpected v10 SHA {sha(BASE)}')

base=BASE.read_bytes()
d=bytearray(base)

old=bytes(d[off(STOCK_FALSE_JZ):off(STOCK_FALSE_JZ)+6])
if old!=bytes.fromhex('0f8475020000'):
    raise RuntimeError(f'unexpected stock pl-false JZ: {old.hex()}')
d[off(STOCK_FALSE_JZ):off(STOCK_FALSE_JZ)+6] = b'\x0f\x84'+rel32(STOCK_FALSE_JZ,6,HELPER)

code=b''
code+=bytes.fromhex('833d')+struct.pack('<I',G_LOCAL)+bytes.fromhex('00')
code+=b'\x0f\x84'+rel32(HELPER+len(code),6,STOCK_EXIT)
code+=bytes.fromhex('a1')+struct.pack('<I',G_SUB0)
code+=bytes.fromhex('3b4508')
code+=b'\x0f\x85'+rel32(HELPER+len(code),6,STOCK_EXIT)
code+=b'\xe9'+rel32(HELPER+len(code),5,STOCK_ACCEPT)
assert len(code)==32

if bytes(d[off(HELPER):off(HELPER)+len(code)]) != b'\xCC'*len(code):
    raise RuntimeError('v11 helper cave is not empty')
d[off(HELPER):off(HELPER)+len(code)] = code

OUT.write_bytes(d)
if sha(OUT)!=EXPECTED_OUT:
    raise RuntimeError(f'output SHA mismatch {sha(OUT)}')

rr=ranges(base,d)

# Lineage proof.
rev=bytearray(d)
rev[off(STOCK_FALSE_JZ):off(STOCK_FALSE_JZ)+6]=old
rev[off(HELPER):off(HELPER)+len(code)]=b'\xCC'*len(code)
if hashlib.sha256(rev).hexdigest()!=EXPECTED_BASE:
    raise RuntimeError('reverting v11-only bytes did not reproduce v10')

manifest={
  'base':BASE.name,
  'base_sha256':EXPECTED_BASE,
  'output':OUT.name,
  'output_sha256':EXPECTED_OUT,
  'status':'STATICALLY VERIFIED ONLY - gameplay not executed in this environment',
  'change':'When stock uItem pickup rejects a non-pl actor, allow only exact tracked local Sub0 while gLocalCoopActive is non-zero.',
  'hook':{
    'stock_false_branch_va':hex(STOCK_FALSE_JZ),
    'helper_va':hex(HELPER),
    'stock_accept_va':hex(STOCK_ACCEPT),
    'stock_exit_va':hex(STOCK_EXIT)
  },
  'globals':{'gSub0Npc':hex(G_SUB0),'gLocalCoopActive':hex(G_LOCAL)},
  'diff_vs_v10':{
    'same_file_size':len(d)==len(base),
    'different_bytes':sum(e-s for s,e in rr),
    'ranges':len(rr)
  },
  'lineage_check':'Restoring only the v11 JZ and 32-byte helper cave reproduces v10 SHA-256 exactly.',
  'safety_scope':'General np/NPC pickup remains rejected. Only exact uPcsPlayerSub0 live actor is admitted, and only while local co-op flag is active.',
  'limitations':[
    'Runtime validation required.',
    'Need validate item-capacity/full-inventory edge cases and world-state sync for every item subtype in gameplay.'
  ]
}
MAN.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps(manifest,indent=2))
