#!/usr/bin/env python3
"""SHA-pinned static witnesses for script input ownership; never executes the image."""
from pathlib import Path
import argparse,hashlib,json,sys,struct
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'patches'))
from build_local_routing import PE
ORIGINAL='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
SITES=(
    (0x02DEE8D0,'558bec81eccc000000','callback prologue replaced'),
    (0x02DEE8F3,'33c0','stock member zero'),
    (0x02DEE8FB,'c20400','thiscall ret4'),
    (0x01C0758A,'e941731e01','callback thunk'),
    (0x02DEE4E0,'894df8','constructor saves ECX'),
    (0x02DEE4EE,'c700f45ee104','uPcsInput exact vtable'),
    (0x02DEE53C,'8b4df851688a75c001','delegate captures input this and callback'),
    (0x02DEE623,'8b4df883c150e83c0ce9fe','command at input+50'),
    (0x02DEE727,'c7403000000000','parent initially null'),
    (0x02DEDC8E,'e89694e7fe','initialization resolves parent'),
    (0x02DEDC96,'8b4830516a00e8d1b7dafe','parent passed to manager singleton'),
    (0x02DEDCA6,'e89b87e6fe8b55f8894234','native result stored as script group+34'),
    (0x02DEDF72,'83783000','parent search only when missing'),
    (0x02DEDFA2,'683c967905','scheduler RTTI'),
    (0x02DEDFF4,'e807e6e8fe3b45f87509','native child compared to input'),
    (0x02DEE004,'894830','matched scheduler stored at+30'),
    (0x01C56446,'e935a81701','group resolver thunk'),
    (0x02DD0CDC,'837dcc10','16 script groups'),
    (0x02DD0CF3,'69c970100000','main stride1070'),
    (0x02DD0CFC,'03883c020000','main array+23C'),
    (0x02DD0D3A,'837dc011','17 sub slots per group'),
    (0x02DD0D49,'69c980100000','sub stride1080'),
    (0x02DD0D55,'038c8240020000','sub arrays+240'),
    (0x01C07D1E,'e91d911c01','scheduler getter thunk'),
    (0x02DD0E66,'8b80900f0000','FSM scheduler+F90'),
    (0x02A437FE,'c700acbfdc04','main constructor vtable'),
    (0x02A75BEE,'c7001cf4dc04','sub constructor vtable'),
    (0x01F01A8E,'8d048570655605','manager singleton table address'),
 )
def audit(data):
    if hashlib.sha256(data).hexdigest()!=ORIGINAL:raise ValueError('requires exact original January SHA256')
    pe=PE(data)
    for at,value,label in SITES:
        expected=bytes.fromhex(value)
        if pe.read(at,len(expected))!=expected:raise ValueError('witness mismatch: '+label)
    for vt,name in ((0x04E15EF4,b'.?AVuPcsInput@pcs@game@app@@'),
                    (0x04E13248,b'.?AVsPcsManager@pcs@game@app@@'),
                    (0x04DCBFAC,b'.?AVcFsmActionPcsMain@fsm@game@app@@'),
                    (0x04DCF41C,b'.?AVcFsmActionPcsSub@fsm@game@app@@')):
        col=struct.unpack('<I',pe.read(vt-4,4))[0]
        td=struct.unpack('<I',pe.read(col+12,4))[0]
        if pe.read(td+8,len(name)+1)!=name+b'\0':raise ValueError('RTTI mismatch')
    return {'status':'PASS','witnesses':len(SITES),'rtti_types':4,'script_input_hooks':1,'game_executed':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);a=p.parse_args()
    print(json.dumps(audit(a.original.read_bytes())))
