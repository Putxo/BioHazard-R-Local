#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, struct, subprocess

SRC = Path('/mnt/data/BioRevHD 30-Enero-2013.exe')
BASE = Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP P2 FULLPAD v7.exe')
ASM = Path('/mnt/data/rev1_local_coop_research/camera_persistence_v8.S')
OBJ = Path('/mnt/data/rev1_local_coop_research/camera_persistence_v8.o')
ELF = Path('/mnt/data/rev1_local_coop_research/camera_persistence_v8.elf')
BIN = Path('/mnt/data/rev1_local_coop_research/camera_persistence_v8.bin')
OUT = Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v9 FULLPAD CLEAN PERSISTENT SPLIT.exe')
MAN = Path('/mnt/data/rev1_local_coop_research/p2-fullpad-clean-persistent-split-v9-manifest.json')

EXPECTED_BASE_SHA='25cb559a183156bbb8de312688da86bec300bd78e66f0d6c6f7d776a3cc88af7'
TEXT_VA=0x01B79000
TEXT_RAW=0x400
DATA_VSIZE_OFF=0x298
DATA_VSIZE_V7=0x003B4188
DATA_VSIZE_V8=0x003B418C
G_SUB0=0x057D9184
G_LOCAL_ACTIVE=0x057D9188
SUB0_VTABLE=0x04E1649C

HELPER_BASE=0x01C94FC0
HELPER_LIMIT=0x01C95400
PAD_REBIND=0x01C94FD4
LOCAL_ENABLE=0x01C94FC0
COND_DEACT=0x01C94FF8
PARTNER_INDEX=0x01C9500C
COND_BIT0=0x01C95020
COND_BIT1=0x01C95036

DEACTIVATE=0x01C684D4
STOCK_VIEW_INDEX=0x01C083B3
STATE_BIT0=0x01C5EFA1
STATE_BIT1=0x01B8D636

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def off(va): return va-TEXT_VA+TEXT_RAW
def rel32(src, instr_len, dst): return struct.pack('<i', dst-(src+instr_len))

def diff_ranges(a,b):
    out=[]; s=None; n=min(len(a),len(b))
    for i in range(n):
        if a[i]!=b[i] and s is None:s=i
        elif a[i]==b[i] and s is not None:out.append((s,i));s=None
    if s is not None:out.append((s,n))
    if len(a)!=len(b):out.append((n,max(len(a),len(b))))
    return out

if sha(BASE)!=EXPECTED_BASE_SHA:
    raise RuntimeError(f'Unexpected v7 base SHA: {sha(BASE)}')

subprocess.run(['as','--32','-o',str(OBJ),str(ASM)],check=True)
subprocess.run(['ld','-m','elf_i386','-Ttext',hex(HELPER_BASE),'-o',str(ELF),str(OBJ)],check=True)
subprocess.run(['objcopy','-O','binary','-j','.text',str(ELF),str(BIN)],check=True)
blob=BIN.read_bytes()
if HELPER_BASE+len(blob)>HELPER_LIMIT:
    raise RuntimeError(f'helper too large: {len(blob)}')

base=SRC.read_bytes()
d=bytearray(BASE.read_bytes())
patches=[]

def patch_file(o, expected, new, label):
    if len(expected)!=len(new):raise ValueError(label)
    got=bytes(d[o:o+len(expected)])
    if got!=expected:raise RuntimeError(f'{label} file {o:#x}: expected {expected.hex()} got {got.hex()}')
    d[o:o+len(new)]=new
    patches.append({'label':label,'file_offset':hex(o),'old':expected.hex(),'new':new.hex(),'bytes':len(new)})

def patch_va(va, expected, new, label):
    patch_file(off(va),expected,new,label)
    patches[-1]['va']=hex(va)

patch_file(DATA_VSIZE_OFF,struct.pack('<I',DATA_VSIZE_V7),struct.pack('<I',DATA_VSIZE_V8),'.data VirtualSize +4 for gLocalCoopActive')

clear_va=0x027A1360
clear_code=b''
clear_code += bytes.fromhex('c7404400000000')
clear_code += bytes.fromhex('8138')+struct.pack('<I',SUB0_VTABLE)
clear_code += b'\x0f\x85'+rel32(clear_va+len(clear_code),6,0x02DF4FAE)
clear_code += bytes.fromhex('c705')+struct.pack('<I',G_SUB0)+bytes.fromhex('00000000')
clear_code += bytes.fromhex('c705')+struct.pack('<I',G_LOCAL_ACTIVE)+bytes.fromhex('00000000')
clear_code += b'\xe9'+rel32(clear_va+len(clear_code),5,0x02DF4FAE)
old_clear=bytes(d[off(clear_va):off(clear_va)+len(clear_code)])
expected_prefix=bytes.fromhex('c740440000000081389c64e1040f853b3c6500c70584917d0500000000e92c3c6500')
if bytes(d[off(clear_va):off(clear_va)+len(expected_prefix)]) != expected_prefix:
    raise RuntimeError('v7 clear cave prefix mismatch')
if any(x!=0xCC for x in d[off(clear_va)+len(expected_prefix):off(clear_va)+len(clear_code)]):
    raise RuntimeError('v8 clear expansion area not CC')
patch_va(clear_va,old_clear,clear_code,'clear exact Sub0 tracking + local-active flag')

