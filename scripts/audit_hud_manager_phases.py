#!/usr/bin/env python3
"""Read-only source-image witnesses for manager-scoped GUI phase gateways.

No process access, hooking, patching or game image generation. Addresses below
are instruction witnesses, not proof of a render fence or allocation lifetime.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

SOURCE_SHA = '9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
SOURCE_SIZE = 60_748_800
SITES = (
    (0x02B497CA, 'cockpit', 8, '8b45f88b483c'),
    (0x02B49C5B, 'cockpit', 9, '8b45f88b483c'),
    (0x02B49DE3, 'cockpit', 11, '8b45f88b483c'),
    (0x02B6847B, 'minimap', 8, '8b45f883c030'),
    (0x02B685B3, 'minimap', 9, '8b45f883c030'),
    (0x02B68724, 'minimap', 11, '8b45f883c030'),
)

def audit(data: bytes) -> dict:
    if len(data) != SOURCE_SIZE or hashlib.sha256(data).hexdigest() != SOURCE_SHA:
        raise ValueError('requires the exact unmodified January 2013 source image')
    pe = struct.unpack_from('<I', data, 0x3C)[0]
    if data[:2] != b'MZ' or data[pe:pe+4] != b'PE\0\0':
        raise ValueError('not a PE image')
    opt = pe+24
    base = struct.unpack_from('<I', data, opt+28)[0]
    table = opt+struct.unpack_from('<H', data, pe+20)[0]
    sections = [struct.unpack_from('<8sIIII',data,table+40*i)
                for i in range(struct.unpack_from('<H',data,pe+6)[0])]
    def read(va, size):
        for _, _, rva, raw_size, raw in sections:
            delta=va-base-rva
            if 0 <= delta and delta+size <= raw_size:
                return data[raw+delta:raw+delta+size]
        raise ValueError('witness outside file-backed section')
    records=[]
    def check(va, expected, label):
        if read(va,len(expected)) != expected:
            raise ValueError(f'{label} @ {va:#x}')
        records.append(dict(va=hex(va),expected=expected.hex(),label=label))
    def call(va, target, label):
        check(va,b'\xE8'+struct.pack('<i',target-va-5),label)
    def pointer(va, target, label):
        check(va,struct.pack('<I',target),label)
    for va, family, phase, expected in SITES:
        check(va,bytes.fromhex(expected),f'{family} ordinary loop phase {phase}')
    for slot,target in zip((8,9,11),(0x01C08CFA,0x01BA3F94,0x01C756DE)):
        pointer(0x04DE5C14+4*slot,target,'cockpit phase vtable')
    for va, target in ((0x01C08CFA,0x02B492D0),(0x01BA3F94,0x02B49A60),(0x01C756DE,0x02B49DA0)):
        check(va,b'\xE9'+struct.pack('<i',target-va-5),'cockpit phase thunk')
    for slot,target in zip((8,9,11),(0x02B68430,0x02B68590,0x02B686A0)):
        thunk=struct.unpack('<I',read(0x04DE86FC+4*slot,4))[0]
        check(thunk,b'\xE9'+struct.pack('<i',target-thunk-5),'minimap phase thunk')
        pointer(0x04DE86FC+4*slot,thunk,'minimap phase vtable')
    call(0x02B686E8,0x01C5F401,'minimap draw eligibility predicate')
    check(0x02B686F2,bytes.fromhex('751c'),'eligible branch enters draw logic')
    check(0x02B6870B,b'\xE9'+struct.pack('<i',0x02B687F0-0x02B68710),
          'ineligible branch skips normal loop to common epilogue')
    call(0x02B68713,0x01C322FD,'minimap view context getter before selected entry')
    call(0x02B49DD2,0x01C322FD,'cockpit view context getter before selected entry')
    check(0x02B49564,b'\xE9'+struct.pack('<i',0x02B4985C-0x02B49569),'special cockpit update bypasses normal loop')
    check(0x02B49C59,bytes.fromhex('eb7f'),'special cockpit late phase bypasses normal loop')
    for va,expected in ((0x02B49831,'8b4220ffd0'),(0x02B49CAF,'8b4224ffd0'),
                        (0x02B68523,'8b4220ffd0'),(0x02B6863E,'8b4224ffd0')):
        check(va,bytes.fromhex(expected),'widget phase indirect call without stack context')
    check(0x02B49E3C,bytes.fromhex('8b450850'),'cockpit draw pushes context')
    check(0x02B687C7,bytes.fromhex('8b450850'),'minimap draw pushes context')
    return dict(status='PASS_READ_ONLY_WITNESSES',source_sha256=SOURCE_SHA,
                checks=len(records),records=records,gameplay_executed=False,
                hooks_installed=False,render_fence_proven=False)

def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original',type=Path)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args(argv)
    try:
        if args.original.resolve()==args.report.resolve():raise ValueError('report aliases source')
        if args.report.exists() or args.report.is_symlink():raise ValueError('report already exists')
        report=audit(args.original.read_bytes())
        with args.report.open('x',encoding='utf8') as stream:json.dump(report,stream,indent=2);stream.write('\n')
        print(json.dumps({k:v for k,v in report.items() if k!='records'}))
        return 0
    except (OSError,ValueError,struct.error) as exc:
        print('ERROR: '+str(exc),file=sys.stderr);return 2
if __name__=='__main__':raise SystemExit(main())
