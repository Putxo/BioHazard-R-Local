#!/usr/bin/env python3
from pathlib import Path
import hashlib, struct, json, subprocess

BASE=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v10 CLEAN SPLIT COOP ITEMBOX.exe')
ASM=Path('/mnt/data/rev1_local_coop_research/pickup_sub0_v11.S')
OBJ=Path('/mnt/data/rev1_local_coop_research/pickup_sub0_v11.o')
ELF=Path('/mnt/data/rev1_local_coop_research/pickup_sub0_v11.elf')
BIN=Path('/mnt/data/rev1_local_coop_research/pickup_sub0_v11.bin')
OUT=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v11 SUB0 PICKUP.exe')
MAN=Path('/mnt/data/rev1_local_coop_research/local-coop-v11-pickup-manifest.json')

EXPECTED_BASE='5060269e5115ef8df53aa0ad4e26c09b4b273d4cfeb6628a7a4f902c606bb4e6'
EXPECTED_OUT='b45de9e4c1aff6aea44251efb863d0a90614856e6e3e4f9d234c5b6883ea6976'

TEXT_VA=0x01B79000
TEXT_RAW=0x400
WRAPPER=0x01C95120
STOCK_IS_PL=0x01C8C9A1
CALLS=[0x02460664,0x02464726]

def off(v): return v-TEXT_VA+TEXT_RAW
def sha_bytes(b): return hashlib.sha256(b).hexdigest()
def sha(p): return sha_bytes(p.read_bytes())
def rel32(s,n,d): return struct.pack('<i',d-(s+n))

def ranges(a,b):
    out=[]; s=None
    for i,(x,y) in enumerate(zip(a,b)):
        if x!=y and s is None:s=i
        elif x==y and s is not None:out.append((s,i));s=None
    if s is not None:out.append((s,min(len(a),len(b))))
    if len(a)!=len(b):out.append((min(len(a),len(b)),max(len(a),len(b))))
    return out

if sha(BASE)!=EXPECTED_BASE:
    raise RuntimeError(f'unexpected v10 SHA: {sha(BASE)}')

subprocess.run(['as','--32','-o',str(OBJ),str(ASM)],check=True)
subprocess.run(['ld','-m','elf_i386','-Ttext',hex(WRAPPER),'-o',str(ELF),str(OBJ)],check=True)
subprocess.run(['objcopy','-O','binary','-j','.text',str(ELF),str(BIN)],check=True)

base=BASE.read_bytes()
d=bytearray(base)
blob=BIN.read_bytes()

wo=off(WRAPPER)
if bytes(d[wo:wo+len(blob)]) != b'\xCC'*len(blob):
    raise RuntimeError('v11 helper cave is not empty')
d[wo:wo+len(blob)] = blob

patches=[]
for va in CALLS:
    o=off(va)
    old=bytes(d[o:o+5])
    if old[0]!=0xE8:
        raise RuntimeError(f'expected CALL @ {va:#x}: {old.hex()}')
    dst=va+5+struct.unpack('<i',old[1:])[0]
    if dst!=STOCK_IS_PL:
        raise RuntimeError(f'unexpected isPl target @ {va:#x}: {dst:#x}')
    new=b'\xE8'+rel32(va,5,WRAPPER)
    d[o:o+5]=new
    patches.append({'va':hex(va),'old':old.hex(),'new':new.hex(),'stock_target':hex(dst)})

OUT.write_bytes(d)
if sha(OUT)!=EXPECTED_OUT:
    raise RuntimeError(f'unexpected v11 SHA: {sha(OUT)}')

rr=ranges(base,d)

# Strong lineage check: remove only v11 changes and recover v10 bit-for-bit.
rev=bytearray(d)
rev[wo:wo+len(blob)] = b'\xCC'*len(blob)
for rec,va in zip(patches,CALLS):
    rev[off(va):off(va)+5]=bytes.fromhex(rec['old'])
if sha_bytes(bytes(rev)) != EXPECTED_BASE:
    raise RuntimeError('v11 lineage check failed')

manifest={
  'base':BASE.name,
  'base_sha256':EXPECTED_BASE,
  'output':OUT.name,
  'output_sha256':sha(OUT),
  'status':'STATICALLY VERIFIED ONLY - gameplay not executed in this environment',
  'change':'Only the two local-pickup isPl gates are widened to stock isPl OR exact tracked Sub0 while gLocalCoopActive=1.',
  'wrapper_va':hex(WRAPPER),
  'wrapper_bytes':len(blob),
  'callsites':[hex(x) for x in CALLS],
  'stock_is_pl_thunk':hex(STOCK_IS_PL),
  'diff_vs_v10':{
    'different_bytes':sum(e-s for s,e in rr),
    'ranges':len(rr),
    'same_file_size':len(d)==len(base)
  },
  'lineage_check':'Reverting only the v11 helper and two redirected calls reproduces v10 SHA-256 exactly.',
  'behavior':{
    'P1_pl':'stock true, unchanged',
    'local_Sub0_np':'true only when gLocalCoopActive=1 and ID matches tracked gSub0Npc',
    'other_NPC':'false, unchanged',
    'online_local_off':'stock behavior'
  },
  'limitations':[
    'Runtime validation required.',
    'Other interaction/QTE gates may have their own pl-only assumptions and are investigated separately.'
  ],
  'patches':patches
}
MAN.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps(manifest,indent=2))
