#!/usr/bin/env python3
"""Pin the January ThinkMode setter contract without executing native code."""
from pathlib import Path
import argparse
import hashlib
import json
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'patches'))
from build_local_routing import PE
ORIGINAL='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
SITES=(
    (0x01BB8B60,'e9db40bd00','full setter thunk'),
    (0x0278CC40,'558bec81eccc000000','displaced full prologue'),
    (0x0278CC6A,'e8e86545ff','base player setter call'),
    (0x0278CC76,'e8a86547ff','second state manager lookup'),
    (0x0278CC7D,'e81f6c4aff','second state manager mode propagation'),
    (0x0278CC95,'c20400','thiscall one argument cleanup'),
    (0x01BE3257,'e934e0c000','base setter thunk'),
    (0x027F12B6,'83b8400e000003','old actor mode is Cpu3'),
    (0x027F12BF,'837d0803','requested Cpu3 skips leaving-CPU cleanup'),
    (0x027F12C8,'81c1dc0e0000','CPU controller member'),
    (0x027F12E7,'e8192239ff','leaving-CPU cleanup'),
    (0x027F12FA,'e8a22544ff','primary manager mode propagation'),
    (0x027F1305,'8988400e0000','actor mode cache update'),
    (0x027A0CE4,'83f8010f8593030000','input Pad1 branch'),
    (0x01F37EC3,'6a03','native character setup requests Cpu3'),
    (0x01F37ED7,'8b8210010000ffd0','character setup virtual mode call'),
    (0x01EAA6EF,'6a03','native reassignment requests Cpu3'),
    (0x01EAA6F9,'8b8210010000ffd0','reassignment virtual mode call'),
)
def audit(data):
    if hashlib.sha256(data).hexdigest()!=ORIGINAL:raise ValueError('requires exact original January SHA256')
    pe=PE(data)
    for at,expected,label in SITES:
        value=bytes.fromhex(expected)
        if pe.read(at,len(value))!=value:raise ValueError('witness mismatch: '+label)
    return {'status':'PASS','witnesses':len(SITES),'source_sha256':ORIGINAL,'game_executed':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path)
    print(json.dumps(audit(p.parse_args().original.read_bytes())))
