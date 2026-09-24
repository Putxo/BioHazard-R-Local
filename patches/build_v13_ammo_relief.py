#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, struct, subprocess

BASE=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v12 SUB0 PICKUP PAD2.exe')
OUT=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v13 SYMMETRIC AMMO RELIEF.exe')
ASM=Path('/mnt/data/rev1_local_coop_research/ammo_relief_v13.S')
OBJ=Path('/mnt/data/rev1_local_coop_research/ammo_relief_v13.o')
ELF=Path('/mnt/data/rev1_local_coop_research/ammo_relief_v13.elf')
BIN=Path('/mnt/data/rev1_local_coop_research/ammo_relief_v13.bin')
MAN=Path('/mnt/data/rev1_local_coop_research/local-coop-v13-ammo-relief-manifest.json')

EXPECTED_BASE='5aafc3fd4273d608b4c9b8b60631256b56cb27d6aab0ecca8d2718d824dd28e8'
EXPECTED_OUT='3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa'

TEXT_VA=0x01B79000
TEXT_RAW=0x400
HELPER_BASE=0x01C95220
COOP_OR_LOCAL=0x01C95220
SELECT_OTHER=0x01C95234
VALID_OTHER=0x01C952A0
AMMO_MULT=0x01C952C5

def off(va): return va-TEXT_VA+TEXT_RAW
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def rel32(src,n,dst): return struct.pack('<i',dst-(src+n))

def ranges(a,b):
    out=[]; s=None
    for i,(x,y) in enumerate(zip(a,b)):
        if x!=y and s is None:s=i
        elif x==y and s is not None:out.append((s,i));s=None
    if s is not None:out.append((s,min(len(a),len(b))))
    if len(a)!=len(b):out.append((min(len(a),len(b)),max(len(a),len(b))))
    return out

if sha(BASE)!=EXPECTED_BASE:
    raise RuntimeError(f'unexpected v12 SHA: {sha(BASE)}')

subprocess.run(['as','--32','-o',str(OBJ),str(ASM)],check=True)
subprocess.run(['ld','-m','elf_i386','-Ttext',hex(HELPER_BASE),'-o',str(ELF),str(OBJ)],check=True)
subprocess.run(['objcopy','-O','binary','-j','.text',str(ELF),str(BIN)],check=True)
blob=BIN.read_bytes()
if len(blob)!=186:
    raise RuntimeError(f'unexpected helper size {len(blob)}')

base=BASE.read_bytes()
d=bytearray(base)
patches=[]

def patch(va,expected,new,label):
    if len(expected)!=len(new): raise ValueError(label)
    o=off(va)
    got=bytes(d[o:o+len(expected)])
    if got!=expected:
        raise RuntimeError(f'{label} @ {va:#x}: expected {expected.hex()} got {got.hex()}')
    d[o:o+len(new)]=new
    patches.append({'label':label,'va':hex(va),'file_offset':hex(o),'old':expected.hex(),'new':new.hex(),'bytes':len(new)})

patch(HELPER_BASE,b'\xCC'*len(blob),blob,'local symmetric ammo-relief helper block')

for va,stock,target,label in [
    (0x027AC016,0x01C076CF,COOP_OR_LOCAL,'ammo relief: GameMode::Coop OR local flag'),
    (0x027AC077,0x01C853D1,VALID_OTHER,'ammo relief: local P1 accepted as reciprocal other actor'),
    (0x027AC0E2,0x01B8D7F8,AMMO_MULT,'ammo relief: local uses configured Coop ammo multiplier'),
]:
    o=off(va)
    old=bytes(d[o:o+5])
    if old[0]!=0xE8:
        raise RuntimeError(f'expected CALL @ {va:#x}: {old.hex()}')
    dst=va+5+struct.unpack('<i',old[1:])[0]
    if dst!=stock:
        raise RuntimeError(f'unexpected call target @ {va:#x}: {dst:#x}')
    patch(va,old,b'\xE8'+rel32(va,5,target),label)

patch(
    0x027AC059,
    bytes.fromhex('8945ec837dec00'),
    b'\xE8'+rel32(0x027AC059,5,SELECT_OTHER)+b'\x90\x90',
    'ammo relief: symmetric P1/Sub0 other-actor selection'
)

OUT.write_bytes(d)
if len(d)!=len(base):
    raise RuntimeError('file size changed')
if sha(OUT)!=EXPECTED_OUT:
    raise RuntimeError(f'v13 SHA mismatch: {sha(OUT)}')

rr=ranges(base,d)
manifest={
  'base':BASE.name,
  'base_sha256':EXPECTED_BASE,
  'output':OUT.name,
  'output_sha256':sha(OUT),
  'status':'STATICALLY VERIFIED ONLY - gameplay not executed in this environment',
  'purpose':'Replicate only the stock Coop ammo-relief rule in local Campaign without globally changing GameMode.',
  'design':{
    'coop_gate':'0x027ABFE0 enters for stock Coop OR gLocalCoopActive',
    'symmetry':'P1 current actor -> exact Sub0; Sub0 current actor -> stock local P1 actor lookup',
    'validation':'stock partner validator preserved; under local co-op a pl other actor is additionally valid',
    'multiplier':'local path uses configured Coop ammo acquisition multiplier [0x05479970] = 1.5',
    'preservation':'gLocalCoopActive=0 follows stock online/Campaign behavior'
  },
  'patches':patches,
  'diff_vs_v12':{
    'same_file_size':len(d)==len(base),
    'different_bytes':sum(e-s for s,e in rr),
    'ranges':len(rr)
  },
  'lineage_check':'Restoring only the v13 helper/calls/store+cmp reproduces v12 SHA-256 exactly.',
  'limitations':[
    'Runtime validation required.',
    'This patch targets only the ammo-relief side rule for normalized item group 0x80040200; normal pickup delivery remains v11/v12 stock-path behavior.',
    'Pickup candidate arbitration still uses nearest P1/Sub0 before the complete stock eligibility checks.'
  ]
}
MAN.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps(manifest,indent=2))
