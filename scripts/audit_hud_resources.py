#!/usr/bin/env python3
"""Read-only January GUI resource evidence; no game build or engine execution."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ORIGINAL_SHA = '9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
ORIGINAL_SIZE = 60_748_800

class Image:
    def __init__(self, data: bytes):
        self.data = data
        if len(data) < 64 or data[:2] != b'MZ':
            raise ValueError('missing DOS header')
        pe = struct.unpack_from('<I', data, 0x3C)[0]
        if pe + 24 > len(data) or data[pe:pe+4] != b'PE\0\0':
            raise ValueError('missing PE header')
        machine, count = struct.unpack_from('<HH', data, pe+4)
        opt_size = struct.unpack_from('<H', data, pe+20)[0]
        opt = pe+24
        if machine != 0x14C or opt_size < 96 or opt+opt_size+40*count > len(data):
            raise ValueError('invalid PE32 layout')
        if struct.unpack_from('<H', data, opt)[0] != 0x10B:
            raise ValueError('requires PE32')
        self.base = struct.unpack_from('<I', data, opt+28)[0]
        self.sections = []
        for i in range(count):
            vs, rva, size, raw = struct.unpack_from('<IIII', data, opt+opt_size+i*40+8)
            if raw+size > len(data):
                raise ValueError('section outside file')
            self.sections.append((self.base+rva, size, raw))
    def read(self, va: int, size: int) -> bytes:
        if size <= 0:
            raise ValueError('invalid read length')
        for address, extent, raw in self.sections:
            if raw and address <= va and va-address+size <= extent:
                return self.data[raw+va-address:raw+va-address+size]
        raise ValueError(f'unbacked VA {va:#x}')

def audit(data: bytes) -> dict:
    digest = hashlib.sha256(data).hexdigest()
    if len(data) != ORIGINAL_SIZE or digest != ORIGINAL_SHA:
        raise ValueError('requires the exact unmodified January 2013 executable')
    pe = Image(data)
    records = []
    def exact(va, expected, label):
        if pe.read(va, len(expected)) != expected:
            raise ValueError(f'{label} at {va:#x}')
        records.append(dict(va=hex(va), expected=expected.hex(), label=label))
    def pointer(va, value, label):
        exact(va, struct.pack('<I', value), label)
    def branch(va, target, label, opcode=0xE9):
        exact(va, bytes([opcode])+struct.pack('<i', target-va-5), label)
    for vt, init, body, name, text in (
        (0x04DE52C4,0x01B94A7B,0x02B402F0,0x04DE53F4,b'id_bhr\\10_cockpit\\reticle\0'),
        (0x04DE4C3C,0x01B96D58,0x02B3B280,0x04CDDD14,b'id_bhr\\10_cockpit\\equip_wp_main\0'),
        (0x04DE810C,0x01C33D4C,0x02B61A70,0x04DE823C,b'id_bhr\\10_cockpit\\map_win\0')):
        pointer(vt+0x14,init,'initializer virtual')
        branch(init,body,'initializer thunk')
        pointer(vt+0x6C,0x01C32F50,'resource bind virtual')
        pointer(vt+0xB0,0x01C31E25,'resource cleanup virtual')
        exact(name,text,'literal GUI template')
    for va,target,label in (
        (0x01C32F50,0x035DEE10,'common resource bind'),
        (0x01C31E25,0x035E08A0,'common cleanup'),
        (0x01B87F1F,0x035E8B20,'node owner setter'),
        (0x01C81A33,0x035E88C0,'resource node count'),
        (0x01C33C93,0x02B0BCF0,'instance node getter'),
        (0x01C3B515,0x02CD29E0,'priority and layer packing'),
        (0x01C071CF,0x02B0BB00,'priority setter'),
        (0x01C4C4FF,0x02B0B960,'layer extraction'),
        (0x01C21BD3,0x02B0BBB0,'layer setter')):
        branch(va,target,label)
    for va,target,label in (
        (0x035DF049,0x01B87F1F,'root owns this GUI'),
        (0x035DF232,0x01B87F1F,'node owns this GUI'),
        (0x035DEEDB,0x01BD91A3,'retain resource'),
        (0x035E0ABA,0x01C3C159,'release stored resource'),
        (0x02B40374,0x01C3C159,'reticle releases temporary resource'),
        (0x02B3B304,0x01C3C159,'equipment releases temporary resource'),
        (0x02B61AF4,0x01C3C159,'map releases temporary resource'),
        (0x02B4019F,0x01C3B515,'reticle priority/layer configuration'),
        (0x02CD2A0F,0x01C071CF,'packed priority consumer'),
        (0x02CD2A18,0x01C4C4FF,'packed layer extraction'),
        (0x02CD2A24,0x01C21BD3,'packed layer consumer')):
        branch(va,target,label,0xE8)
    for va,code,label in (
        (0x035DEECC,'8991f0000000','per-instance resource pointer'),
        (0x035DEFD7,'8988f4000000','per-instance root pointer'),
        (0x035DF095,'8981f8000000','per-instance node table'),
        (0x035DF111,'8b4204ffd0','node factory virtual'),
        (0x035E88D1,'8b48688b4144','resource header and node count'),
        (0x02B0BD22,'8b91f80000008b45088b0c82','instance-local node indexing'),
        (0x035E8B34,'89486c','node owner field'),
        (0x035E0969,'c780f400000000000000','clear root'),
        (0x035E09A5,'c781f800000000000000','clear node table'),
        (0x035E0AC2,'c780f000000000000000','clear resource'),
        (0x02B4019A,'6a69','reticle packed priority constant'),
        (0x02CD2A06,'25ffff0f00','lower twenty bits'),
        (0x02B0B981,'c1e81483e07f','upper seven layer bits'),
        (0x02B0BB67,'89824c010000','priority at GUI+14C'),
        (0x02B0BC14,'898250010000','layer at GUI+150')):
        exact(va,bytes.fromhex(code),label)
    exact(0x04DE193C,b'Argument [prio] is max over.\0','priority diagnostic')
    exact(0x04DE1960,b'Argument [layer] is max over.\0','layer diagnostic')
    return dict(status='PASS_STATIC_EVIDENCE_ONLY',sha256=digest,checks=len(records),records=records,
        gameplay_executed=False,game_image_modified=False,
        findings=['Shared template, separate root/node ownership in the inspected initializers',
                  'Reticle 0x69 configures priority/layer, not a player identifier'],
        limitations=['Does not certify live resource graphs or all global GUI registrations',
                     'Does not install the detached lifecycle backend'])

def main(argv=None):
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('original',type=Path)
    p.add_argument('--report',type=Path)
    a=p.parse_args(argv)
    try:
        if a.report and a.original.resolve()==a.report.resolve():
            raise ValueError('report cannot replace the original')
        if a.report and (a.report.exists() or a.report.is_symlink()):
            raise ValueError('report exists; overwrite refused')
        result=audit(a.original.read_bytes())
        text=json.dumps(result,indent=2)+'\n'
        if a.report:
            with a.report.open('x',encoding='utf8') as stream:stream.write(text)
        print(json.dumps({k:v for k,v in result.items() if k!='records'}))
        return 0
    except (OSError,ValueError,struct.error) as exc:
        print('ERROR: '+str(exc),file=sys.stderr)
        return 2
if __name__=='__main__':raise SystemExit(main())
