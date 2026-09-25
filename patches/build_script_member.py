#!/usr/bin/env python3
"""Add guarded, exact-Sub0 script member selection to one January candidate.

This is an incremental experimental patch, not a complete local-coop release.
It accepts only LOCAL ROUTING EXPERIMENTAL (0c019d43...). No game is executed.
Python 3.10+; standard library only. Input and existing outputs are never changed.
"""
from __future__ import annotations
import argparse
import array
import hashlib
import json
from pathlib import Path
import struct
import sys

BASE_SHA = '0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378'
BASE_SIZE = 60_755_456
CAVE = 0x01C95340
CALLBACKS = (0x02976C30, 0x029FF600)
PREFIX = bytes.fromhex('558bec81eccc000000')
HELPER = bytes.fromhex(
    '31c085c9743f833d88917d0500743681391cf4dc04752e8b1584917d05'
    '85d2742483ba400e000001751b39917810000075138b923c0e000085d2'
    '78093991741000000f94c0c20400')
HELPER_SHA = '5fa7f66b5524bd35bb297745bbd32854db8a43f48d17d52b12de2d2b54531433'
# Set after independent construction and before distribution.
OUTPUT_SHA = 'a7abf1545547aa41c5c918a4233ee7d8b87e0f503ce99b996d28409ef8070da2'

def digest(data: bytes | bytearray) -> str:
    return hashlib.sha256(data).hexdigest()

class Image:
    def __init__(self, data: bytes | bytearray):
        self.data = data
        if len(data) < 0x100 or data[:2] != b'MZ':
            raise ValueError('not a DOS/PE image')
        pe = struct.unpack_from('<I', data, 0x3c)[0]
        if pe+24 > len(data) or data[pe:pe+4] != b'PE\0\0':
            raise ValueError('invalid PE header')
        opt = pe+24
        machine, count = struct.unpack_from('<HH', data, pe+4)
        size = struct.unpack_from('<H', data, pe+20)[0]
        if machine != 0x14c or size < 96 or opt+size+40*count > len(data):
            raise ValueError('invalid i386 section table')
        if struct.unpack_from('<H', data, opt)[0] != 0x10b:
            raise ValueError('not PE32')
        self.checksum_offset = opt+64
        self.image_base = struct.unpack_from('<I', data, opt+28)[0]
        self.count = count
        self.sections = []
        for i in range(count):
            pos = opt+size+40*i
            virtual_size, rva, raw_size, raw = struct.unpack_from('<4I', data, pos+8)
            if raw+raw_size > len(data):
                raise ValueError('truncated section')
            self.sections.append((self.image_base+rva, raw_size, raw))
    def offset(self, va: int, count: int) -> int:
        if count <= 0:
            raise ValueError('non-positive range')
        for start, size, raw in self.sections:
            if start <= va and va+count <= start+size:
                return raw+va-start
        raise ValueError(f'address is not file-backed: {va:#x}')
    def read(self, va: int, count: int) -> bytes:
        pos = self.offset(va,count)
        return bytes(self.data[pos:pos+count])

def checksum(data: bytes | bytearray, offset: int) -> int:
    copy = bytearray(data)
    struct.pack_into('<I',copy,offset,0)
    if len(copy) % 2:
        copy.append(0)
    words = array.array('H')
    words.frombytes(copy)
    if sys.byteorder != 'little':
        words.byteswap()
    total = sum(words)
    while total >> 16:
        total = (total & 0xffff)+(total >> 16)
    return (total+len(data)) & 0xffffffff

