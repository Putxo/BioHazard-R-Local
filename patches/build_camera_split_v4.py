#!/usr/bin/env python3
from pathlib import Path
import hashlib, struct

ORIG = Path('/mnt/data/BioRevHD 30-Enero-2013.exe')
INPUT_V3 = Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP P2 INPUT v3.exe')
CAVE_BIN = Path('/mnt/data/rev1_local_coop_research/camera_split_v4.bin')
OUT_CAMERA = Path('/mnt/data/BioRevHD 30-Enero-2013 CAMERA SPLIT v4.exe')
OUT_COMBINED = Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v4 NATIVE SPLIT.exe')

TEXT_DELTA = 0x01B78C00
HOOK_VA = 0x0203E3A9
CAVE_VA = 0x0203E3BD
NEXT_FN_VA = 0x0203E470

def raw(va): return va - TEXT_DELTA

def sha256(p):
    h=hashlib.sha256(); h.update(p.read_bytes()); return h.hexdigest()

def patch(src, out):
    data = bytearray(src.read_bytes())
    cave = CAVE_BIN.read_bytes()
    assert len(cave) <= NEXT_FN_VA-CAVE_VA, (len(cave), NEXT_FN_VA-CAVE_VA)
    ho = raw(HOOK_VA); co=raw(CAVE_VA)
    expected_hook = bytes.fromhex('5f5e5b81c4')
    if data[ho:ho+5] != expected_hook:
        raise RuntimeError(f'{src.name}: hook bytes mismatch: {data[ho:ho+5].hex()}')
    expected_cave = b'\xCC'*len(cave)
    if data[co:co+len(cave)] != expected_cave:
        raise RuntimeError(f'{src.name}: cave is not pristine CC padding')
    rel = CAVE_VA - (HOOK_VA + 5)
    data[ho:ho+5] = b'\xE9' + struct.pack('<i', rel)
    data[co:co+len(cave)] = cave
    out.write_bytes(data)
    return len(cave)

def ranges(a,b):
    out=[]; s=None
    for i,(x,y) in enumerate(zip(a,b)):
        if x!=y and s is None:s=i
        if x==y and s is not None:out.append((s,i));s=None
    if s is not None:out.append((s,min(len(a),len(b))))
    if len(a)!=len(b):out.append((min(len(a),len(b)),max(len(a),len(b))))
    return out

cave_len=patch(ORIG, OUT_CAMERA)
patch(INPUT_V3, OUT_COMBINED)
base=ORIG.read_bytes()
for p in [OUT_CAMERA, OUT_COMBINED]:
    b=p.read_bytes(); rr=ranges(base,b)
    print(p)
    print('sha256',sha256(p))
    print('size',len(b),'same',len(b)==len(base))
    print('diff_bytes',sum(e-s for s,e in rr),'ranges',len(rr))
    for s,e in rr: print(f'  0x{s:08X}-0x{e-1:08X} {e-s}')
print('cave_len',cave_len,'cave_capacity',NEXT_FN_VA-CAVE_VA)
