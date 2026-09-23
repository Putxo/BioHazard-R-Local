from pathlib import Path
import hashlib, json, struct

SRC=Path('/mnt/data/BioRevHD 30-Enero-2013.exe')
OUT=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP P2 INPUT v3.exe')
MAN=Path('/mnt/data/rev1_local_coop_research/p2-input-v3-manifest.json')
TEXT_VA=0x01B79000
TEXT_RAW=0x400

def off(va):
    return va-TEXT_VA+TEXT_RAW

def rel32(src_va, instr_len, dst_va):
    return struct.pack('<i', dst_va-(src_va+instr_len))

data=bytearray(SRC.read_bytes())
patches=[]

def patch(va, expected, replacement, label):
    assert len(expected)==len(replacement),(label,len(expected),len(replacement))
    o=off(va)
    got=bytes(data[o:o+len(expected)])
    if got!=expected:
        raise RuntimeError(f'{label} @ {va:#x}: expected {expected.hex()} got {got.hex()}')
    data[o:o+len(replacement)]=replacement
    patches.append({'label':label,'va':hex(va),'file_offset':hex(o),'old':expected.hex(),'new':replacement.hex(),'bytes':len(expected)})

# A) Route only ThinkMode 3 into the existing local-control branch.
# Existing cmp eax,1 remains. JNE now goes to cave; mode 1 still falls through normally.
branch_cave=0x027A1318
patch(0x027A0CE7,
      bytes.fromhex('0f8593030000'),
      b'\x0f\x85'+rel32(0x027A0CE7,6,branch_cave),
      'ThinkMode non-1 -> discriminator cave')
# Cave: if eax==3 -> local branch 0x27A0CED, otherwise preserve original nonlocal target 0x27A1080.
code=b''
code += bytes.fromhex('83f803') # cmp eax,3
code += b'\x0f\x84'+rel32(branch_cave+len(code),6,0x027A0CED) # je local
code += b'\xe9'+rel32(branch_cave+len(code),5,0x027A1080) # jmp original nonlocal
patch(branch_cave, b'\xcc'*len(code), code, 'ThinkMode 3 -> local-control path cave')

# B) Make existing selector function return 1 only for ThinkMode 3; 0 otherwise.
# Preserve prologue + this save. Replace xor/pop/pop/pop beginning at 0x27A27D3 with jump to cave.
selector_cave=0x027A1340
patch(0x027A27D3,
      bytes.fromhex('33c05f5e5b'),
      b'\xe9'+rel32(0x027A27D3,5,selector_cave),
      'selector body -> mode-aware cave')
code=b''
code += bytes.fromhex('8b4df8') # mov ecx,[ebp-8] this
# call thunk that gets think/controller object
callsite=selector_cave+len(code); code += b'\xe8'+rel32(callsite,5,0x01C31524)
code += bytes.fromhex('8bc8') # mov ecx,eax
callsite=selector_cave+len(code); code += b'\xe8'+rel32(callsite,5,0x01B8660B) # get mode
code += bytes.fromhex('83f803') # cmp eax,3
code += bytes.fromhex('0f94c0') # sete al
code += bytes.fromhex('0fb6c0') # movzx eax,al
# epilogue originally at 0x27A27D5..DB
code += bytes.fromhex('5f5e5b8be55dc3')
patch(selector_cave,b'\xcc'*len(code),code,'selector cave: mode3=>1 else0')

# C) Restore the selector argument in PC sGamePad getters.
# Analog getter ABI uses aligned-stack EBX; selector is [ebx+0x0C].
analog_loads=[
    0x02DB15B7,0x02DB15C8, # move
    0x02DB1787,0x02DB1798, # aim analog
    0x02DB1957,0x02DB1968, # rotate
    0x02DB1CD7,0x02DB1CE8, # alternate rotate
]
for va in analog_loads:
    patch(va, bytes.fromhex('8b9170090000'), bytes.fromhex('8b530c909090'), f'sGamePad selector arg @ {va:#x}')

# Bool getter ABI: selector is [ebp+8], retain destination register.
bool_patches=[
    (0x02DAF404, bytes.fromhex('8b8270090000'), bytes.fromhex('8b4508909090'), 'aim bool index -> arg'),
    (0x02DAF40E, bytes.fromhex('8b9170090000'), bytes.fromhex('8b5508909090'), 'aim bool second index -> arg'),
    (0x02DAFFE5, bytes.fromhex('8b9170090000'), bytes.fromhex('8b5508909090'), 'run bool index -> arg'),
    (0x02DAFFEF, bytes.fromhex('8b8870090000'), bytes.fromhex('8b4d08909090'), 'run bool second index -> arg'),
]
for va,old,new,label in bool_patches:
    patch(va,old,new,label)

OUT.write_bytes(data)
sha=lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
manifest={
    'source':SRC.name,
    'source_sha256':sha(SRC),
    'output':OUT.name,
    'output_sha256':sha(OUT),
    'status':'STATICALLY VERIFIED ONLY - gameplay not executed in this environment',
    'intent':'Restore the existing sGamePad selector argument, route only inferred remote ThinkMode 3 through local control, and select pad index 1 only for that mode.',
    'patches':patches,
    'notes':[
        'ThinkMode names are inferred from behavior; mode 3 is strongly consistent with remote/network control but the numeric enum label is not yet recovered.',
        'Mode 2 is deliberately left on its original path.',
        'Mode 1 remains original local-control path and selector 0.',
        'No global mStartPadNo mutation is used.'
    ]
}
MAN.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(OUT)
print(manifest['output_sha256'])
print('patch records',len(patches),'patch bytes',sum(p['bytes'] for p in patches))
print(MAN)
