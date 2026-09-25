#!/usr/bin/env python3
"""Read-only evidence for January HUD ownership; no patch/build/engine execution.

Only the hash-pinned original is accepted. The witness checks are structural
regressions, not proof that GUI coordinates or gameplay work in two viewports.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

SHA = '9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
SIZE = 60_748_800

class Image:
    def __init__(self, data: bytes):
        if len(data) != SIZE or hashlib.sha256(data).hexdigest() != SHA:
            raise ValueError('requires exact unmodified January image')
        self.data = data
        u16 = lambda n: struct.unpack_from('<H', data, n)[0]
        u32 = lambda n: struct.unpack_from('<I', data, n)[0]
        pe = u32(0x3c)
        if data[:2] != b'MZ' or data[pe:pe+4] != b'PE\0\0' or u16(pe+4) != 0x14c:
            raise ValueError('not i386 PE')
        opt = pe+24
        if u16(opt) != 0x10b:
            raise ValueError('not PE32')
        self.base = u32(opt+28)
        start = opt+u16(pe+20)
        self.sections = []
        for i in range(u16(pe+6)):
            _, rva, raw_size, raw = struct.unpack_from('<4I', data, start+i*40+8)
            if raw+raw_size > len(data):
                raise ValueError('section outside image')
            self.sections.append((self.base+rva, raw_size, raw))
    def read(self, va: int, size: int) -> bytes:
        if size < 0:
            raise ValueError('negative size')
        for base, length, raw in self.sections:
            if base <= va and va+size <= base+length:
                return self.data[raw+va-base:raw+va-base+size]
        raise ValueError(f'VA outside backed sections: {va:#x}')
    def u32(self, va: int) -> int:
        return struct.unpack('<I', self.read(va, 4))[0]
    def cstr(self, va: int) -> str:
        result = bytearray()
        for i in range(512):
            byte = self.read(va+i, 1)
            if byte == b'\0':
                return result.decode('ascii')
            result.extend(byte)
        raise ValueError('unterminated RTTI name')


def audit(data: bytes) -> dict:
    pe = Image(data)
    checks = []
    def exact(va, expected, label):
        if pe.read(va, len(expected)) != expected:
            raise ValueError(f'{label} at {va:#x}')
        checks.append(dict(va=hex(va), size=len(expected), label=label))
    def hexes(va, text, label):
        exact(va, bytes.fromhex(text), label)
    def ptr(va, value, label):
        exact(va, struct.pack('<I', value), label)
    def relative(va, target, label, opcode=0xe8):
        exact(va, bytes([opcode])+struct.pack('<i', target-va-5), label)
    owners = []
    for vt, col, td, name, cockpit in (
        (0x04DE5C14,0x0537C8E8,0x054CEFA4,'uCockpitManagerMain',False),
        (0x04DE86FC,0x0537D3F4,0x054D0D50,'uMiniMapManager',False),
        (0x04DE52C4,0x0537C5A8,0x054CEB48,'uGUI_Reticle',True),
        (0x04DE4C3C,0x0537C408,0x054CE908,'uGUI_MainEquipWin',True),
        (0x04DE810C,0x0537D144,0x054D09E0,'uGUI_MapBaseAndHerb',False),
    ):
        ptr(vt-4,col,name+' COL');ptr(col+12,td,name+' descriptor')
        exact(td+8,('.?AV'+name+'@gui@game@app@@\0').encode(),name+' RTTI')
        chd=pe.u32(col+16);count=pe.u32(chd+8);array=pe.u32(chd+12)
        if not 1 <= count <= 16:
            raise ValueError('unexpected base count')
        bases=[pe.cstr(pe.u32(pe.u32(array+4*i))+8) for i in range(count)]
        if ('.?AVuBioCockpitGUI@gui@game@app@@' in bases) != cockpit:
            raise ValueError('cockpit inheritance mismatch: '+name)
        checks.append(dict(va=hex(chd),label=name+' inheritance',size=16))
        owners.append(dict(name=name,vtable=hex(vt),bases=bases))
    for vt,slot,thunk,target in (
        (0x04DE5C14,8,0x01C08CFA,0x02B492D0),
        (0x04DE5C14,9,0x01BA3F94,0x02B49A60),
        (0x04DE5C14,11,0x01C756DE,0x02B49DA0),
        (0x04DE86FC,8,0x01C4ACBD,0x02B68430),
        (0x04DE86FC,9,0x01B909DA,0x02B68590),
        (0x04DE86FC,11,0x01C6266F,0x02B686A0),
        (0x04DE52C4,9,0x01C7E437,0x02B40480),
    ):
        ptr(vt+4*slot,thunk,f'vslot {slot}');relative(thunk,target,'virtual implementation',0xe9)
    # Verified 21-slot construction: 0x8C is NOT a link-list member.
    va=0x02B4C496
    slots=list(range(0x3c,0x8c,4))+[0x90]
    for slot in slots:
        code=b'\x8b\x48'+bytes([slot]) if slot<0x80 else b'\x8b\x88'+struct.pack('<I',slot)
        exact(va,code,'cockpit source slot '+hex(slot));va+=len(code)+6
    for va,text,label in (
        (0x02B4C56E,'837d9015','21-slot limit'),
        (0x02B4C5EF,'6a00','last link cleared'),
        (0x02B4C719,'898890020000','cockpit next setter'),
        (0x02B49A06,'8b8090020000','cockpit next getter'),
        (0x02B48F53,'898890000000','reticle owned at manager+90'),
        (0x02B47AA9,'ffd0','owned widget destructor call'),
        (0x02B47AC7,'c7403c00000000','owned slot cleared'),
        (0x02B681C7,'894840','herb named pointer at mini+40'),
        (0x02B681D6,'890c90','herb also in mini unit array'),
        (0x02B6807E,'837dec04','mini destructor four entries'),
        (0x02B680EB,'c744813000000000','mini owned slot cleared'),
        (0x0279D1C6,'8b400cc1e81025ff030000','ten-bit draw-view getter'),
        (0x01EB6153,'8b450825ff030000c1e010','draw-view setter mask'),
        (0x01EB6164,'81e2ffff00fc0bd0','preserve unrelated unit flags'),
        (0x01EB616F,'89500c','store draw-view packed flags'),
        (0x02B49EF6,'8b805801000025ff000000','draw-context view index'),
        (0x02B49E26,'2345ec7429','cockpit AND mask and skip'),
        (0x02B687C0,'2345e07426','mini AND mask and skip'),
        (0x02B404A0,'894df8','reticle frame holds this'),
        (0x02B3B690,'894df8','equipment frame holds this'),
        (0x02B61D80,'894df8','herb frame holds this'),
        (0x02B404CE,'837dec007505e9c1010000','reticle null exits before pack'),
        (0x02B3B6D0,'837dec007505e99f030000','equipment null exits before pack'),
        (0x02B61F44,'837dbc000f8407010000','herb first null guard'),
        (0x02B62188,'837dbc000f847f010000','herb null exits before pack'),
        (0x01CB74B9,'c20400','Self finder pops opaque argument'),
    ):
        hexes(va,text,label)
    for va,target,label in (
        (0x02B48F80,0x01BFFE7F,'create builds list'),
        (0x02B4C5B3,0x01C52F1C,'link next widget'),
        (0x02B4C5FB,0x01C52F1C,'clear final next'),
        (0x02B681A7,0x01C6392A,'mini creates real herb class'),
        (0x02B49E21,0x01BE2D20,'cockpit draw-view getter'),
        (0x02B687BB,0x01BE2D20,'mini draw-view getter'),
        (0x02B404C6,0x01C2C7F4,'reticle Self'),
        (0x02B3B6C8,0x01C2C7F4,'equipment Self'),
        (0x02B61F3C,0x01C2C7F4,'herb Self'),
    ):
        relative(va,target,label)
    exact(0x03478231,b'\x68'+struct.pack('<I',0x01C61814),'metadata view setter')
    exact(0x0347823F,b'\x68'+struct.pack('<I',0x01BE2D20),'metadata view getter')
    exact(0x0347824D,b'\x68'+struct.pack('<I',0x04F145F8),'metadata view name')
    exact(0x04F145F8,b'mDrawView\0','literal metadata name')
    return dict(status='PASS_READ_ONLY_EVIDENCE',original_sha256=SHA,size=len(data),
        checks=len(checks),witnesses=checks,owners=owners,cockpit_list_slots=slots,
        gameplay_executed=False,game_image_modified=False,
        hud_installed=False,projection_or_coordinates_validated=False)


def main(argv=None):
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('original',type=Path);p.add_argument('--report',required=True,type=Path)
    a=p.parse_args(argv)
    created=False
    try:
        if a.original.resolve()==a.report.resolve() or os.path.lexists(a.report):
            raise ValueError('report destination must be a new, distinct file')
        report=audit(a.original.read_bytes())
        with a.report.open('x',encoding='utf-8') as f:
            created=True;json.dump(report,f,indent=2);f.write('\n')
        print(json.dumps({k:v for k,v in report.items() if k not in ('witnesses','owners')}))
        return 0
    except (OSError,ValueError,struct.error) as exc:
        if created:a.report.unlink(missing_ok=True)
        print('ERROR: '+str(exc),file=sys.stderr);return 2

if __name__=='__main__':raise SystemExit(main())
