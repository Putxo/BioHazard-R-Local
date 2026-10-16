#!/usr/bin/env python3
"""Run without game data; synthetic x86 PE32 fixtures only."""
import contextlib
import io
import json
from pathlib import Path
import sys
import tempfile
import struct

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import identify_revelations_build as identify

def fixture() -> bytes:
    data = bytearray(2048)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 0x3C, 0x80)
    data[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HHI', data, 0x84, 0x14C, 1, 0x12345678)
    struct.pack_into('<H', data, 0x94, 0xE0)
    o = 0x98
    struct.pack_into('<H', data, o, 0x10B)
    struct.pack_into('<I', data, o + 16, 0x1100)
    struct.pack_into('<I', data, o + 28, 0x400000)
    struct.pack_into('<I', data, o + 56, 0x4000)
    struct.pack_into('<H', data, o + 70, 0x8100)
    struct.pack_into('<8sIIII', data, o + 0xE0, b'.text\0\0\0', 512, 0x1000, 512, 512)
    return bytes(data)

def tests() -> int:
    checks = 0
    def check(yes, msg):
        nonlocal checks
        checks += 1
        if not yes:
            raise AssertionError(msg)
    assert len(identify.BUILDS) == 4
    good = fixture()
    fact = identify.inspect_pe(good[:65536], len(good))
    check(fact['entry_va'] == 0x401100, 'entry VA')
    check(fact['section_names'] == ['.text'], 'fixture sections')
    check(fact['format'] == 'PE32' and fact['machine'] == 'x86', 'format')
    check(identify.match_build('f'*64, len(good), fact)['mod_status'] == 'UNKNOWN_BUILD', 'unknown hash')
    for digest, build in identify.BUILDS.items():
        mock = dict(fact, image_base=0x400000, entry_rva=build['entry_rva'],
                    section_names=list(build['section_names']))
        expected = identify.match_build(digest, build['size_bytes'], mock)
        check(expected['build'] == build['build'], 'exact '+build['build'])
        check(expected['mod_status'] == build['mod_status'], 'status '+build['build'])
        check(identify.match_build(digest, build['size_bytes'] - 1, mock)['mod_status'] ==
              'INCONSISTENT_BUILD', 'different size')
        check(identify.match_build(digest, build['size_bytes'], dict(mock, entry_rva=0x1000))[
            'mod_status'] == 'INCONSISTENT_BUILD', 'different entry')
        check(identify.match_build(digest, build['size_bytes'], dict(mock, section_names=['.text']))[
            'mod_status'] == 'INCONSISTENT_BUILD', 'different sections')
    failures = [
        ('bad MZ', b'NOTDOS'),
        ('bad PE signature', good[:0x80] + b'ABCD' + good[0x84:]),
        ('bad section table', good[:0x86] + b'\xFF\xFF' + good[0x88:]),
        ('bad PE32 magic', good[:0x98] + b'\x0b\x02' + good[0x9a:]),
        ('outside raw data', good[:0x188] + b'\xFF\xFF\xFF\xFF' + good[0x18c:]),
    ]
    for name, bad in failures:
        try:
            identify.inspect_pe(bad[:65536], len(bad))
        except (ValueError, struct.error):
            check(True, name)
        else:
            check(False, 'accepted '+name)
    with tempfile.TemporaryDirectory() as d:
        root = Path(d)
        p = root/'fake.exe'
        copy = root/'duplicate.exe'
        p.write_bytes(good)
        copy.write_bytes(good)
        records = identify.inspect_paths([p, copy])
        check(records[0]['mod_status'] == 'UNKNOWN_BUILD', 'unknown file')
        check(records[1]['identical_to'] == 'fake.exe', 'duplicate file')
        report = root/'result.json'
        with contextlib.redirect_stdout(io.StringIO()):
            check(identify.main([str(p), str(copy), '--report', str(report)]) == 0,
                  'informational CLI accepts unknown PE files')
        check(json.loads(report.read_text())['game_executed'] is False, 'metadata not execution')
        with contextlib.redirect_stdout(io.StringIO()):
            check(identify.main([str(p), '--require-mod-target']) == 3,
                  'guard rejects unknown file')
        with contextlib.redirect_stdout(io.StringIO()):
            check(identify.main([str(root/'missing.exe')]) == 2,
                  'invalid input rejects')
        try:
            with contextlib.redirect_stderr(io.StringIO()):
                identify.main([str(p), '--report', str(report)])
        except SystemExit as e:
            check(e.code == 2, 'never overwrite report')
        else:
            check(False, 'report overwritten')
    return checks

if __name__ == '__main__':
    print(json.dumps({'status': 'PASS', 'assertions': tests(), 'game_data_used': False}))
