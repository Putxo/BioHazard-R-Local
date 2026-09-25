#!/usr/bin/env python3
"""Read-only January CPU pipeline witnesses. Does not patch or execute the game."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import sys

ORIGINAL_SHA = '9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
ORIGINAL_SIZE = 60_748_800
BODY_START, BODY_SIZE = 0x02F50680, 2271
BODY_SHA = 'a2c8f20788ce3a48cb01f468bb92241fca65eb82e723b26131266e670f88d894'
SITES = ((0x02F506A0, '894df88b4df8', 0x02F506A6),
         (0x02F50F4B, '5f5e5b81c4d0000000', 0x02F50F54))

class Image:
    def __init__(self, data: bytes):
        if len(data) != ORIGINAL_SIZE or hashlib.sha256(data).hexdigest() != ORIGINAL_SHA:
            raise ValueError('requires exact unmodified January 2013 executable')
        self.data = data
        pe = struct.unpack_from('<I', data, 0x3C)[0]
        if data[:2] != b'MZ' or data[pe:pe+4] != b'PE\0\0':
            raise ValueError('invalid DOS/PE signature')
        machine, count = struct.unpack_from('<HH', data, pe+4)
        optional_size = struct.unpack_from('<H', data, pe+20)[0]
        opt = pe+24
        if machine != 0x14C or struct.unpack_from('<H', data, opt)[0] != 0x10B:
            raise ValueError('requires PE32 i386')
        base = struct.unpack_from('<I', data, opt+28)[0]
        if base != 0x400000 or not 1 <= count <= 32:
            raise ValueError('unexpected image base/section count')
        self.sections = []
        for i in range(count):
            pos = opt+optional_size+i*40
            _, rva, raw_size, raw = struct.unpack_from('<4I', data, pos+8)
            if raw > len(data) or raw_size > len(data)-raw:
                raise ValueError('invalid raw section range')
            self.sections.append((base+rva, raw_size, raw))
    def read(self, va: int, size: int) -> bytes:
        if size < 0 or va < 0 or va+size > 0x100000000:
            raise ValueError('invalid VA range')
        for start, raw_size, raw in self.sections:
            delta = va-start
            if 0 <= delta <= raw_size and size <= raw_size-delta:
                return self.data[raw+delta:raw+delta+size]
        raise ValueError(f'VA not backed by file: {va:#x}')

def audit(data: bytes) -> dict:
    image = Image(data)
    records = []
    def exact(va, expected, label):
        if image.read(va,len(expected)) != expected:
            raise ValueError(label)
        records.append(dict(va=hex(va), size=len(expected), expected=expected.hex(), label=label))
    def ptr(va, target, label): exact(va,struct.pack('<I',target),label)
    def rel(va,target,opcode,label): exact(va,bytes([opcode])+struct.pack('<i',target-va-5),label)
    for va, expected, continuation in SITES:
        exact(va,bytes.fromhex(expected),'pipeline gateway displaced instructions')
        if va+len(bytes.fromhex(expected)) != continuation:
            raise ValueError('invalid continuation')
    ptr(0x04E38B38,0x053965A8,'sSkeletonMain vtable COL')
    ptr(0x053965B4,0x054EEE9C,'sSkeletonMain type descriptor')
    exact(0x054EEEA4,b'.?AVsSkeletonMain@@\0','sSkeletonMain RTTI name')
    ptr(0x04E38B54,0x01B826B4,'sSkeletonMain virtual6')
    rel(0x01B826B4,0x02F50680,0xE9,'outer pipeline thunk')
    rel(0x02F507F6,0x01BE37D9,0xE8,'renderer begin inside outer cycle')
    rel(0x01BE37D9,0x033BFD80,0xE9,'renderer begin thunk')
    exact(0x02F50D10,bytes.fromhex('8b8870a40200'),'renderer owner member')
    exact(0x02F50D25,bytes.fromhex('8b4218ffd0'),'renderer virtual6 call')
    ptr(0x04F042D8+6*4,0x01C15E73,'renderer virtual6 table')
    rel(0x01C15E73,0x033C7330,0xE9,'renderer virtual6 thunk')
    rel(0x02F50E7E,0x01BEBCC7,0xE8,'renderer end inside outer cycle')
    rel(0x01BEBCC7,0x033BFFC0,0xE9,'renderer end thunk')
    exact(0x02F50F5B,bytes.fromhex('8be55dc3'),'single normal epilogue return')
    for va,raw in ((0x02F50914,'742c'),(0x02F50A05,'7425'),
                   (0x02F50A19,'7511'),(0x02F50AB5,'742c')):
        exact(va,bytes.fromhex(raw),'recorded internal forward conditional branch')
    exact(0x033BFDAE,b'\x68'+struct.pack('<I',0x04F013D4),'renderer begin string reference')
    exact(0x04F013D4,b'sRender::begin\0','renderer begin literal')
    exact(0x033BFDF1,bytes.fromhex('0fb6512085d27415'),'renderer begin early active gate')
    rel(0x033BFE09,0x033BFF5D,0xE9,'renderer begin bypasses increment on active path')
    exact(0x033BFE47,bytes.fromhex('8b4dfc8b91e03d030083c2018b45fc8990e03d0300'),'renderer counter increment')
    exact(0x034FD481,bytes.fromhex('8b80e03d0300'),'renderer counter getter')
    exact(0x033C00A7,bytes.fromhex('3d02010000750d'),'renderer wait timeout gate')
    rel(0x033C00B6,0x033C02BD,0xE9,'renderer end timeout bypass')
    exact(0x033C00F0,bytes.fromhex('8b4844ffd1'),'device virtual call')
    exact(0x033C00FF,bytes.fromhex('817de468087688'),'device-loss result check')
    digest = hashlib.sha256(image.read(BODY_START,BODY_SIZE)).hexdigest()
    if digest != BODY_SHA: raise ValueError('outer pipeline body identity mismatch')
    return dict(status='PASS_STATIC_EVIDENCE_ONLY',original_sha256=ORIGINAL_SHA,
        checks=len(records)+1,records=records,body=dict(va=hex(BODY_START),size=BODY_SIZE,sha256=digest),
        clock_semantics='one CPU update/draw pipeline invocation, not a successful Present count',
        gameplay_executed=False,game_image_modified=False,render_fence_proved=False,
        limits=['Selected direct branches and exact body identity, not exception analysis',
                'Asynchronous callback correlation and native render scopes remain unvalidated'])

def audit_sources(root: Path) -> dict:
    folder = root/'patches/hud_ownership'
    header=(folder/'pipeline_clock.hpp').read_text(encoding='utf8')
    asm=(folder/'pipeline_gateways.S').read_text(encoding='utf8')
    linker=(folder/'pipeline_continuations.ld').read_text(encoding='utf8')
    for name,(va,_,continuation) in zip(('begin','end'),SITES):
        title='Begin' if name=='begin' else 'End'
        patterns=((header,rf'Pipeline{title}\s*=\s*0x{va:08X}\s*;'),
                  (asm,rf'^pipeline_gate\s+{name},\s*0x{va:08X},\s*{int(name=="end")}\s*$'),
                  (linker,rf'rev_pipeline_continue_{name}\s*=\s*0x{continuation:08X}\s*;'))
        if any(not re.search(pattern,content,re.M|re.I) for content,pattern in patterns):
            raise ValueError('source/assembly/continuation map mismatch: '+name)
    if 'push dword ptr [esi+24]' not in asm or 'push ebp' not in asm:
        raise ValueError('BEGIN must use original ECX and caller frame cookie')
    return dict(status='PASS_SOURCE_MAP_ONLY',sites=2,engine_executed=False)

def main(argv=None) -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original',type=Path)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--source-root',type=Path)
    args=parser.parse_args(argv)
    created=False
    try:
        if args.original.resolve()==args.report.resolve() or os.path.lexists(args.report):
            raise ValueError('output exists or aliases original; overwrite refused')
        data=args.original.read_bytes()
        report=audit(data)
        if args.source_root: report['source_map']=audit_sources(args.source_root)
        with args.report.open('x',encoding='utf8') as output:
            created=True;json.dump(report,output,indent=2);output.write('\n')
        print(json.dumps({k:v for k,v in report.items() if k!='records'}))
        return 0
    except (OSError,ValueError,struct.error) as error:
        if created: args.report.unlink(missing_ok=True)
        print('ERROR: '+str(error),file=sys.stderr)
        return 2
if __name__=='__main__': raise SystemExit(main())