def build(base: bytes) -> tuple[bytes,dict]:
    if len(base) != BASE_SIZE or digest(base) != BASE_SHA:
        raise ValueError('input is not the exact January LOCAL ROUTING candidate')
    if len(HELPER) != 72 or digest(HELPER) != HELPER_SHA:
        raise ValueError('corrupt embedded helper')
    image = Image(base)
    if image.image_base != 0x400000 or image.count != 10:
        raise ValueError('unexpected baseline image layout')
    if checksum(base,image.checksum_offset) != struct.unpack_from('<I',base,image.checksum_offset)[0]:
        raise ValueError('baseline PE checksum mismatch')
    data = bytearray(base)
    records = []
    def put(pos: int, old: bytes, new: bytes, label: str, va: int | None = None):
        if len(old) != len(new) or data[pos:pos+len(old)] != old:
            raise ValueError(f'expected-byte mismatch: {label}')
        for item in records:
            if pos < item['offset']+len(bytes.fromhex(item['old'])) and item['offset'] < pos+len(old):
                raise ValueError('overlapping modifications')
        data[pos:pos+len(new)] = new
        records.append(dict(label=label,va=va,offset=pos,old=old.hex(),new=new.hex()))
    put(image.offset(CAVE,72),b'\xcc'*72,HELPER,'guarded script member helper',CAVE)
    for va in CALLBACKS:
        jump = b'\xe9'+struct.pack('<i',CAVE-va-5)+b'\x90'*4
        put(image.offset(va,len(PREFIX)),PREFIX,jump,'FSM member entry',va)
    pos = image.checksum_offset
    put(pos,base[pos:pos+4],struct.pack('<I',checksum(data,pos)),'PE checksum')
    result = bytes(data)
    if OUTPUT_SHA and digest(result) != OUTPUT_SHA:
        raise ValueError('unexpected output SHA-256')
    restored = bytearray(result)
    for item in records:
        old = bytes.fromhex(item['old'])
        restored[item['offset']:item['offset']+len(old)] = old
    if restored != base or len(result) != BASE_SIZE:
        raise ValueError('reversal/size validation failed')
    if Image(result).read(0x02DEE8D0,46) != image.read(0x02DEE8D0,46):
        raise ValueError('uPcsInput was unexpectedly changed')
    report = {
        'status':'EXPERIMENTAL_SCRIPT_MEMBER_PATCH',
        'base_sha256':BASE_SHA,'output_sha256':digest(result),'output_size':len(result),
        'helper_va':hex(CAVE),'helper_size':72,'helper_sha256':HELPER_SHA,
        'changed_bytes':sum(a!=b for a,b in zip(base,result)),
        'patches':records,'exact_reversal':True,
        'scope':'Two FSM callbacks, only for exact cFsmActionPcsSub owned by the live local Sub0.',
        'unmodified_callback':'uPcsInput 0x02DEE8D0',
        'gameplay_executed':False,'runtime_validated':False,
        'limitations':['No game execution.','No claim that all scripted inputs or QTEs are fixed.',
                       'uPcsInput ownership remains separate; shared/global commands retain zero.',
                       'HUD, menus, full death/checkpoint/cutscene flow and partnerless scenes are not implemented by this patch.']}
    return result,report

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input',type=Path)
    parser.add_argument('output',type=Path)
    parser.add_argument('--report',type=Path,required=True)
    args = parser.parse_args()
    try:
        paths = [args.input,args.output,args.report]
        if len({p.resolve() for p in paths}) != len(paths):
            raise ValueError('input/output/report paths must be different')
        for path in paths[1:]:
            if path.exists() or path.is_symlink():
                raise ValueError(f'destination already exists: {path}')
            if not path.parent.is_dir():
                raise ValueError(f'destination parent does not exist: {path.parent}')
        result,report = build(args.input.read_bytes())
        report_bytes = (json.dumps(report,indent=2)+'\n').encode()
        with args.output.open('xb') as handle:
            handle.write(result)
        try:
            with args.report.open('xb') as handle:
                handle.write(report_bytes)
        except OSError:
            args.output.unlink()
            raise
        print(json.dumps({k:report[k] for k in ('status','output_sha256','output_size','changed_bytes','runtime_validated')}))
        return 0
    except (OSError,ValueError,struct.error) as exc:
        print(f'ERROR: {exc}',file=sys.stderr)
        return 2

if __name__ == '__main__':
    raise SystemExit(main())