old=bytes(d[off(0x027A13B5):off(0x027A13B5)+6])
if old[:2]!=b'\x0f\x85': raise RuntimeError('expected JNE at 0x27A13B5')
patch_va(0x027A13B5,old,b'\x0f\x85'+rel32(0x027A13B5,6,PAD_REBIND),'Sub0 non-Cpu -> local Pad rebind discriminator')
old=bytes(d[off(0x027A13C4):off(0x027A13C4)+5])
if old[0]!=0xE9: raise RuntimeError('expected JMP at 0x27A13C4')
patch_va(0x027A13C4,old,b'\xe9'+rel32(0x027A13C4,5,LOCAL_ENABLE),'after canonical Cpu->Pad -> enable persistent local split')

patch_va(HELPER_BASE,b'\xCC'*len(blob),blob,'persistent split helper block')

for va,label in [(0x0203E92D,'Self View: conditional Partner deactivate'),(0x0203E98B,'Partner View: conditional Self deactivate')]:
    old=bytes(d[off(va):off(va)+5])
    if old[0]!=0xE8:raise RuntimeError(f'expected call @ {va:#x}')
    dst=va+5+struct.unpack('<i',old[1:])[0]
    if dst!=DEACTIVATE:raise RuntimeError(f'unexpected call target @ {va:#x}: {dst:#x}')
    patch_va(va,old,b'\xe8'+rel32(va,5,COND_DEACT),label)

va=0x0203E9AD
old=bytes(d[off(va):off(va)+5]); dst=va+5+struct.unpack('<i',old[1:])[0]
if old[0]!=0xE8 or dst!=STOCK_VIEW_INDEX:raise RuntimeError(f'partner view index call mismatch {old.hex()}->{dst:#x}')
patch_va(va,old,b'\xe8'+rel32(va,5,PARTNER_INDEX),'Partner View viewport index -> conditional VIEW_1')

for va,target,wrapper,label in [
    (0x01F38054,STATE_BIT0,COND_BIT0,'Partner camera setup bit0: conditional keep-active'),
    (0x01F38061,STATE_BIT1,COND_BIT1,'Partner camera setup bit1: conditional keep-active'),
]:
    old=bytes(d[off(va):off(va)+5]); dst=va+5+struct.unpack('<i',old[1:])[0]
    if old[0]!=0xE8 or dst!=target:raise RuntimeError(f'state call mismatch @ {va:#x}: {old.hex()}->{dst:#x}')
    patch_va(va,old,b'\xe8'+rel32(va,5,wrapper),label)

orig=base
cam_init_o=off(0x0203E3A9)
cam_init_n=off(0x0203E464)
if bytes(d[cam_init_o:cam_init_n]) != bytes(orig[cam_init_o:cam_init_n]):
    raise RuntimeError('clean v9 unexpectedly modified stock camera init/v4 cave')

OUT.write_bytes(d)

if len(d)!=len(base): raise RuntimeError('file size changed')
if sha(OUT)==sha(BASE): raise RuntimeError('output unchanged')
assert bytes(d[off(HELPER_BASE):off(HELPER_BASE)+len(blob)])==blob
assert int.from_bytes(d[DATA_VSIZE_OFF:DATA_VSIZE_OFF+4],'little')==DATA_VSIZE_V8

rr=diff_ranges(base,d)
rr_v7=diff_ranges(BASE.read_bytes(),d)
manifest={
  'source':SRC.name,
  'source_sha256':sha(SRC),
  'base_v7_input_only':BASE.name,
  'base_v7_sha256':EXPECTED_BASE_SHA,
  'output':OUT.name,
  'output_sha256':sha(OUT),
  'status':'STATICALLY VERIFIED ONLY - gameplay not executed in this environment',
  'design':{
    'local_activation_flag':f'{G_LOCAL_ACTIVE:#010x}; set only after exact Sub0 canonical ThinkMode::Cpu(2)->Pad(1), cleared when Sub0 binding clears',
    'network_preservation':'ThinkMode::Network(3) never sets the local flag; because v9 is built on input-only v7, stock camera init/exclusivity/viewport behavior is byte-identical to the original whenever flag=0',
    'partner_target':'ensure_split writes the exact tracked Sub0 uNpc* into Partner uCameraManage via stock target setter 0x01C275EC->0x02066AB0',
    'camera_coexistence':'when local flag=1, Self View / Partner View no longer deactivate the other manager; both are activated with stock 0x01BF353F',
    'viewports':'Self->VIEW_0 TOP(2), Partner->VIEW_1 BOTTOM(3), both visible on display 0 via native setCameraForViewport',
    'persistence':'Sub0 bind/rebind, Self/Partner view callbacks, and the later Partner camera state-clear sites all preserve/reassert local split'
  },
  'diff':{
    'same_file_size':True,
    'different_bytes_vs_original':sum(e-s for s,e in rr),'ranges_vs_original':len(rr),
    'additional_different_bytes_vs_v7':sum(e-s for s,e in rr_v7),'ranges_vs_v7':len(rr_v7)
  },
  'patches':patches
}
MAN.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('output',OUT)
print('sha256',manifest['output_sha256'])
print('helper bytes',len(blob))
print('diff vs original',manifest['diff']['different_bytes_vs_original'],manifest['diff']['ranges_vs_original'])
print('additional vs v7',manifest['diff']['additional_different_bytes_vs_v7'],manifest['diff']['ranges_vs_v7'])
