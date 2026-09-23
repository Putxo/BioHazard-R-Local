#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, struct

SRC=Path('/mnt/data/BioRevHD 30-Enero-2013.exe')
OUT_INPUT=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP P2 SUB0 INPUT v5.exe')
OUT_COMBINED=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v5 SUB0 NATIVE SPLIT.exe')
MAN=Path('/mnt/data/rev1_local_coop_research/p2-sub0-input-v5-manifest.json')
CAM_BIN=Path('/mnt/data/rev1_local_coop_research/camera_split_v4.bin')

TEXT_VA=0x01B79000
TEXT_RAW=0x400
TEXT_DELTA=TEXT_VA-TEXT_RAW

# Extend .data VirtualSize by four bytes and use the new zero-filled DWORD as writable scratch.
DATA_VSIZE_FILE_OFF=0x298
DATA_VSIZE_OLD=0x003B4184
DATA_VSIZE_NEW=0x003B4188
G_SUB0_NPC=0x057D9184

# Existing native camera patch (v4)
CAM_HOOK_VA=0x0203E3A9
CAM_CAVE_VA=0x0203E3BD
CAM_NEXT_VA=0x0203E470


def off(va): return va-TEXT_VA+TEXT_RAW

def rel32(src_va, instr_len, dst_va):
    return struct.pack('<i', dst_va-(src_va+instr_len))

def sha256_bytes(x): return hashlib.sha256(x).hexdigest()
def sha256(p): return sha256_bytes(p.read_bytes())

def ranges(a,b):
    out=[]; s=None
    n=min(len(a),len(b))
    for i in range(n):
        if a[i]!=b[i] and s is None: s=i
        elif a[i]==b[i] and s is not None: out.append((s,i)); s=None
    if s is not None: out.append((s,n))
    if len(a)!=len(b): out.append((n,max(len(a),len(b))))
    return out

base=SRC.read_bytes()
data=bytearray(base)
patches=[]

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

# 0) Allocate a real writable zero-filled DWORD by extending .data VirtualSize by 4.
patch_file(DATA_VSIZE_FILE_OFF, struct.pack('<I',DATA_VSIZE_OLD), struct.pack('<I',DATA_VSIZE_NEW), '.data VirtualSize +4 for gLocalCoopSub0Npc')

# 1) Preserve original ThinkMode::Pad behavior. For non-Pad modes, only route SubPlayer0 CPU to local input.
branch_cave=0x027A1318
patch_va(0x027A0CE7, bytes.fromhex('0f8593030000'), b'\x0f\x85'+rel32(0x027A0CE7,6,branch_cave), 'non-Pad ThinkMode -> Sub0 discriminator')
code=b''
code += bytes.fromhex('83f802')                              # cmp eax,2 (ThinkMode::Cpu)
code += b'\x0f\x85'+rel32(branch_cave+len(code),6,0x027A1080) # jne original nonlocal
code += bytes.fromhex('8b4df8')                              # mov ecx,[ebp-8] (uNpc this)
code += bytes.fromhex('3b0d')+struct.pack('<I',G_SUB0_NPC)   # cmp ecx,[gSub0]
code += b'\x0f\x85'+rel32(branch_cave+len(code),6,0x027A1080) # jne original nonlocal
code += b'\xe9'+rel32(branch_cave+len(code),5,0x027A0CED)   # jmp local Pad branch
patch_va(branch_cave,b'\xcc'*len(code),code,'Sub0 CPU -> local Pad branch only')

# 2) Selector function: return PadData index 1 only for the exact live SubPlayer0 uNpc, else 0.
selector_cave=0x027A1340
patch_va(0x027A27D3, bytes.fromhex('33c05f5e5b'), b'\xe9'+rel32(0x027A27D3,5,selector_cave), 'selector body -> Sub0 identity cave')
code=b''
code += bytes.fromhex('8b4df8')                              # mov ecx,[ebp-8]
code += bytes.fromhex('33c0')                                # xor eax,eax
code += bytes.fromhex('3b0d')+struct.pack('<I',G_SUB0_NPC)   # cmp ecx,[gSub0]
code += bytes.fromhex('0f94c0')                              # sete al
code += bytes.fromhex('0fb6c0')                              # movzx eax,al
code += bytes.fromhex('5f5e5b8be55dc3')                    # original epilogue
patch_va(selector_cave,b'\xcc'*len(code),code,'selector: exact Sub0=>1 else0')

# 3) Track the exact live uNpc bound to uPcsPlayerSub0.
# Clear scratch when the common PCS binding scan clears +0x44, but only for uPcsPlayerSub0 vtable.
clear_cave=0x027A1360
patch_va(0x02DF4FA7, bytes.fromhex('c7404400000000'), b'\xe9'+rel32(0x02DF4FA7,5,clear_cave)+b'\x90\x90', 'uPcsPlayer actor-clear -> Sub0 tracking cave')
code=b''
code += bytes.fromhex('c7404400000000')                    # mov [eax+44],0 (original)
code += bytes.fromhex('81389c64e104')                      # cmp dword ptr [eax], uPcsPlayerSub0 vtable 0x04E1649C
code += b'\x0f\x85'+rel32(clear_cave+len(code),6,0x02DF4FAE)
code += bytes.fromhex('c705')+struct.pack('<I',G_SUB0_NPC)+bytes.fromhex('00000000') # g=0
code += b'\xe9'+rel32(clear_cave+len(code),5,0x02DF4FAE)
patch_va(clear_cave,b'\xcc'*len(code),code,'clear gSub0 when Sub0 binding is cleared')

