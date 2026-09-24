#!/usr/bin/env python3
from pathlib import Path
import hashlib, json

BASE=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v11 SUB0 PICKUP.exe')
OUT=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v12 SUB0 PICKUP PAD2.exe')
MAN=Path('/mnt/data/rev1_local_coop_research/local-coop-v12-sub0-pickup-pad2-manifest.json')

EXPECTED_BASE='2c69c5f16626dc7478a6221c2bac98ed19f5b37dcf7991fc78f744f35f192d69'
EXPECTED_OUT='5aafc3fd4273d608b4c9b8b60631256b56cb27d6aab0ecca8d2718d824dd28e8'

TEXT_VA=0x01B79000
TEXT_RAW=0x400

def off(va): return va-TEXT_VA+TEXT_RAW
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()

if sha(BASE)!=EXPECTED_BASE:
    raise RuntimeError(f'unexpected v11 SHA: {sha(BASE)}')

d=bytearray(BASE.read_bytes())
patches=[]

def patch(va,old,new,label):
    if len(old)!=len(new):
        raise ValueError(label)
    o=off(va)
    got=bytes(d[o:o+len(old)])
    if got!=old:
        raise RuntimeError(f'{label} @ {va:#x}: expected {old.hex()} got {got.hex()}')
    d[o:o+len(new)]=new
    patches.append({'label':label,'va':hex(va),'file_offset':hex(o),'old':old.hex(),'new':new.hex(),'bytes':len(old)})

# uItem cActionCommand member selector.
# Stock returned 0. Under local co-op, return member/index 1 only when the
# current uItem pickup target (+0xF48) is the exact tracked Sub0 uNpc.
patch(
    0x026D7AD0,
    bytes.fromhex(
      '558bec81eccc000000535657518dbd34ffffffb933000000'
      'b8ccccccccf3ab59894df833c05f5e5b8be55dc20400'
    ),
    bytes.fromhex(
      '833d88917d050074168b81480f00003b0584917d057508'
      'b801000000c2040033c0c20400cccccccccccccccccccc'
    ),
    'uItem action-command selector: exact local Sub0 => member 1'
)

# PC sGamePad action-command getter already receives one selector argument.
# Restore that ABI instead of forcing sGamePad::mStartPadNo.
patch(
    0x02DB2B7D,
    bytes.fromhex('8b8870090000'),
    bytes.fromhex('8b4d08909090'),
    'cActionCommand sGamePad getter: honor selector argument'
)

OUT.write_bytes(d)
if len(d)!=len(BASE.read_bytes()):
    raise RuntimeError('file size changed')
if sha(OUT)!=EXPECTED_OUT:
    raise RuntimeError(f'v12 SHA mismatch: {sha(OUT)}')

manifest={
  'base':BASE.name,
  'base_sha256':EXPECTED_BASE,
  'output':OUT.name,
  'output_sha256':sha(OUT),
  'status':'STATICALLY VERIFIED ONLY - gameplay not executed in this environment',
  'design':{
    'uitem_member_selector':'stock 0x026D7AD0 returned 0; v12 returns 1 only when gLocalCoopActive && uItem+0xF48 == exact tracked Sub0',
    'sgamepad':'0x02DB2B50 receives one selector argument; v12 replaces its PC mStartPadNo load with [ebp+8]',
    'preservation':'inactive local co-op and P1-targeted uItems keep selector 0'
  },
  'patches':patches,
  'limitations':[
    'Runtime validation required.',
    'v11/v12 still arbitrate a single stock uItem pickup-target slot rather than maintaining two simultaneous prompts.',
    'Eligibility-aware arbitration when both players are near the same item remains to validate.'
  ]
}
MAN.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps(manifest,indent=2))
