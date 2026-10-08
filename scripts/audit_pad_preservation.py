#!/usr/bin/env python3
"""Static January pad preservation and action-flag contracts; never executes EXE."""
from pathlib import Path
import argparse,hashlib,json,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'patches'));sys.path.insert(0,str(ROOT/'tools'))
from build_local_routing import PE
from pad_state_sites import patches
ORIGINAL='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
SITES=(
    (0x01CB2F5E,'a180747a05','sGamePad singleton'),
    (0x02DAC77F,'e8e77ae2fe','physical polling precedes synthesis'),
    (0x02DAC8A1,'e889abe7fe','keyboard synthesis precedes PadData update'),
    (0x02DAC8D5,'e8e5f0e9fe','per-slot PadData update'),
    (0x02DB35B6,'83b874090000007505','keyboard mode gate'),
    (0x02DB3694,'6a2c6a008b45f8059801000050e81026e6fe83c40c','pad0 raw clear cdecl'),
    (0x02DB36A9,'6a2c6a008b45f8059004000050e8fb25e6fe83c40c','pad1 raw clear cdecl'),
    (0x01C15CB6,'e99532b602','memset thunk'),
    (0x04778F50,'8b54240c8b4c2404','memset reads size and destination'),
    (0x04778FBF,'8b4424085fc3','memset returns destination with plain ret'),
    (0x02DAE700,'558bec81ecd8000000','flag3 getter prologue'),
    (0x02DAE723,'6a038b45f8','flag3 selector'),
    (0x02DAE76C,'c20400','flag3 indexed ret4'),
    (0x02DAE870,'558bec81ecd8000000','flag4 getter prologue'),
    (0x02DAE893,'6a048b45f8','flag4 selector'),
    (0x02DAE8DC,'c20400','flag4 indexed ret4'),
    (0x02DBA9A3,'8b450850e8480eedfe8bc8e8598eebfe','action producer forwards index into flag3'),
    (0x02DBA6F7,'8b450850e8f410edfe8bc8e80d60ddfe','action producer forwards index into flag4'),
 )
def audit(data):
    if hashlib.sha256(data).hexdigest()!=ORIGINAL:raise ValueError('requires exact original January SHA256')
    pe=PE(data)
    for at,value,label in SITES:
        expected=bytes.fromhex(value)
        if pe.read(at,len(expected))!=expected:raise ValueError('witness mismatch: '+label)
    for at,old,new,label in patches():
        if pe.read(at,len(old))!=old:raise ValueError('state query mismatch: '+label)
    return {'status':'PASS','witnesses':len(SITES)+len(list(patches())),'game_executed':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path)
    print(json.dumps(audit(p.parse_args().original.read_bytes())))
