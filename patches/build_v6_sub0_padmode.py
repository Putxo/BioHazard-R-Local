#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, struct

SRC=Path('/mnt/data/BioRevHD 30-Enero-2013.exe')
OUT_INPUT=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP P2 SUB0 PADMODE v6.exe')
OUT_COMBINED=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v6 SUB0 PADMODE NATIVE SPLIT.exe')
MAN=Path('/mnt/data/rev1_local_coop_research/p2-sub0-padmode-v6-manifest.json')
CAM_BIN=Path('/mnt/data/rev1_local_coop_research/camera_split_v4.bin')

TEXT_VA=0x01B79000
TEXT_RAW=0x400
DATA_VSIZE_FILE_OFF=0x298
DATA_VSIZE_OLD=0x003B4184
DATA_VSIZE_NEW=0x003B4188
G_SUB0_NPC=0x057D9184
SUB0_VTABLE=0x04E1649C
SET_THINKMODE_FULL=0x01BB8B60  # thunk -> 0x0278CC40

CAM_HOOK_VA=0x0203E3A9
CAM_CAVE_VA=0x0203E3BD
CAM_NEXT_VA=0x0203E470

def off(va): return va-TEXT_VA+TEXT_RAW

def rel32(src_va, instr_len, dst_va):
    return struct.pack('<i', dst_va-(src_va+instr_len))

def sha256(p): return hashlib.sha256(p.read_bytes()).hexdigest()

def ranges(a,b):
    out=[]; s=None; n=min(len(a),len(b))
    for i in range(n):
        if a[i]!=b[i] and s is None: s=i
        elif a[i]==b[i] and s is not None: out.append((s,i)); s=None
    if s is not None: out.append((s,n))
    if len(a)!=len(b): out.append((n,max(len(a),len(b))))
    return out

base=SRC.read_bytes(); data=bytearray(base); patches=[]

def patch_va(va, expected, replacement, label):
    assert len(expected)==len(replacement),(label,len(expected),len(replacement))
    o=off(va); got=bytes(data[o:o+len(expected)])
    if got!=expected:
        raise RuntimeError(f'{label} @ {va:#x}: expected {expected.hex()} got {got.hex()}')
    data[o:o+len(replacement)]=replacement
    patches.append({'label':label,'va':hex(va),'file_offset':hex(o),'old':expected.hex(),'new':replacement.hex(),'bytes':len(expected)})

def patch_file(o, expected, replacement, label):
    assert len(expected)==len(replacement)
    got=bytes(data[o:o+len(expected)])
    if got!=expected:
        raise RuntimeError(f'{label} @ file {o:#x}: expected {expected.hex()} got {got.hex()}')
    data[o:o+len(replacement)]=replacement
    patches.append({'label':label,'file_offset':hex(o),'old':expected.hex(),'new':replacement.hex(),'bytes':len(expected)})

# Writable tracker for exact live SubPlayer0 actor.
patch_file(DATA_VSIZE_FILE_OFF, struct.pack('<I',DATA_VSIZE_OLD), struct.pack('<I',DATA_VSIZE_NEW), '.data VirtualSize +4 for gLocalCoopSub0Npc')

# Selector function: exact tracked Sub0 -> PadData[1], everyone else -> PadData[0].
selector_cave=0x027A1340
patch_va(0x027A27D3, bytes.fromhex('33c05f5e5b'), b'\xe9'+rel32(0x027A27D3,5,selector_cave), 'selector body -> Sub0 identity cave')
code=b''
code += bytes.fromhex('8b4df8')
code += bytes.fromhex('33c0')
code += bytes.fromhex('3b0d')+struct.pack('<I',G_SUB0_NPC)
code += bytes.fromhex('0f94c0')
code += bytes.fromhex('0fb6c0')
code += bytes.fromhex('5f5e5b8be55dc3')
patch_va(selector_cave,b'\xcc'*len(code),code,'selector: exact Sub0=>1 else0')

# Track SubPlayer0 actor. Clear tracker when common binder clears its actor.
clear_cave=0x027A1360
patch_va(0x02DF4FA7, bytes.fromhex('c7404400000000'), b'\xe9'+rel32(0x02DF4FA7,5,clear_cave)+b'\x90\x90', 'uPcsPlayer actor-clear -> Sub0 tracking cave')
code=b''
code += bytes.fromhex('c7404400000000')
code += bytes.fromhex('8138')+struct.pack('<I',SUB0_VTABLE)
code += b'\x0f\x85'+rel32(clear_cave+len(code),6,0x02DF4FAE)
code += bytes.fromhex('c705')+struct.pack('<I',G_SUB0_NPC)+bytes.fromhex('00000000')
code += b'\xe9'+rel32(clear_cave+len(code),5,0x02DF4FAE)
patch_va(clear_cave,b'\xcc'*len(code),code,'clear gSub0 when Sub0 binding is cleared')

