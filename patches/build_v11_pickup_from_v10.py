#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, struct, subprocess

BASE=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v10 CLEAN SPLIT COOP ITEMBOX.exe')
ASM=Path('/mnt/data/rev1_local_coop_research/pickup_v11.S')
OBJ=Path('/mnt/data/rev1_local_coop_research/pickup_v11.o')
ELF=Path('/mnt/data/rev1_local_coop_research/pickup_v11.elf')
BIN=Path('/mnt/data/rev1_local_coop_research/pickup_v11.bin')
OUT=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v11 PICKUP.exe')
MAN=Path('/mnt/data/rev1_local_coop_research/local-coop-v11-pickup-manifest.json')

EXPECTED_BASE='5060269e5115ef8df53aa0ad4e26c09b4b273d4cfeb6628a7a4f902c606bb4e6'
EXPECTED_OUT='ef0217bf823edfc81f157ea29045417b56011b7c8b446b7e572e341a80245a71'
TEXT_VA=0x01B79000
TEXT_RAW=0x400
HELPER=0x01C95120

def off(va): return va-TEXT_VA+TEXT_RAW
def sha_bytes(b): return hashlib.sha256(b).hexdigest()
def sha(p): return sha_bytes(p.read_bytes())
def rel32(src,n,dst): return struct.pack('<i',dst-(src+n))

def ranges(a,b):
    out=[]; s=None
    for i,(x,y) in enumerate(zip(a,b)):
        if x!=y and s is None: s=i
        elif x==y and s is not None:
            out.append((s,i)); s=None
    if s is not None: out.append((s,min(len(a),len(b))))
    if len(a)!=len(b): out.append((min(len(a),len(b)),max(len(a),len(b))))
    return out

if sha(BASE)!=EXPECTED_BASE:
    raise RuntimeError(f'unexpected v10 SHA: {sha(BASE)}')

subprocess.run(['as','--32','-o',str(OBJ),str(ASM)],check=True)
subprocess.run(['ld','-m','elf_i386','-Ttext',hex(HELPER),'-o',str(ELF),str(OBJ)],check=True)
subprocess.run(['objcopy','-O','binary','-j','.text',str(ELF),str(BIN)],check=True)

nm=subprocess.check_output(['nm','-n',str(ELF)],text=True).splitlines()
sym={}
for line in nm:
    parts=line.split()
    if len(parts)>=3 and parts[2] in ('pickup_find_candidate','pickup_accept_f48','pickup_accept_arg'):
        sym[parts[2]]=int(parts[0],16)

base=BASE.read_bytes()
d=bytearray(base)
blob=BIN.read_bytes()
patches=[]

co=off(HELPER)
if bytes(d[co:co+len(blob)]) != b'\xCC'*len(blob):
    raise RuntimeError('v11 cave is not empty')
d[co:co+len(blob)] = blob

def patch_call(va,expected_dst,new_dst,label):
    o=off(va)
    old=bytes(d[o:o+5])
    if old[0]!=0xE8:
        raise RuntimeError(f'{label}: expected CALL, got {old.hex()}')
    dst=va+5+struct.unpack('<i',old[1:])[0]
    if dst!=expected_dst:
        raise RuntimeError(f'{label}: target {dst:#x}, expected {expected_dst:#x}')
    new=b'\xE8'+rel32(va,5,new_dst)
    d[o:o+5]=new
    patches.append({'label':label,'va':hex(va),'old':old.hex(),'new':new.hex(),
                    'old_target':hex(dst),'new_target':hex(new_dst)})

patch_call(0x02462159,0x01C07341,sym['pickup_find_candidate'],
           'uItem candidate finder -> Main/Sub0 nearest wrapper')
patch_call(0x02460664,0x01C8C9A1,sym['pickup_accept_f48'],
           'uItem pickup guard -> pl OR exact local Sub0')
patch_call(0x02464726,0x01C8C9A1,sym['pickup_accept_arg'],
           'uItem helper guard -> pl OR exact local Sub0')

OUT.write_bytes(d)
if sha(OUT)!=EXPECTED_OUT:
    raise RuntimeError(f'v11 SHA mismatch: {sha(OUT)}')

# Strong lineage check: remove only v11 and get exact v10 bytes/SHA.
rev=bytearray(d)
rev[co:co+len(blob)] = b'\xCC'*len(blob)
for rec in patches:
    rev[off(int(rec['va'],16)):off(int(rec['va'],16))+5] = bytes.fromhex(rec['old'])
if sha_bytes(bytes(rev))!=EXPECTED_BASE:
    raise RuntimeError('v11 lineage check failed')

rr=ranges(base,d)
manifest={
  'base':BASE.name,
  'base_sha256':EXPECTED_BASE,
  'output':OUT.name,
  'output_sha256':sha(OUT),
  'status':'STATICALLY VERIFIED ONLY - gameplay not executed in this environment',
  'design':{
    'candidate':'stock Main plus exact gSub0Npc when local coop is active; nearest squared 3D distance wins; tie keeps Main',
    'processing_guard':'stock pl OR exact gSub0Npc while gLocalCoopActive==1',
    'helper_guard':'same exact Sub0 exception in 0x024646F0',
    'global_category_predicates':'unchanged',
    'arbitrary_npc':'never accepted'
  },
  'symbols':{k:hex(v) for k,v in sym.items()},
  'helper_size':len(blob),
  'patches':patches,
  'diff_vs_v10':{
    'different_bytes':sum(e-s for s,e in rr),
    'ranges':len(rr),
    'same_file_size':len(d)==len(base)
  },
  'lineage_check':'Reverting only the v11 helper and three redirected calls reproduces v10 SHA-256 exactly.',
  'limitations':[
    'Runtime validation required.',
    'uItem has one candidate pointer, so when both players are eligible the nearer actor owns the candidate; ties keep Main.',
    'Other interaction types, scripted events and QTEs are separate follow-up work.'
  ]
}
MAN.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps(manifest,indent=2))
