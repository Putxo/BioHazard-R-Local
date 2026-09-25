#!/usr/bin/env python3
"""Read-only January HUD evidence. Never patches files or executes the engine."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'patches'))
from build_local_routing import PE, sha
ORIGINAL_SHA='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
CANDIDATE_SHA='71f5e70dc19c334d303ee29a686d65a72cd18b0c87b98472170bff961cb048f4'

def audit(data: bytes, candidate: bytes | None=None) -> dict:
    if len(data)!=60_748_800 or sha(data)!=ORIGINAL_SHA:
        raise ValueError('requires exact unmodified January executable')
    pe=PE(data); records=[]
    def exact(va, expected, label):
        if pe.read(va,len(expected))!=expected:raise ValueError(label)
        records.append(dict(va=hex(va),size=len(expected),expected=expected.hex(),label=label))
    def ptr(va,value,label):exact(va,struct.pack('<I',value),label)
    def call(va,target,label):exact(va,b'\xE8'+struct.pack('<i',target-va-5),label)
    def thunk(va,target,label):exact(va,b'\xE9'+struct.pack('<i',target-va-5),label)
    ptr(0x04DE8108,0x0537D144,'regular HUD RTTI COL')
    ptr(0x0537D150,0x054D09E0,'regular HUD type descriptor')
    exact(0x054D09E8,b'.?AVuGUI_MapBaseAndHerb@gui@game@app@@\0','regular HUD RTTI name')
    exact(0x054D0B18,b'.?AVuGUI_MapBaseAndHerbBlur@gui@game@app@@\0','separate Blur type, not a player role')
    ptr(0x04DE810C+8*4,0x01C28CAD,'regular HUD update vslot8')
    thunk(0x01C28CAD,0x02B61D60,'regular HUD update')
    exact(0x04A9BEC2,bytes.fromhex('68c0020000'),'regular HUD registered size')
    exact(0x04A9BECD,bytes.fromhex('680c83de04'),'regular HUD DTI name reference')
    exact(0x04A9BED2,bytes.fromhex('b98c5d5905'),'regular HUD DTI global')
    call(0x02B61F3C,0x01C2C7F4,'Self actor resolver')
    thunk(0x01C2C7F4,0x01CB7400,'Self resolver implementation')
    exact(0x02B61F41,bytes.fromhex('8945bc'),'store resolved actor in frame -44')
    exact(0x02B62192,bytes.fromhex('8b4dbc'),'same resolved actor supplies inventory')
    call(0x02B62195,0x01BCC327,'actor pack getter')
    call(0x02B621A0,0x01C2A51C,'herb count getter')
    thunk(0x01C2A51C,0x0243CE90,'herb getter implementation')
    exact(0x0243CEB6,bytes.fromhex('8b80c8000000'),'pack herb count at C8')
    call(0x02B621A9,0x01BC6251,'regular HUD count setter')
    thunk(0x01BC6251,0x02B62880,'regular HUD count setter implementation')
    exact(0x02B628B7,bytes.fromhex('898894020000'),'single per-instance cached count at 294')
    exact(0x02B6269A,bytes.fromhex('0594020000'),'metadata field offset 294')
    exact(0x02B626A0,bytes.fromhex('68bc82de04'),'metadata name reference')
    exact(0x04DE82BC,b'mHerbHaveNum\0','metadata name mHerbHaveNum')
    thunk(0x01B96FDD,0x02B63DE0,'Blur setter is different')
    exact(0x02B63E00,bytes.fromhex('894df85f5e5b8be55dc20400'),'Blur count setter empty ret4')
    preserved=None
    if candidate is not None:
        if sha(candidate)!=CANDIDATE_SHA:raise ValueError('unrecognized candidate')
        cp=PE(candidate)
        for record in records:
            if cp.read(int(record['va'],16),record['size'])!=bytes.fromhex(record['expected']):
                raise ValueError('candidate unexpectedly changes HUD evidence')
        preserved=True
    return dict(status='PASS_STATIC_EVIDENCE_ONLY',original_sha256=ORIGINAL_SHA,
        checks=len(records),records=records,candidate_hud_evidence_unchanged=preserved,
        gameplay_executed=False,hud_2p_implemented=False,
        finding='Audited regular HUD update obtains Self and writes one herb count per HUD instance.',
        next_step='Identify HUD creation, lifetime and viewport/actor ownership before adding a second independent presentation.')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('original',type=Path);p.add_argument('--candidate',type=Path)
    p.add_argument('--report',type=Path,required=True);args=p.parse_args()
    try:
        report=audit(args.original.read_bytes(),args.candidate.read_bytes() if args.candidate else None)
        with args.report.open('x',encoding='utf8') as f:json.dump(report,f,indent=2);f.write('\n')
        print(json.dumps({k:v for k,v in report.items() if k!='records'}))
    except (OSError,ValueError,struct.error) as exc:
        print('ERROR: '+str(exc),file=sys.stderr);raise SystemExit(2)