# Save exact Sub0 uNpc*. If it is the offline CPU partner, use the canonical
# ThinkMode wrapper to convert ONLY Cpu(2) -> Pad(1). Leave Pad and Network intact.
bind_cave=0x027A1390
old=bytes.fromhex('8bc8e81693dffe8b4df8894144')
patch_va(0x02DF5015,old,b'\xe9'+rel32(0x02DF5015,5,bind_cave)+b'\x90'*(len(old)-5),'uPcsPlayer actor-bind -> Sub0 PadMode cave')
code=b''
code += bytes.fromhex('8bc8')
cs=bind_cave+len(code); code += b'\xe8'+rel32(cs,5,0x01BEE332)  # unwrap actual uNpc*
code += bytes.fromhex('8b4df8')
code += bytes.fromhex('894144')                           # original [uPcsPlayer+44]=uNpc*
code += bytes.fromhex('8139')+struct.pack('<I',SUB0_VTABLE)
jne_pos=bind_cave+len(code); code += b'\x0f\x85'+rel32(jne_pos,6,0x02DF5022)
code += bytes.fromhex('a3')+struct.pack('<I',G_SUB0_NPC)
code += bytes.fromhex('83b8400e000002')                 # cmp [uNpc+E40], ThinkMode::Cpu
jne2_pos=bind_cave+len(code); code += b'\x0f\x85'+rel32(jne2_pos,6,0x02DF5022)
code += bytes.fromhex('6a01')                               # push ThinkMode::Pad
code += bytes.fromhex('8bc8')                               # this=uNpc
call_pos=bind_cave+len(code); code += b'\xe8'+rel32(call_pos,5,SET_THINKMODE_FULL)
code += b'\xe9'+rel32(bind_cave+len(code),5,0x02DF5022)
patch_va(bind_cave,b'\xcc'*len(code),code,'capture Sub0 and canonical Cpu->Pad ThinkMode switch')

# Restore selector argument in PC sGamePad getters.
analog_loads=[0x02DB15B7,0x02DB15C8,0x02DB1787,0x02DB1798,0x02DB1957,0x02DB1968,0x02DB1CD7,0x02DB1CE8]
for va in analog_loads:
    patch_va(va,bytes.fromhex('8b9170090000'),bytes.fromhex('8b530c909090'),f'sGamePad analog selector arg @ {va:#x}')
for va,oldb,newb,label in [
    (0x02DAF404,bytes.fromhex('8b8270090000'),bytes.fromhex('8b4508909090'),'aim bool index -> arg'),
    (0x02DAF40E,bytes.fromhex('8b9170090000'),bytes.fromhex('8b5508909090'),'aim bool second index -> arg'),
    (0x02DAFFE5,bytes.fromhex('8b9170090000'),bytes.fromhex('8b5508909090'),'run bool index -> arg'),
    (0x02DAFFEF,bytes.fromhex('8b8870090000'),bytes.fromhex('8b4d08909090'),'run bool second index -> arg'),
]: patch_va(va,oldb,newb,label)

OUT_INPUT.write_bytes(data)

# Combine with independently verified camera split v4.
combined=bytearray(data); cave=CAM_BIN.read_bytes()
assert len(cave) <= CAM_NEXT_VA-CAM_CAVE_VA
ho=off(CAM_HOOK_VA); co=off(CAM_CAVE_VA)
assert combined[ho:ho+5] == bytes.fromhex('5f5e5b81c4')
assert combined[co:co+len(cave)] == b'\xcc'*len(cave)
combined[ho:ho+5]=b'\xe9'+rel32(CAM_HOOK_VA,5,CAM_CAVE_VA)
combined[co:co+len(cave)]=cave
OUT_COMBINED.write_bytes(combined)

rr_i=ranges(base,data); rr_c=ranges(base,combined)
manifest={
 'source':SRC.name,'source_sha256':sha256(SRC),
 'input_output':OUT_INPUT.name,'input_sha256':sha256(OUT_INPUT),
 'combined_output':OUT_COMBINED.name,'combined_sha256':sha256(OUT_COMBINED),
 'status':'STATICALLY VERIFIED ONLY - gameplay not executed in this environment',
 'design':{
   'identity':'Track exact uNpc* bound to uPcsPlayerSub0 by vtable 0x04E1649C.',
   'thinkmode':'Only tracked Sub0 in ThinkMode::Cpu(2) is changed through canonical virtual wrapper 0x01BB8B60 to ThinkMode::Pad(1). Existing Pad and Network remain untouched.',
   'pad':'Selector returns 1 only for tracked Sub0; restored PC sGamePad getters honor selector and reach PadData[1].',
   'camera':'Combined output applies native Self VIEW_0 TOP + Partner VIEW_1 BOTTOM camera split v4.'
 },
 'diff':{
   'input_same_size':len(data)==len(base),'input_different_bytes':sum(e-s for s,e in rr_i),'input_ranges':len(rr_i),
   'combined_same_size':len(combined)==len(base),'combined_different_bytes':sum(e-s for s,e in rr_c),'combined_ranges':len(rr_c)
 },
 'patches':patches,
 'limitations':[
   'Runtime validation required.',
   'This is more native than v5 because the partner is actually put into ThinkMode::Pad, so AI systems that key on Cpu should stop treating it as CPU; this remains to be confirmed in gameplay.',
   'Network(3) is deliberately preserved to avoid breaking online sessions.',
   'HUD, inventory, scripted actions, QTEs and partner camera lifecycle remain to test.'
 ]
}
MAN.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('input',OUT_INPUT,manifest['input_sha256'])
print('combined',OUT_COMBINED,manifest['combined_sha256'])
print('diff input',manifest['diff']['input_different_bytes'],manifest['diff']['input_ranges'])
print('diff combined',manifest['diff']['combined_different_bytes'],manifest['diff']['combined_ranges'])
print('patch records',len(patches))
print('manifest',MAN)
