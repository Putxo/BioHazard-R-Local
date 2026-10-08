#!/usr/bin/env python3
"""Read-only January raw-pad producer evidence; does not execute game code."""
from pathlib import Path
import argparse, hashlib, json, sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'patches'))
sys.path.insert(0,str(ROOT/'tools'))
from build_local_routing import PE
from pad_axis_sites import patches
ORIGINAL='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
SITES=(
    (0x02DB11D0, '558bec81ecd0000000', 'standard EBP frame'),
    (0x02DB1260, '558bec81ecd0000000', 'standard EBP frame'),
    (0x02DB12F0, '558bec81ecd0000000', 'standard EBP frame'),
    (0x02DB1380, '558bec81ecd0000000', 'standard EBP frame'),
    (0x02DB1410, '558bec81ecd8000000', 'standard EBP frame'),
    (0x02DB14C0, '558bec81ecd8000000', 'standard EBP frame'),
    (0x02DB123F, 'c20400', 'one explicit argument ret4'),
    (0x02DB12CF, 'c20400', 'one explicit argument ret4'),
    (0x02DB135F, 'c20400', 'one explicit argument ret4'),
    (0x02DB13EF, 'c20400', 'one explicit argument ret4'),
    (0x02DB1498, 'c20400', 'one explicit argument ret4'),
    (0x02DB1548, 'c20400', 'one explicit argument ret4'),
    (0x02DB7107, '8b430850e8e446edfe8bc8e8b5a9e9fe', 'PadData first axis uses index from original frame'),
    (0x02DB7123, '8b430850e8c846edfe8bc8e8e5cbe9fe', 'PadData second axis uses same index'),
    (0x02DB73FC, '8b430850e8ef43edfe8bc8e87129e3fe', 'PadData other stick first axis'),
    (0x02DB7418, '8b430850e8d343edfe8bc8e835fee7fe', 'PadData other stick second axis'),
    (0x02DB756E, '8b430850e87d42edfe8bc8e8aef6e7fe', 'PadData digital fallback x'),
    (0x02DB7595, '8b430850e85642edfe8bc8e8a08decfe', 'PadData digital fallback y'),
    (0x02DAC8B8, '837de002', 'PadData update loop has two slots'),
    (0x02DAC8BE, '8b45e0508b4de069c9c00000008b55f88d8c0a68060000e8e5f0e9fe', 'PadData update receives loop index'),
    (0x01CB32C3, '8b450869c0f80200008b4df88b8401b8010000', 'raw stick uses argument times 2F8'),
    (0x01CB3373, '8b450869c0f80200008b4df88b8401bc010000', 'raw stick uses argument times 2F8'),
    (0x01CB3423, '8b450869c0f80200008b4df88b8401b0010000', 'raw stick uses argument times 2F8'),
    (0x01CB34D3, '8b450869c0f80200008b4df88b8401b4010000', 'raw stick uses argument times 2F8'),
    (0x01CB3003, '8b450869c0f80200008b4df88b840198010000', 'raw button reader uses argument times 2F8'),
 )
def audit(data):
    if hashlib.sha256(data).hexdigest()!=ORIGINAL:raise ValueError('requires exact original January SHA256')
    pe=PE(data)
    for at,expected,label in SITES:
        value=bytes.fromhex(expected)
        if pe.read(at,len(value))!=value:raise ValueError('witness mismatch: '+label)
    for at,old,new,label in patches():
        if pe.read(at,len(old))!=old:raise ValueError('axis source mismatch: '+label)
    return {'status':'PASS','witnesses':len(SITES)+len(list(patches())),
            'source_sha256':ORIGINAL,'game_executed':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path)
    print(json.dumps(audit(p.parse_args().original.read_bytes())))
