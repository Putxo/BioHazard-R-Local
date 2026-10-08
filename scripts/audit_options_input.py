#!/usr/bin/env python3
"""Static audit of January option input callsites. No game execution."""
from pathlib import Path
import argparse,hashlib,json,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'patches'))
from build_local_routing import PE
ORIGINAL='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
SITES=(
    (0x02C1C51B,'6a008b4dece8fbfb03ff','Options Menu/Top/Manager owner-scoped raw pad +0x198'),
    (0x02C2458B,'6a008b4dece88b7b03ff','Options Menu/Top/Manager owner-scoped raw pad +0x198'),
    (0x02C1C8DE,'6a008b4dece86febf7fe','Options Menu/Top/Manager owner-scoped raw pad +0x1A0'),
    (0x02C1DC7D,'6a008b4dece8d0d7f7fe','Options Menu/Top/Manager owner-scoped raw pad +0x1A0'),
    (0x02C1E954,'6a008b4dece8f9caf7fe','Options Menu/Top/Manager owner-scoped raw pad +0x1A0'),
    (0x02C1F002,'6a008b4dece84bc4f7fe','Options Menu/Top/Manager owner-scoped raw pad +0x1A0'),
    (0x02C1FC4B,'6a008b4dece802b8f7fe','Options Menu/Top/Manager owner-scoped raw pad +0x1A0'),
    (0x02C20626,'6a008b4dd4e827aef7fe','Options Menu/Top/Manager owner-scoped raw pad +0x1A0'),
    (0x02C2193D,'6a008b4dece8109bf7fe','Options Menu/Top/Manager owner-scoped raw pad +0x1A0'),
    (0x02C22525,'6a008b4de8e8288ff7fe','Options Menu/Top/Manager owner-scoped raw pad +0x1A0'),
    (0x02C244DB,'6a008b4dece8726ff7fe','Options Menu/Top/Manager owner-scoped raw pad +0x1A0'),
    (0x02C2464D,'6a008b4dece8006ef7fe','Options Menu/Top/Manager owner-scoped raw pad +0x1A0'),
    (0x02C248FB,'6a008b4dece8526bf7fe','Options Menu/Top/Manager owner-scoped raw pad +0x1A0'),
    (0x02C28262,'6a008b4dd4e8eb31f7fe','Options Menu/Top/Manager owner-scoped raw pad +0x1A0'),
    (0x02C2A13B,'6a008b4dece81213f7fe','Options Menu/Top/Manager owner-scoped raw pad +0x1A0'),
    (0x02C2CB4A,'6a00e8a3ec05ff8bc8e8ffe8f6fe','Options Menu/Top/Manager owner-scoped raw pad +0x1A0'),
    (0x02C1C8EB,'6a008b4dece823d6fbfe','Options Menu/Top/Manager owner-scoped raw pad +0x1AC'),
    (0x02C1DC8A,'6a008b4dece884c2fbfe','Options Menu/Top/Manager owner-scoped raw pad +0x1AC'),
    (0x02C1F00F,'6a008b4dece8ffaefbfe','Options Menu/Top/Manager owner-scoped raw pad +0x1AC'),
    (0x02C1FC58,'6a008b4dece8b6a2fbfe','Options Menu/Top/Manager owner-scoped raw pad +0x1AC'),
    (0x02C20633,'6a008b4dd4e8db98fbfe','Options Menu/Top/Manager owner-scoped raw pad +0x1AC'),
    (0x02C2194A,'6a008b4dece8c485fbfe','Options Menu/Top/Manager owner-scoped raw pad +0x1AC'),
    (0x02C22532,'6a008b4de8e8dc79fbfe','Options Menu/Top/Manager owner-scoped raw pad +0x1AC'),
    (0x02C2826F,'6a008b4dd4e89f1cfbfe','Options Menu/Top/Manager owner-scoped raw pad +0x1AC'),
    (0x02C2A148,'6a008b4dece8c6fdfafe','Options Menu/Top/Manager owner-scoped raw pad +0x1AC'),
    (0x02C1ADB0,'c700344cdf04','uGUI_OptionMenu vtable'),
    (0x02C2505E,'c700e44edf04','uGUI_OptionTop vtable'),
    (0x02C2BFAE,'c700e45adf04','uOptionManager vtable'),
    (0x01CB3003,'8b450869c0f80200008b4df88b840198010000','raw held reader stride and index'),
    (0x01CB3103,'8b45f88b8870090000518b4df8e8e854f9ff5f','raw decision reader'),
    (0x01CB31B3,'8b45f88b8870090000518b4df8e8bb5ef3ff5f','raw repeat reader'),
 )
def audit(data):
    if hashlib.sha256(data).hexdigest()!=ORIGINAL:raise ValueError('requires exact original January SHA256')
    pe=PE(data)
    for at,value,label in SITES:
        expected=bytes.fromhex(value)
        if pe.read(at,len(expected))!=expected:raise ValueError('witness mismatch: '+label)
    return {'status':'PASS','witnesses':len(SITES),'option_input_calls':25,'game_executed':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path)
    print(json.dumps(audit(p.parse_args().original.read_bytes())))
