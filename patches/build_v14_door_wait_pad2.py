#!/usr/bin/env python3
from pathlib import Path
import hashlib, json

BASE = Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v13 SYMMETRIC AMMO RELIEF.exe')
OUT  = Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v14 DOOR WAIT PAD2.exe')
MAN  = Path('/mnt/data/local-coop-v14-door-wait-pad2.json')

EXPECTED_BASE = '3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa'
TEXT_VA=0x01B79000
TEXT_RAW=0x400

def off(va): return va-TEXT_VA+TEXT_RAW
def sha_bytes(b): return hashlib.sha256(b).hexdigest()
def sha(p): return sha_bytes(p.read_bytes())

def ranges(a,b):
    out=[]; s=None; n=min(len(a),len(b))
    for i in range(n):
        if a[i]!=b[i] and s is None: s=i
        elif a[i]==b[i] and s is not None: out.append((s,i)); s=None
    if s is not None: out.append((s,n))
    if len(a)!=len(b): out.append((n,max(len(a),len(b))))
    return out

if sha(BASE) != EXPECTED_BASE:
    raise RuntimeError(f'unexpected v13 SHA: {sha(BASE)}')

base=BASE.read_bytes(); d=bytearray(base); patches=[]

def patch(va,expected,new,label):
    if len(expected)!=len(new): raise ValueError(label)
    o=off(va); got=bytes(d[o:o+len(expected)])
    if got!=expected:
        raise RuntimeError(f'{label} @ {va:#x}: expected {expected.hex()} got {got.hex()}')
    d[o:o+len(new)]=new
    patches.append({'label':label,'va':hex(va),'file_offset':hex(o),'old':expected.hex(),'new':new.hex(),'bytes':len(new)})

patch(0x02DB0DF5, bytes.fromhex('8b9170090000'), bytes.fromhex('8b5508909090'),
      'Door2p WaitState sGamePad getter: first PadData index uses selector arg')
patch(0x02DB0DFF, bytes.fromhex('8b8870090000'), bytes.fromhex('8b4d08909090'),
      'Door2p WaitState sGamePad getter: second PadData index uses selector arg')

OUT.write_bytes(d)
if len(d)!=len(base): raise RuntimeError('file size changed')

rev=bytearray(d)
for rec in patches:
    va=int(rec['va'],16); o=off(va)
    rev[o:o+rec['bytes']]=bytes.fromhex(rec['old'])
if sha_bytes(bytes(rev)) != EXPECTED_BASE:
    raise RuntimeError('lineage check failed: reverting v14 does not reproduce v13')

rr=ranges(base,d)
manifest={
  'base':BASE.name,
  'base_sha256':EXPECTED_BASE,
  'output':OUT.name,
  'output_sha256':sha(OUT),
  'status':'STATICALLY VERIFIED ONLY - gameplay not executed in this environment',
  'purpose':'Restore the existing player/pad selector in the one Door2p WaitState sGamePad boolean getter omitted by v7.',
  'evidence':{
    'wait_state':'cDoor2pBaseWaitState_PL vtable 0x04D5B220; input path contains calls 0x025746A7 and 0x025747E2',
    'selector':'0x02574661 calls 0x01C6C746 -> 0x027A27B0; canonical local-coop selector returns 1 only for exact tracked Sub0, otherwise 0',
    'already_fixed_sibling':'0x025746A7 -> 0x02DB0CF0; v7 replaced mStartPadNo at 0x02DB0D25/0x02DB0D2F with [ebp+8]',
    'missed_sibling':'0x025747E2 -> 0x02DB0DC0; v13 still loaded mStartPadNo at 0x02DB0DF5/0x02DB0DFF despite receiving the same selector argument',
    'caller_scope':'0x01C0DC6E -> 0x02DB0DC0 has one direct callsite in .text: 0x025747E2',
    'door_offline_routing':'Closed/ready logic already handles Self and pPt slots 0/1 natively; no global local-index patch is needed.'
  },
  'preservation':{
    'p1':'selector 0 keeps PadData[0]',
    'sub0_local':'selector 1 now reaches PadData[1] in both Door2p WaitState button-query variants',
    'network':'no ThinkMode, network predicate, local-member global, GameMode, door flags, or packet logic is modified',
    'scope':'only two 6-byte mStartPadNo loads are replaced'
  },
  'patches':patches,
  'diff_vs_v13':{
    'same_file_size':len(d)==len(base),
    'different_bytes':sum(e-s for s,e in rr),
    'ranges':len(rr),
    'ranges_raw':[[hex(s),hex(e)] for s,e in rr]
  },
  'lineage_check':'Restoring the two v14 sites reproduces v13 SHA-256 exactly.',
  'limitations':[
    'Runtime validation is required.',
    'This fixes the proven WaitState_PL pad-routing omission only; later door/QTE/cutscene systems remain to audit.',
    'No semantic name is assigned to 0x02DB0CF0/0x02DB0DC0 beyond adjacent sGamePad boolean-query getters without stronger symbol evidence.'
  ]
}
MAN.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps(manifest,indent=2))
