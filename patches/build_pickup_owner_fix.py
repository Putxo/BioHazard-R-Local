#!/usr/bin/env python3
"""Build an experimental owner-corrected pickup patch. No game execution.

Accepts only the documented January v13 or v14 images. v13 is first upgraded
in memory to the exact published v14. Neither input nor existing outputs
are overwritten. The resulting candidate is not gameplay-validated.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

V13 = '3df0e1020b58b3ccc7e31a9046a2a3ce9e5230e1764869b517943b4a808838aa'
V14 = '73fe1255697c47a624025ba40b33bab8df5812e76021bdc2d9acdeec3daf56ea'
CANDIDATE = 'e3c5c188782309a1683d27ade0ce9e38cf1c40219de9fb5d684b3d6fda2e285a'
STOCK = bytes.fromhex('558bec81eccc000000535657518dbd34ffffffb933000000b8ccccccccf3ab59894df833c05f5e5b8be55dc20400')
MISPLACED = bytes.fromhex('833d88917d050074168b81480f00003b0584917d057508b801000000c2040033c0c20400cccccccccccccccccccc')
SELECTOR = bytes.fromhex('31c085c9741c833d88917d050074138b1584917d0585d274093991480f00000f94c0c20400')
REPLACEMENT = SELECTOR + b'\xcc' * (len(STOCK) - len(SELECTOR))
V14_HELPER = bytes.fromhex('833d88917d0500740a8b4de0e83574fdffeb0231c050e8d964ffff89c1e831fcefffc3')
OLD_DOOR = bytes.fromhex('6a00e8d5a66fff8bc8e82d3e60ff')
NEW_DOOR = bytes.fromhex('e8e34170ff') + b'\x90' * 9

class PatchError(ValueError):
    pass

def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

class Image:
    def __init__(self, data: bytes):
        self.data = data
        if len(data) < 64 or data[:2] != b'MZ':
            raise PatchError('Invalid DOS header')
        pe = struct.unpack_from('<I', data, 0x3c)[0]
        if pe + 24 > len(data) or data[pe:pe + 4] != b'PE\0\0':
            raise PatchError('Invalid PE header')
        opt = pe + 24
        machine, count = struct.unpack_from('<HH', data, pe + 4)
        optsize = struct.unpack_from('<H', data, pe + 20)[0]
        if optsize < 96 or opt + optsize + 40 * count > len(data):
            raise PatchError('Truncated PE headers')
        if machine != 0x14c or struct.unpack_from('<H', data, opt)[0] != 0x10b:
            raise PatchError('Expected PE32 i386')
        base = struct.unpack_from('<I', data, opt + 28)[0]
        if base != 0x400000:
            raise PatchError('Unexpected ImageBase')
        self.sections = []
        for i in range(count):
            p = opt + optsize + 40 * i
            _, rva, size, raw = struct.unpack_from('<4I', data, p + 8)
            if raw + size > len(data):
                raise PatchError('Truncated section')
            self.sections.append((base + rva, size, raw))

    def offset(self, va: int, size: int) -> int:
        if size <= 0:
            raise PatchError('Invalid read size')
        for start, count, raw in self.sections:
            if start <= va and va + size <= start + count:
                return raw + va - start
        raise PatchError(f'VA not file-backed: {va:#x}')

    def read(self, va: int, size: int) -> bytes:
        pos = self.offset(va, size)
        return self.data[pos:pos + size]

def replace(data: bytearray, va: int, old: bytes, new: bytes) -> dict:
    if len(old) != len(new):
        raise PatchError('Length-changing patch refused')
    pos = Image(data).offset(va, len(old))
    if data[pos:pos + len(old)] != old:
        raise PatchError(f'Expected bytes mismatch at {va:#x}')
    data[pos:pos + len(old)] = new
    return {'va': f'{va:#010x}', 'file_offset': hex(pos),
            'old': old.hex(), 'new': new.hex(),
            'span_bytes': len(new),
            'different_bytes': sum(a != b for a, b in zip(old, new))}

def build(source: bytes) -> tuple[bytes, dict]:
    source_hash = digest(source)
    if source_hash not in (V13, V14):
        raise PatchError('Only exact January v13/v14 input is accepted: ' + source_hash)
    if len(source) != 60748800:
        raise PatchError('Unexpected input length')
    data = bytearray(source)
    upgrade = []
    if source_hash == V13:
        if digest(V14_HELPER) != '0c83033764b4760949441b145486edf44807acfbf6af352f1f31198bc2301ac3':
            raise PatchError('Published v14 helper mismatch')
        upgrade.append(replace(data, 0x01c95300, b'\xcc' * 35, V14_HELPER))
        upgrade.append(replace(data, 0x02591118, OLD_DOOR, NEW_DOOR))
    if digest(data) != V14:
        raise PatchError('Cannot reproduce the exact published v14')
    base = bytes(data)
    patches = [
        replace(data, 0x024605d0, STOCK, REPLACEMENT),
        replace(data, 0x026d7ad0, MISPLACED, STOCK),
    ]
    if len(data) != len(base) or digest(data) != CANDIDATE:
        raise PatchError('Candidate size/hash validation failed')
    # Reverse exactly the two owner corrections, leaving the parallel v14 intact.
    rev = bytearray(data)
    for rec in reversed(patches):
        replace(rev, int(rec['va'], 16), bytes.fromhex(rec['new']), bytes.fromhex(rec['old']))
    if bytes(rev) != base or digest(rev) != V14:
        raise PatchError('Exact v14 reversibility failed')
    image = Image(data)
    if image.read(0x02591118, 14) != NEW_DOOR or image.read(0x01c95300, 35) != V14_HELPER:
        raise PatchError('Parallel door patch was not preserved')
    if image.read(0x02db2b7d, 6) != bytes.fromhex('8b4d08909090'):
        raise PatchError('Existing ActionCommand getter fix was not preserved')
    return bytes(data), {
        'status': 'PASS_STATIC_BUILD_NOT_GAMEPLAY_VALIDATED',
        'candidate': 'pickup-owner-fix-experimental',
        'source_sha256': source_hash, 'base_v14_sha256': V14,
        'output_sha256': digest(data), 'output_size': len(data),
        'v13_to_published_v14': upgrade, 'owner_corrections': patches,
        'changed_bytes_vs_v14': sum(x['different_bytes'] for x in patches),
        'reversibility_to_v14': True, 'parallel_v14_preserved': True,
        'gameplay_executed': False,
        'limitations': [
            'Only the uItem member selector ownership is corrected; gameplay is untested.',
            'Door-gimmick Self/pl actor gates remain unresolved.',
            'Pickup proximity arbitration still chooses nearest before full eligibility.',
            'Use only with the compatible January development build and its assets.',
        ],
    }

def write_new(path: Path, data: bytes) -> None:
    # All binary/semantic checks happen before opening an output file.
    with path.open('xb') as stream:
        stream.write(data)

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    try:
        outputs = [args.output] + ([args.report] if args.report else [])
        paths = [args.source.resolve()] + [p.resolve() for p in outputs]
        if len(set(paths)) != len(paths):
            raise PatchError('Input/output/report paths must be distinct')
        if any(p.exists() for p in outputs):
            raise PatchError('Existing output/report will not be overwritten')
        result, report = build(args.source.read_bytes())
        text = json.dumps(report, indent=2) + '\n'
        write_new(args.output, result)
        if args.report:
            write_new(args.report, text.encode('utf-8'))
        print(text, end='')
        return 0
    except (OSError, ValueError, struct.error) as exc:
        print(json.dumps({'status': 'FAIL', 'reason': str(exc)}))
        return 2

if __name__ == '__main__':
    raise SystemExit(main())