# Save the actual uNpc* when the common binder finds the object for uPcsPlayerSub0.
bind_cave=0x027A1390
old=bytes.fromhex('8bc8e81693dffe8b4df8894144')
new=b'\xe9'+rel32(0x02DF5015,5,bind_cave)+b'\x90'*(len(old)-5)
patch_va(0x02DF5015,old,new,'uPcsPlayer actor-bind -> Sub0 tracking cave')
code=b''
code += bytes.fromhex('8bc8')                                # mov ecx,eax (iterator item)
cs=bind_cave+len(code); code += b'\xe8'+rel32(cs,5,0x01BEE332) # unwrap -> actual uNpc*
code += bytes.fromhex('8b4df8')                              # mov ecx,[ebp-8] uPcsPlayer*
code += bytes.fromhex('894144')                              # mov [ecx+44],eax (original)
code += bytes.fromhex('81399c64e104')                      # cmp [ecx],uPcsPlayerSub0 vtable
code += b'\x0f\x85'+rel32(bind_cave+len(code),6,0x02DF5022)
code += bytes.fromhex('a3')+struct.pack('<I',G_SUB0_NPC)     # mov [gSub0],eax
code += b'\xe9'+rel32(bind_cave+len(code),5,0x02DF5022)
patch_va(bind_cave,b'\xcc'*len(code),code,'capture exact SubPlayer0 uNpc pointer')

# 4) Restore the selector argument already present in PC sGamePad getters.
analog_loads=[
    0x02DB15B7,0x02DB15C8,
    0x02DB1787,0x02DB1798,
    0x02DB1957,0x02DB1968,
    0x02DB1CD7,0x02DB1CE8,
]
for va in analog_loads:
    patch_va(va, bytes.fromhex('8b9170090000'), bytes.fromhex('8b530c909090'), f'sGamePad analog selector arg @ {va:#x}')
for va,old,new,label in [
    (0x02DAF404, bytes.fromhex('8b8270090000'), bytes.fromhex('8b4508909090'), 'aim bool index -> arg'),
    (0x02DAF40E, bytes.fromhex('8b9170090000'), bytes.fromhex('8b5508909090'), 'aim bool second index -> arg'),
    (0x02DAFFE5, bytes.fromhex('8b9170090000'), bytes.fromhex('8b5508909090'), 'run bool index -> arg'),
    (0x02DAFFEF, bytes.fromhex('8b8870090000'), bytes.fromhex('8b4d08909090'), 'run bool second index -> arg'),
]: patch_va(va,old,new,label)

OUT_INPUT.write_bytes(data)

# 5) Add the independently verified native camera split v4 to the corrected Sub0 input build.
combined=bytearray(data)
cave=CAM_BIN.read_bytes()
assert len(cave) <= CAM_NEXT_VA-CAM_CAVE_VA
ho=off(CAM_HOOK_VA); co=off(CAM_CAVE_VA)
assert combined[ho:ho+5] == bytes.fromhex('5f5e5b81c4')
assert combined[co:co+len(cave)] == b'\xcc'*len(cave)
combined[ho:ho+5]=b'\xe9'+rel32(CAM_HOOK_VA,5,CAM_CAVE_VA)
combined[co:co+len(cave)]=cave
OUT_COMBINED.write_bytes(combined)

rr_input=ranges(base,data); rr_comb=ranges(base,combined)
manifest={
 'source':SRC.name,'source_sha256':sha256(SRC),
 'input_output':OUT_INPUT.name,'input_sha256':sha256(OUT_INPUT),
 'combined_output':OUT_COMBINED.name,'combined_sha256':sha256(OUT_COMBINED),
 'status':'STATICALLY VERIFIED ONLY - gameplay not executed in this environment',
 'design':{
   'identity':'uPcsPlayerSub0+0x44 is the live uNpc* matched by [uNpc+0xE3C] against the native Sub0 PCS ID.',
   'tracking_global':hex(G_SUB0_NPC),
   'tracking_storage':'new 4-byte zero-filled writable tail allocated by increasing .data VirtualSize from 0x3B4184 to 0x3B4188',
   'thinkmode':'ThinkMode::Pad(1) remains stock; ThinkMode::Cpu(2) is redirected to local input ONLY when uNpc this == tracked SubPlayer0; ThinkMode::Network(3) remains stock.',
   'pad':'tracked SubPlayer0 selector=1; every other NPC selector=0; PC sGamePad getters restored to honor the selector argument.',
   'camera':'combined output additionally applies native camera split v4: Self->VIEW_0 TOP and Partner->VIEW_1 BOTTOM, both display 0.'
 },
 'patches':patches,
 'diff':{
   'input_same_size':len(data)==len(base),'input_different_bytes':sum(e-s for s,e in rr_input),'input_ranges':len(rr_input),
   'combined_same_size':len(combined)==len(base),'combined_different_bytes':sum(e-s for s,e in rr_comb),'combined_ranges':len(rr_comb),
 },
 'limitations':[
   'Runtime validation is still required.',
   'The tracked Sub0 binding is established by uPcsPlayerSub0 vtable identity and the common PCS actor lookup; scenes that intentionally have no Sub0 will leave the pointer zero.',
   'This version deliberately does not hijack ThinkMode::Network, preserving the original online/network path.',
   'HUD, inventory, scripted actions, QTEs and camera lifecycle still require gameplay tests.'
 ]
}
MAN.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('input',OUT_INPUT,manifest['input_sha256'])
print('combined',OUT_COMBINED,manifest['combined_sha256'])
print('diff input',manifest['diff']['input_different_bytes'],manifest['diff']['input_ranges'])
print('diff combined',manifest['diff']['combined_different_bytes'],manifest['diff']['combined_ranges'])
print('patch records',len(patches),'patched bytes records',sum(x['bytes'] for x in patches))
print('manifest',MAN)
