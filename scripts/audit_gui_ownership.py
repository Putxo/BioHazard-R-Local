#!/usr/bin/env python3
"""Read-only January GUI/scheduler ownership evidence; never executes the game."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'patches'))
from build_script_member import Image
ORIGINAL = '9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'

def audit(data: bytes) -> dict:
    if len(data) != 60_748_800 or hashlib.sha256(data).hexdigest() != ORIGINAL:
        raise ValueError('not the exact original January image')
    pe = Image(data)
    checks=[]
    def require(ok: bool, label: str):
        if not ok: raise ValueError('evidence mismatch: '+label)
        checks.append(label)
    def u32(va: int) -> int: return struct.unpack('<I',pe.read(va,4))[0]
    def call(va: int, target: int, label: str):
        expected=b'\xe8'+struct.pack('<i',target-va-5)
        require(pe.read(va,5)==expected,label)
    def rtti(vt: int, name: str):
        col=u32(vt-4);td=u32(col+12);encoded=name.encode()+b'\0'
        require(pe.read(td+8,len(encoded))==encoded,'RTTI '+name)
        return {'vtable':hex(vt),'col':hex(col),'type_descriptor':hex(td),'name':name}
    classes=[rtti(vt,name) for vt,name in (
        (0x04DE5C14,'.?AVuCockpitManagerMain@gui@game@app@@'),
        (0x04DE98B4,'.?AVuBioCockpitGUI@gui@game@app@@'),
        (0x04DE52C4,'.?AVuGUI_Reticle@gui@game@app@@'),
        (0x04DE4C3C,'.?AVuGUI_MainEquipWin@gui@game@app@@'),
        (0x04DE810C,'.?AVuGUI_MapBaseAndHerb@gui@game@app@@'),
        (0x04DF5E5C,'.?AVuGUI_PauseHD@gui@game@app@@'))]
    for va in (0x02B404C6,0x02B3B6C8,0x02B61F3C,0x02C30A06):
        call(va,0x01C2C7F4,'GUI Self finder '+hex(va))
    require(pe.read(0x01C2C7F4,5)==b'\xe9'+struct.pack('<i',0x01CB7400-0x01C2C7F4-5),'Self finder thunk')
    call(0x01CB7480,0x01BB3192,'finder invokes Self predicate')
    require(pe.read(0x01BB3192,5)==b'\xe9'+struct.pack('<i',0x01CB7560-0x01BB3192-5),'Self predicate thunk')
    call(0x01CB7599,0x01BEDBB7,'Self predicate reads actor serial')
    require(pe.read(0x01CB759E,4)==bytes.fromhex('3bf0751b'),'Self predicate requires serial equality')
    call(0x02B4054D,0x01BCC327,'reticle uses selected actor pack')
    call(0x02B3B6DE,0x01BCC327,'equipment uses selected actor pack')
    call(0x02B62195,0x01BCC327,'herb UI uses selected actor pack')
    call(0x02B40168,0x01BEF075,'reticle calls cockpit base constructor')
    call(0x02B3B0A8,0x01BEF075,'equipment calls cockpit base constructor')
    require(pe.read(0x01BEF075,5)==b'\xe9'+struct.pack('<i',0x02B79700-0x01BEF075-5),'cockpit constructor thunk')
    require(pe.read(0x02B79733,6)==bytes.fromhex('c700b498de04'),'cockpit constructor vtable')
    call(0x02B48F33,0x01C19CCB,'manager creates reticle')
    require(pe.read(0x02B48F53,6)==bytes.fromhex('898890000000'),'manager stores reticle at +90')
    require(pe.read(0x02B477CE,6)==bytes.fromhex('c700145cde04'),'manager constructor vtable')
    # The parent discriminator is uScheduler, not uPlayer/uNpc.
    require(pe.read(0x02DF1B60,5)==bytes.fromhex('683c967905'),'uPcs parent checked cast uses scheduler DTI')
    require(pe.read(0x04B00AD4,5)==bytes.fromhex('b93c967905'),'scheduler DTI registration target')
    require(pe.read(0x04B00ACF,5)==bytes.fromhex('68845dec04'),'scheduler DTI name pointer')
    require(pe.read(0x04EC5D84,11)==b'uScheduler\0','parent DTI literal name')
    return {'status':'STATIC_GUI_AND_PARENT_EVIDENCE','source_sha256':ORIGINAL,
            'assertions':len(checks),'checks':checks,'classes':classes,
            'gui_self_calls':['0x02B404C6','0x02B3B6C8','0x02B61F3C','0x02C30A06'],
            'scheduler_dti':'0x0579963C','gameplay_executed':False,
            'hud_patch_created':False,
            'limits':['This audit does not duplicate HUD, route menu ownership or validate rendering.',
                      'The manager reticle slot is per instance; this does not prove a global instance count.',
                      'A uScheduler parent is not sufficient evidence for a unique player owner.']}

def main() -> int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('original',type=Path);p.add_argument('--report',type=Path)
    a=p.parse_args()
    try:
        report=audit(a.original.read_bytes())
        if a.report:
            if a.report.resolve()==a.original.resolve():raise ValueError('report must not replace original')
            with a.report.open('x') as h:json.dump(report,h,indent=2);h.write('\n')
        print(json.dumps(report,indent=2))
        return 0
    except (OSError,ValueError,struct.error) as e:
        print('ERROR: '+str(e),file=sys.stderr);return 2
if __name__=='__main__':raise SystemExit(main())
