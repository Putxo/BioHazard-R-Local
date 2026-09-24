#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, struct

BASE=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v11 SUB0 PICKUP.exe')
OUT=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v12 SUB0 PICKUP PAD2.exe')
MAN=Path('/mnt/data/rev1_local_coop_research/local-coop-v12-sub0-pickup-pad2-manifest.json')
EXPECTED_BASE='2c69c5f16626dc7478a6221c2bac98ed19f5b37dcf7991fc78f744f35f192d69'
EXPECTED_OUT='5aafc3fd4273d608b4c9b8b60631256b56cb27d6aab0ecca8d2718d824dd28e8'
TEXT_VA=0x01B79000
TEXT_RAW=0x400
G_SUB0=0x057D9184
G_ACTIVE=0x057D9188

def off(va): return va-TEXT_VA+TEXT_RAW
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()

def ranges(a,b):
    out=[]; s=None; n=min(len(a),len(b))
    for i in range(n):
        if a[i]!=b[i] and s is None:s=i
        elif a[i]==b[i] and s is not None:out.append((s,i));s=None
    if s is not None:out.append((s,n))
    if len(a)!=len(b):out.append((n,max(len(a),len(b))))
    return out

if sha(BASE)!=EXPECTED_BASE:
    raise RuntimeError(f'unexpected v11 SHA: {sha(BASE)}')

base=BASE.read_bytes()
d=bytearray(base)
patches=[]

def patch_va(va,expected,new,label):
    if len(expected)!=len(new): raise ValueError(label)
    o=off(va); got=bytes(d[o:o+len(expected)])
    if got!=expected:
        raise RuntimeError(f'{label} @ {va:#x}: expected {expected.hex()} got {got.hex()}')
    d[o:o+len(new)]=new
    patches.append({'label':label,'va':hex(va),'file_offset':hex(o),'old':expected.hex(),'new':new.hex(),'bytes':len(new)})

# uItem installs this callback as the cActionCommand owner/member selector.
# Stock implementation always returns 0.  The delegate is bound only once in
# the executable, in uItem's ActionCommand setup.
callback_va=0x026D7AD0
callback_len=0x2E
old=bytes(d[off(callback_va):off(callback_va)+callback_len])
if not old.startswith(bytes.fromhex('558bec81eccc00000053565751')):
    raise RuntimeError('unexpected stock uItem callback prologue')

code=bytearray()
code += b'\x83\x3d'+struct.pack('<I',G_ACTIVE)+b'\x00' # cmp [gActive],0
je_pos=len(code); code += b'\x74\x00'
code += b'\x8b\x81'+struct.pack('<I',0xF48)             # mov eax,[ecx+F48]
code += b'\x3b\x05'+struct.pack('<I',G_SUB0)            # cmp eax,[gSub0]
jne_pos=len(code); code += b'\x75\x00'
code += b'\xb8\x01\x00\x00\x00\xc2\x04\x00'     # return 1
zero_pos=len(code)
code += b'\x33\xc0\xc2\x04\x00'                     # return 0
code[je_pos+1]=(zero_pos-(je_pos+2)) & 0xff
code[jne_pos+1]=(zero_pos-(jne_pos+2)) & 0xff
new=bytes(code)+b'\xCC'*(callback_len-len(code))
patch_va(callback_va,old,new,'uItem ActionCommand member selector: exact local Sub0 => 1 else 0')

# cActionCommand passes its resolved member selector to this sGamePad helper.
# PC stock ignored the argument and read mStartPadNo. Restore the existing ABI.
patch_va(
    0x02DB2B7D,
    bytes.fromhex('8b8870090000'),
    bytes.fromhex('8b4d08909090'),
    'sGamePad 0x02DB2B50 uses ActionCommand member selector arg'
)

OUT.write_bytes(d)
if len(d)!=len(base): raise RuntimeError('file size changed')
if sha(OUT)!=EXPECTED_OUT: raise RuntimeError(f'output SHA mismatch: {sha(OUT)}')

rr=ranges(base,d)
manifest={
  'base':BASE.name,
  'base_sha256':EXPECTED_BASE,
  'output':OUT.name,
  'output_sha256':sha(OUT),
  'status':'STATICALLY VERIFIED ONLY - gameplay not executed in this environment',
  'design':{
    'uitem_member_selector':'stock uItem cActionCommand selector callback 0x026D7AD0 returned 0 unconditionally; v12 returns 1 only when gLocalCoopActive && uItem+0xF48 == exact tracked Sub0, otherwise 0',
    'unique_binding':'raw function pointer 0x01BA99B7 -> 0x026D7AD0 occurs once in the executable, in uItem cActionCommand setup',
    'sgamepad':'0x02DB2B50 already receives one selector argument (ret 4); v12 replaces the PC mStartPadNo load with [ebp+8]',
    'preservation':'when local co-op is inactive or uItem targets P1, cActionCommand member selection remains 0'
  },
  'diff_vs_v11':{
    'same_file_size':len(d)==len(base),
    'different_bytes':sum(e-s for s,e in rr),
    'ranges':len(rr)
  },
  'limitations':[
    'Runtime validation is required.',
    'v11/v12 still use the stock single uItem pickup-target slot and arbitrate P1/Sub0 rather than maintaining two simultaneous prompts.',
    'Eligibility-aware arbitration when both players are close to the same item still needs static/runtime validation.'
  ],
  'patches':patches
}
MAN.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps(manifest,indent=2))
