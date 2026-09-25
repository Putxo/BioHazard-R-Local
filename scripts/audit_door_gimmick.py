#!/usr/bin/env python3
"""Read-only, hash-pinned structural audit. Not a gameplay or emulator test."""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

HASHES = {
    'original': '9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69',
    'v13': '3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa',
}

class AuditError(ValueError):
    pass

class Image:
    def __init__(self, data: bytes):
        self.data = data
        if data[:2] != b'MZ' or len(data) < 64:
            raise AuditError('Invalid DOS header')
        pe = self.u32(0x3c)
        if data[pe:pe + 4] != b'PE\0\0':
            raise AuditError('Invalid PE signature')
        opt = pe + 24
        if self.u16(pe + 4) != 0x14c or self.u16(opt) != 0x10b:
            raise AuditError('Expected PE32 i386')
        self.base = self.u32(opt + 28)
        self.sections = []
        for i in range(self.u16(pe + 6)):
            pos = opt + self.u16(pe + 20) + 40 * i
            _, rva, size, raw = struct.unpack('<4I', self.slice(pos + 8, 16))
            if raw + size > len(data):
                raise AuditError('Truncated section')
            self.sections.append((self.base + rva, size, raw))

    def slice(self, pos: int, size: int) -> bytes:
        if pos < 0 or size < 0 or pos + size > len(self.data):
            raise AuditError('Out-of-file read')
        return self.data[pos:pos + size]

    def u16(self, pos: int) -> int:
        return struct.unpack('<H', self.slice(pos, 2))[0]

    def u32(self, pos: int) -> int:
        return struct.unpack('<I', self.slice(pos, 4))[0]

    def read(self, va: int, size: int) -> bytes:
        for start, count, raw in self.sections:
            if start <= va and va + size <= start + count:
                return self.slice(raw + va - start, size)
        raise AuditError(f'VA not backed by file bytes: {va:#x}')

    def ptr(self, va: int) -> int:
        return struct.unpack('<I', self.read(va, 4))[0]

    def relative(self, va: int, opcode: int) -> int:
        data = self.read(va, 5)
        if data[0] != opcode:
            raise AuditError(f'Unexpected relative instruction at {va:#x}')
        return va + 5 + struct.unpack('<i', data[1:])[0]


