#!/usr/bin/env python3
"""Inspect the January Sub0 binder on disk; never launch or attach to the game.

Recognizes the complete published local-routing binder, including its native
setter and continuations. This proves patch presence, not controller/gameplay
operation. Unknown builds are rejected instead of guessed from a byte search.
"""
from pathlib import Path
import argparse
import json
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'patches'))
from build_local_routing import PE, rel, sha

BIND_SITE = 0x02DF5015
BIND_SIZE = 108


def target(data, at, opcode):
    if len(data) != 5 or data[0] != opcode:
        raise ValueError('unexpected branch encoding')
    return at + 5 + struct.unpack_from('<i', data, 1)[0]


def inspect_binder(pe):
    site = pe.read(BIND_SITE, 13)
    if site[5:] != b'\x90' * 8:
        raise ValueError('no recognized local-routing binder at January bind site')
    at = target(site[:5], BIND_SITE, 0xE9)
    sections = [s for s in pe.sections if s['name'] == '.lcfix']
    if len(sections) != 1:
        raise ValueError('missing or ambiguous .lcfix section')
    section = sections[0]
    def in_code(address, size):
        return (section['characteristics'] & 0xE0000020 == 0x60000020 and
                section['va'] <= address and address + size <=
                section['va'] + min(section['virtual_size'], section['raw_size']))
    if not in_code(at, BIND_SIZE):
        raise ValueError('binder is outside read/execute .lcfix code')
    code = pe.read(at, BIND_SIZE)
    forget = target(code[73:78], at + 73, 0xE8)
    if not in_code(forget, 1):
        raise ValueError('local reset callback outside .lcfix')
    expected = (bytes.fromhex('89c1') + rel(at+2, 0x01BEE332) +
        bytes.fromhex('8b4df889414481399c64e1047552a384917d0585c07449'
                      '83b8400e000003742183b8400e0000017537'
                      '83bde8feffff00742e3b85e4feffff75263b45e07521eb1050') +
        rel(at+73, forget) + bytes.fromhex('586a0189c1') +
        rel(at+83, 0x01BB8B60) + bytes.fromhex('c70588917d0501000000') +
        rel(at+98, 0x01C9504C) + rel(at+103, 0x02DF5022, 0xE9))
    suspended = bytearray(expected); suspended[46] = 0x77  # JA rather than JNE after mode<=1
    legacy = bytearray(expected); legacy[36] = 2
    if code not in (expected, bytes(suspended), bytes(legacy)):
        raise ValueError('unrecognized binder; no input claim can be made')
    corrected = code != bytes(legacy)
    return {'status': 'CPU3_BINDER_PRESENT' if corrected else 'LEGACY_CPU2_BUG',
            'binder_va': hex(at), 'cpu_comparison': code[36],
            'native_setter': '0x01bb8b60', 'network2_preserved': corrected,
            'suspended_rebind_preserved': code == bytes(suspended),
            'gameplay_validated': False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    args = parser.parse_args()
    try:
        data = args.image.read_bytes()
        report = inspect_binder(PE(data))
        from pad_axis_sites import inspect_axes
        report.update(sha256=sha(data), game_executed=False, axes=inspect_axes(PE(data)))
        print(json.dumps(report))
        return 0 if report['status'] == 'CPU3_BINDER_PRESENT' else 1
    except (OSError, ValueError, struct.error) as exc:
        print(json.dumps({'status': 'UNKNOWN_IMAGE', 'detail': str(exc),
                          'game_executed': False}))
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
