#!/usr/bin/env python3
"""Experimental January script-member correction. Does not execute the game.

Input: exact LOCAL ROUTING EXPERIMENTAL image. The inherited routing module,
all existing patches, uPcsInput and engine globals are preserved byte-for-byte.
Output/report are created exclusively; existing files are never overwritten.
"""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import struct
import sys
from build_local_routing import PE, sha, pe_checksum

BASE_SHA = '0c019d43b92c0092fa458abcf7e2990f8783895eb2bd8181f6e9f130b697d378'
BASE_SIZE = 60_755_456
HELPER_VA = 0x01C95340
ENTRY_VAS = (0x02976C30, 0x029FF600)
HELPER = bytes.fromhex(
    '31c085c9743f833d88917d0500743681391cf4dc04752e8b1584917d05'
    '85d27424399178100000751c83ba400e00000175138b923c0e000085d2'
    '78093991741000000f94c0c20400'
)
HELPER_SHA = '500a82d8071f5c40ece76397fee1bac97888768a3e9029d52222d4d933eb7896'
STOCK_BODY = bytes.fromhex(
    '558bec81eccc000000535657518dbd34ffffffb933000000'
    'b8ccccccccf3ab59894df833c05f5e5b8be55dc20400'
)

def build(base: bytes) -> tuple[bytes, dict]:
    if len(base) != BASE_SIZE or sha(base) != BASE_SHA:
        raise ValueError('requires exact LOCAL ROUTING EXPERIMENTAL input')
    pe = PE(base)
    if (pe.base, pe.count) != (0x400000, 10):
        raise ValueError('unexpected PE32 layout')
    if pe.u16(pe.opt+70) & 0x40 or not pe.u16(pe.pe+22) & 1:
        raise ValueError('requires fixed-address relocation-stripped image')
    if pe.u32(pe.opt+96+4*8) or pe.u32(pe.opt+96+5*8):
        raise ValueError('unexpected certificate/relocation directory')
    if len(HELPER) != 72 or sha(HELPER) != HELPER_SHA:
        raise ValueError('embedded helper identity changed')
    if pe.read(HELPER_VA, len(HELPER)) != b'\xCC'*len(HELPER):
        raise ValueError('helper cave occupied')
    for va in ENTRY_VAS:
        if pe.read(va, len(STOCK_BODY)) != STOCK_BODY:
            raise ValueError('callback differs from audited stock body')
    data = bytearray(base)
    changes = []
    def patch(offset: int, old: bytes, new: bytes, label: str, va=None):
        if len(old) != len(new) or data[offset:offset+len(old)] != old:
            raise ValueError('unexpected bytes: '+label)
        if any(offset < c['offset']+c['size'] and c['offset'] < offset+len(old) for c in changes):
            raise ValueError('overlapping changes')
        data[offset:offset+len(new)] = new
        changes.append(dict(offset=offset,va=va,size=len(old),label=label,
                            old=old.hex(),new=new.hex(),changed=sum(a!=b for a,b in zip(old,new))))
    patch(pe.offset(HELPER_VA, len(HELPER)), b'\xCC'*len(HELPER), HELPER,
          'exact PcsSub owner, Pad actor and matching nonnegative serial', HELPER_VA)
    for va in ENTRY_VAS:
        jump = b'\xE9'+struct.pack('<i', HELPER_VA-va-5)+b'\x90'*4
        patch(pe.offset(va, 9), STOCK_BODY[:9], jump, 'FSM member callback', va)
    off = pe.opt+64
    patch(off, base[off:off+4], struct.pack('<I', pe_checksum(data,off)), 'PE checksum')
    output = bytes(data)
    if len(output) != len(base) or PE(output).sections != pe.sections:
        raise ValueError('layout changed')
    restored = bytearray(output)
    for c in changes:
        restored[c['offset']:c['offset']+c['size']] = bytes.fromhex(c['old'])
    if bytes(restored) != base:
        raise ValueError('exact reversibility failed')
    report = dict(status='EXPERIMENTAL_COMPONENT_VALIDATION_ONLY',base_sha256=BASE_SHA,
        output_sha256=sha(output),file_size=len(output),helper_va=HELPER_VA,
        helper_sha256=HELPER_SHA,helper_size=len(HELPER),changes=changes,
        changed_bytes=sum(c['changed'] for c in changes),layout_preserved=True,
        exact_reversal_to_base=True,gameplay_executed=False,
        rule='active exact cFsmActionPcsSub; +1078 equals tracked Pad Sub0; +1074 equals its nonnegative +E3C serial',
        unchanged=['uPcsInput callback','inherited .lcfix/.lcdata','global Self/GameMode/network serial','existing pickup/door/camera hooks'],
        limits=['No game execution','Not full QTE coverage','HUD/menu/checkpoint/cutscene and absent-partner cases remain open'])
    return output, report

def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input',type=Path)
    parser.add_argument('output',type=Path)
    parser.add_argument('--report',required=True,type=Path)
    args = parser.parse_args(argv)
    created = []
    try:
        paths = [args.input,args.output,args.report]
        resolved = [p.resolve() for p in paths]
        if len(set(resolved)) != 3:
            raise ValueError('input, output and report must be different')
        if any(os.path.lexists(p) for p in paths[1:]):
            raise ValueError('output or report exists; overwrite refused')
        if not resolved[0].is_file() or any(not p.parent.is_dir() for p in resolved[1:]):
            raise ValueError('input and destination directories must exist')
        output, report = build(resolved[0].read_bytes())
        report.update(input_name=args.input.name, output_name=args.output.name)
        payloads = ((resolved[1],output),(resolved[2],(json.dumps(report,indent=2)+'\n').encode()))
        for path,payload in payloads:
            with path.open('xb') as stream:
                created.append(path)
                stream.write(payload)
                stream.flush()
                os.fsync(stream.fileno())
        print(json.dumps({k:report[k] for k in ('status','output_sha256','file_size','changed_bytes')}))
        return 0
    except (OSError,ValueError,struct.error) as exc:
        for path in reversed(created):
            try:path.unlink()
            except OSError:pass
        print('ERROR: '+str(exc),file=sys.stderr)
        return 2

if __name__=='__main__':
    raise SystemExit(main())