def inspect(data: bytes, label: str) -> dict:
    digest = hashlib.sha256(data).hexdigest()
    if digest != HASHES[label]:
        raise AuditError(f'{label}: SHA-256 mismatch: {digest}')
    image = Image(data)
    checks = []

    def check(name, actual, expected):
        if actual != expected:
            raise AuditError(f'{name}: expected {expected!r}, got {actual!r}')
        checks.append(name)

    def raw(name, va, expected):
        check(name, image.read(va, len(bytes.fromhex(expected))).hex(), expected)

    check('size', len(data), 60748800)
    check('ImageBase', image.base, 0x400000)
    for vt, col, td, name, slot, thunk, target in [
        (0x04d5d61c, 0x0533662c, 0x0548551c, 'WaitState', 8, 0x01b7e9ec, 0x02590040),
        (0x04d5d684, 0x05336704, 0x054855c0, 'MoveState', 8, 0x01c4df17, 0x02590f60),
        (0x04d5d754, 0x05336850, 0x054856d0, 'PlayerDoorGimmickWaitState', 8, 0x01c74013, 0x02711d70),
    ]:
        check(name + ': COL', image.ptr(vt - 4), col)
        check(name + ': type descriptor', image.ptr(col + 12), td)
        expected = ('.?AV' + name + '@door_gimmick@obj_model@chara@game@app@@\0').encode()
        check(name + ': RTTI', image.read(td + 8, len(expected)), expected)
        check(name + ': virtual slot', image.ptr(vt + slot * 4), thunk)
        check(name + ': implementation', image.relative(thunk, 0xe9), target)

    for site, target in [
        (0x02590072, 0x01c16468), (0x025900b4, 0x01c85962),
        (0x0259040c, 0x01c8c9a1), (0x0259041b, 0x01c16468),
        (0x02590dd3, 0x01c07341), (0x02590de6, 0x01c4c9c3),
        (0x02590fa0, 0x01c07341), (0x0259681d, 0x01c07341),
        (0x02596c02, 0x01c07341), (0x02596f3f, 0x01c07341),
        (0x02591121, 0x01b94f53), (0x02da3898, 0x01c8c9a1),
    ]:
        check(f'call {site:#x}', image.relative(site, 0xe8), target)
    for thunk, target in [
        (0x01c07341, 0x02da3840), (0x01c8c9a1, 0x01d77eb0),
        (0x01c1c8b8, 0x01d4b120), (0x01b94f53, 0x02db0cf0),
    ]:
        check(f'thunk {thunk:#x}', image.relative(thunk, 0xe9), target)
    for name, va, expected in [
        ('local serial stored', 0x025900b9, '8845d78b45f88a4dd7884810'),
        ('signed entry argument', 0x0259022f, '6a008b45f80fbe4810516a01'),
        ('candidate must equal Self', 0x02590420, '3945b07504c645cb01'),
        ('MoveState arg2 stored', 0x02590d23, '8b45f88b4d0c894810'),
        ('lookup-null early exit', 0x02590fa5, '8945e0837de0007505e923050000'),
        ('pad zero hardcoded', 0x02591118, '6a00'),
        ('mUsePlayer default sentinel', 0x0258f897, 'c64004ff'),
        ('isPlayer constant', 0x01d77edc, '3d00000180'),
        ('category mask', 0x01d4b141, '2500000ff0'),
        ('serial getter', 0x01cb7636, '8b803c0e0000'),
        ('serial comparison', 0x02da38bf, '3b4508'),
        ('finder return ABI', 0x02da38eb, 'c20400'),
    ]:
        raw(name, va, expected)
    expected = '8b5508909090' if label == 'v13' else '8b9170090000'
    raw('general door getter selector load', 0x02db0d25, expected)
    return {'build': label, 'sha256': digest, 'assertions': len(checks),
            'status': 'PASS_STATIC_ASSERTIONS', 'checks': checks}


def negative_controls(original: bytes) -> list[str]:
    passed = []
    tests = [
        ('corrupted source rejected', lambda: inspect(original[:-1] + bytes([original[-1] ^ 1]), 'original')),
        ('wrong build label rejected', lambda: inspect(original, 'v13')),
        ('invalid header rejected', lambda: Image(b'not a PE')),
        ('truncated PE rejected', lambda: Image(original[:128])),
        ('unbacked VA rejected', lambda: Image(original).read(0x00401000, 4)),
        ('negative offset rejected', lambda: Image(original).slice(-1, 4)),
    ]
    for name, test in tests:
        try:
            test()
        except AuditError:
            passed.append(name)
        else:
            raise AuditError('Negative control unexpectedly succeeded: ' + name)
    return passed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original', type=Path)
    parser.add_argument('--v13', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    try:
        original = args.original.read_bytes()
        reports = [inspect(original, 'original')]
        if args.v13:
            reports.append(inspect(args.v13.read_bytes(), 'v13'))
        report = {'status': 'PASS_STATIC_AUDIT', 'gameplay_executed': False,
                  'patch_generated': False, 'builds': reports,
                  'negative_controls': negative_controls(original)}
        text = json.dumps(report, indent=2) + '\n'
        if args.report:
            inputs = [args.original.resolve()]
            if args.v13:
                inputs.append(args.v13.resolve())
            if args.report.resolve() in inputs:
                raise AuditError('Report cannot overwrite an input executable')
            args.report.write_text(text, encoding='utf-8')
        print(text, end='')
        return 0
    except (OSError, ValueError, struct.error) as exc:
        print(json.dumps({'status': 'FAIL', 'reason': str(exc)}))
        return 2

if __name__ == '__main__':
    raise SystemExit(main())
