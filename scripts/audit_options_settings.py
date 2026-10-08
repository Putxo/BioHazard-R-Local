#!/usr/bin/env python3
"""Static audit of January option settings ownership. No game execution."""
from pathlib import Path
import argparse,hashlib,json,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'patches'))
from build_local_routing import PE
ORIGINAL='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
SITES=(
    (0x02C2C641,'e88611f5fe','rev_menu_gate_options_current'),
    (0x02C2C64F,'e87811f5fe','rev_menu_gate_options_reference'),
    (0x02C2CF0E,'e8c3d1fefe','rev_menu_gate_options_apply'),
    (0x02DAE930,'8b8870090000','rev_menu_gate_options_ecx'),
    (0x02DAE960,'8b8870090000','rev_menu_gate_options_ecx'),
    (0x02DAE9CA,'8b8870090000','rev_menu_gate_options_ecx'),
    (0x02DAE9FA,'8b8870090000','rev_menu_gate_options_ecx'),
    (0x02DAEA5B,'8b8870090000','rev_menu_gate_options_ecx'),
    (0x02DAEA89,'8b8870090000','rev_menu_gate_options_ecx'),
    (0x02DAEBE6,'8b8870090000','rev_menu_gate_options_ecx'),
    (0x02DAECF6,'8b8870090000','rev_menu_gate_options_ecx'),
    (0x02DAEE06,'8b8870090000','rev_menu_gate_options_ecx'),
    (0x02DAEF16,'8b8870090000','rev_menu_gate_options_ecx'),
    (0x02DAE98C,'8b9170090000','rev_menu_gate_options_edx'),
    (0x02DAEA26,'8b9170090000','rev_menu_gate_options_edx'),
    (0x02C2C63B,'8b4df883c14ce88611f5fe','editable parameter at manager+4C'),
    (0x02C2C646,'8b4df881c150010000e87811f5fe','reference parameter at manager+150'),
    (0x02C2CF06,'6a018b4df883c14ce8c3d1fefe0fb6d083fa01','apply save=1 and AL result'),
    (0x02C2BFAE,'c700e45adf04','uOptionManager vtable'),
    (0x01B7D7CC,'e9dfbe0901','read thunk'),
    (0x01C1A0D6,'e9b5fcff00','apply thunk'),
    (0x01C0AE6A,'e90112ea00','publish thunk'),
    (0x02C196DC,'e8c82cfbfe8b4df8894104','read layout into config+4'),
    (0x02C196E7,'8b45f88378040675146a00e8fd2007ff8bc8e85d2203ff8b4df8894104','keyboard layout6 uses previous layout'),
    (0x02C1970D,'e85757fcfe8b4df8894108','read config+8'),
    (0x02C19721,'e88d5702ff8b4df889410c','read config+C'),
    (0x02C19735,'e81205fdfe8b4df8894110','read config+10'),
    (0x02C19749,'e80375fbfe8b4df8894114','read config+14'),
    (0x02C19DB3,'0fb6450883f801751ce8b71f05ff0fb6c085c074108b45f850e867dbfcfe8bc8e89210fffe','publish only if low save byte equals1 and singleton exists'),
    (0x02C19DED,'8b45f88b4804516a008b4dece8062a00ff','apply layout'),
    (0x02C19DFE,'8b45f88b4808516a008b4dece894a3f6fe','apply config+8'),
    (0x02C19E0F,'8b45f88b480c516a008b4dece86223fcfe','apply config+C'),
    (0x02C19E20,'8b45f88b4810516a008b4dece840f701ff','apply config+10'),
    (0x02C19E31,'8b45f88b4814516a008b4dece82ed6f6fe','apply config+14'),
    (0x02C1A03F,'b001eb0232c0','AL indicates change, not general success'),
    (0x02C1A058,'c20800','apply thiscall ret8'),
    (0x01DAD59E,'833df4265905000f95c0','global parameter existence'),
    (0x01DAD5DE,'a1f4265905','global parameter singleton'),
    (0x02AAC0A4,'68040100008b4508508b4df883c13c51e8bb731cff83c40c','copy104 bytes into global+3C'),
    (0x02AAC0CF,'c20400','publication thiscall ret4'),
 )

def audit(data):
    if hashlib.sha256(data).hexdigest()!=ORIGINAL:raise ValueError('requires exact original January SHA256')
    pe=PE(data)
    for at,value,label in SITES:
        expected=bytes.fromhex(value)
        if pe.read(at,len(expected))!=expected:raise ValueError('witness mismatch: '+label)
    return {'status':'PASS','witnesses':len(SITES),'option_settings_hooks':15,'game_executed':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path)
    print(json.dumps(audit(p.parse_args().original.read_bytes())))
