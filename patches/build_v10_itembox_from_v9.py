#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, struct

BASE=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v9 FULLPAD CLEAN PERSISTENT SPLIT.exe')
OUT=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v10 CLEAN SPLIT COOP ITEMBOX.exe')
MAN=Path('/mnt/data/rev1_local_coop_research/local-coop-v10-itembox-manifest.json')
EXPECTED_BASE='5dc7a7a413ce5916c2758be43b3107d5adf00beee93533b4acf5d83cec97c4be'
EXPECTED_OUT='5060269e5115ef8df53aa0ad4e26c09b4b273d4cfeb6628a7a4f902c606bb4e6'
TEXT_VA=0x01B79000
TEXT_RAW=0x400
G_LOCAL_ACTIVE=0x057D9188
STOCK_IS_COOP=0x01C076CF
WRAPPER=0x01C95100
CALLSITE=0x01D067AF

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
    raise RuntimeError(f'unexpected v9 SHA {sha(BASE)}')

d=bytearray(BASE.read_bytes())
base=bytes(d)

# ECX remains the stock cSystemData<Game>* from the original ItemBox selector call site.
code=b''
code+=b'\xE8'+rel32(WRAPPER,5,STOCK_IS_COOP)
code+=bytes.fromhex('84c0')       # test al,al
code+=bytes.fromhex('750a')       # if stock Coop => true
code+=bytes.fromhex('833d')+struct.pack('<I',G_LOCAL_ACTIVE)+bytes.fromhex('00')
code+=bytes.fromhex('0f95c0')     # setne al
code+=bytes.fromhex('c3')
assert len(code)==20

wo=off(WRAPPER)
if bytes(d[wo:wo+20])!=b'\xCC'*20:
    raise RuntimeError(f'wrapper cave not empty: {bytes(d[wo:wo+20]).hex()}')
d[wo:wo+20]=code

co=off(CALLSITE)
old=bytes(d[co:co+5])
if old[0]!=0xE8:
    raise RuntimeError(f'expected CALL at ItemBox selector: {old.hex()}')
dst=CALLSITE+5+struct.unpack('<i',old[1:])[0]
if dst!=STOCK_IS_COOP:
    raise RuntimeError(f'unexpected stock target {dst:#x}')
d[co:co+5]=b'\xE8'+rel32(CALLSITE,5,WRAPPER)

OUT.write_bytes(d)
if sha(OUT)!=EXPECTED_OUT:
    raise RuntimeError(f'output SHA mismatch {sha(OUT)}')

rr=ranges(base,d)
manifest={
  'base':BASE.name,'base_sha256':EXPECTED_BASE,
  'output':OUT.name,'output_sha256':sha(OUT),
  'status':'STATICALLY VERIFIED ONLY - gameplay not executed in this environment',
  'change':'ItemBox normal selector returns stock GameMode::Coop OR gLocalCoopActive; mGameMode itself remains unchanged.',
  'wrapper_va':hex(WRAPPER),'callsite_va':hex(CALLSITE),'local_flag':hex(G_LOCAL_ACTIVE),
  'stock_is_coop_thunk':hex(STOCK_IS_COOP),
  'diff_vs_v9':{'different_bytes':sum(e-s for s,e in rr),'ranges':len(rr),'same_file_size':len(d)==len(base)},
  'expected_behavior':{
    'campaign_local_off':'stock slot 0',
    'stock_coop':'stock slot 1',
    'campaign_local_on':'slot 1 sItemBoxCoop',
  },
  'limitations':['Runtime validation required.','Menu/pause input ownership is not yet proven independent for Pad 2.']
}
MAN.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps(manifest,indent=2))
