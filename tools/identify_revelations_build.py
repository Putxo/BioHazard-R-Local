#!/usr/bin/env python3
"""Identify four exact PC Revelations builds without executing them.

Only BioRevHD 2013-01-30 is an accepted *input* to the existing January
local-coop source build. Recognition is not evidence of functional gameplay,
and this tool does not install or modify any file.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
from typing import Any

# These digests are of unmodified, user-supplied PE32 executables. The February,
# May and retail builds are reference inputs for future ports, not patch targets.
BUILDS: dict[str, dict[str, Any]] = {
    '9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69': {
        'build': 'BioRevHD_2013_01_30', 'date': '2013-01-30',
        'size_bytes': 60748800, 'entry_rva': 0x018897E7,
        'section_names': ('.textbss', '.text', '.rdata', '.data', '.idata', '.didat', '.tls', '.rsrc'),
        'mod_status': 'JANUARY_SOURCE_SUPPORTED',
    },
    'f67742e6f8d0f6ade1defc2dcdc34ba06e9e95d2abdc630a605a7da9b437a041': {
        'build': 'BioRevHD_2013_02_23', 'date': '2013-02-23',
        'size_bytes': 16100864, 'entry_rva': 0x00CE982F,
        'section_names': ('.text', '.rdata', '.data', '.tls', '.rsrc'),
        'mod_status': 'PORT_REQUIRED',
    },
    'a015f15a92dd4d242404d83d012a5319d677c15195a0fe862091ebb827dafe34': {
        'build': 'rerev_2013_05_17', 'date': '2013-05-17',
        'size_bytes': 16736600, 'entry_rva': 0x00FFE2EE,
        'section_names': ('.text', '.rdata', '.data', '.tls', '.rsrc', '.bind'),
        'mod_status': 'PORT_REQUIRED',
    },
    '1573b79eb921b1e7f571e6deb71c39961c7f1940fad6d19283fc3aea7fca48aa': {
        'build': 'rerev_2024_02_07', 'date': '2024-02-07',
        'size_bytes': 14260576, 'entry_rva': 0x00DF0310,
        'section_names': ('.text', '.rdata', '.data', '.rsrc', '.bind'),
        'mod_status': 'PORT_REQUIRED',
    },
}

def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from('<H', data, offset)[0]

def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from('<I', data, offset)[0]

def inspect_pe(prefix: bytes, filesize: int) -> dict[str, Any]:
    """Validate fixed PE32 header layout, returning facts only (no execution)."""
    if len(prefix) < 0x40 or prefix[:2] != b'MZ':
        raise ValueError('not a DOS MZ executable')
    pe_off = u32(prefix, 0x3C)
    if pe_off < 0x40 or pe_off > 0x10000 - 1024 or pe_off + 24 > len(prefix):
        raise ValueError('PE header outside inspection limit')
    if prefix[pe_off:pe_off + 4] != b'PE\0\0':
        raise ValueError('missing PE signature')
    machine, count, timestamp = struct.unpack_from('<HHI', prefix, pe_off + 4)
    optional_size = u16(prefix, pe_off + 20)
    optional_off = pe_off + 24
    section_off = optional_off + optional_size
    if machine != 0x014C or not 1 <= count <= 64:
        raise ValueError('requires x86 PE with 1-64 sections')
    if optional_size < 96 or section_off + count * 40 > len(prefix):
        raise ValueError('truncated PE optional header or section table')
    if u16(prefix, optional_off) != 0x10B:
        raise ValueError('requires PE32, not PE32+ or another format')
    entry_rva = u32(prefix, optional_off + 16)
    image_base = u32(prefix, optional_off + 28)
    image_size = u32(prefix, optional_off + 56)
    dll_characteristics = u16(prefix, optional_off + 70)
    names: list[str] = []
    spans: list[tuple[int, int]] = []
    sections: list[dict[str, Any]] = []
    for i in range(count):
        off = section_off + i * 40
        raw_name, virt_size, rva, raw_size, raw_offset = struct.unpack_from('<8sIIII', prefix, off)
        if not raw_name.strip(b'\0') or not all(32 <= c < 127 for c in raw_name.rstrip(b'\0')):
            raise ValueError(f'invalid name in section {i}')
        name = raw_name.split(b'\0', 1)[0].decode('ascii')
        if name in names:
            raise ValueError(f'duplicate section {name}')
        if raw_size and (not raw_offset or raw_offset + raw_size > filesize):
            raise ValueError(f'section {name} outside executable')
        if raw_size:
            spans.append((raw_offset, raw_offset + raw_size))
        names.append(name)
        sections.append({'name': name, 'rva': rva, 'virtual_size': virt_size,
                         'file_offset': raw_offset, 'file_size': raw_size})
    spans.sort()
    if any(a[1] > b[0] for a, b in zip(spans, spans[1:])):
        raise ValueError('overlapping section file ranges')
    if not image_size or not entry_rva or entry_rva >= image_size:
        raise ValueError('invalid PE entry or image size')
    return {
        'machine': 'x86', 'format': 'PE32', 'pe_timestamp': timestamp,
        'image_base': image_base, 'entry_rva': entry_rva,
        'entry_va': image_base + entry_rva, 'image_size': image_size,
        'aslr_enabled': bool(dll_characteristics & 0x40),
        'section_names': names, 'sections': sections,
    }

def match_build(digest: str, size: int, pe: dict[str, Any]) -> dict[str, Any]:
    """Never accept a version by name, approximate bytes or isolated signatures."""
    info = BUILDS.get(digest)
    if info is None:
        return {'build': None, 'date': None, 'mod_status': 'UNKNOWN_BUILD'}
    if (size != info['size_bytes'] or pe['image_base'] != 0x400000 or
            pe['entry_rva'] != info['entry_rva'] or
            tuple(pe['section_names']) != info['section_names']):
        return {'build': None, 'date': None, 'mod_status': 'INCONSISTENT_BUILD'}
    return {key: info[key] for key in ('build', 'date', 'mod_status')}

def inspect_file(path: Path) -> dict[str, Any]:
    """Stream the hash to avoid unnecessary copies of the large January build."""
    result: dict[str, Any] = {'filename': path.name}
    try:
        with path.open('rb') as f:
            head = f.read(65536)
            h = hashlib.sha256(head)
            count = len(head)
            for chunk in iter(lambda: f.read(1024 * 1024), b''):
                h.update(chunk)
                count += len(chunk)
        result.update({'size_bytes': count, 'sha256': h.hexdigest()})
        pe = inspect_pe(head, count)
        result.update(pe)
        result.update(match_build(h.hexdigest(), count, pe))
    except (OSError, ValueError, struct.error) as e:
        result.update({'mod_status': 'INVALID_OR_UNREADABLE', 'reason': str(e)})
    return result

def inspect_paths(paths: list[Path]) -> list[dict[str, Any]]:
    results = []
    first_by_hash: dict[str, str] = {}
    for path in paths:
        record = inspect_file(path)
        digest = record.get('sha256')
        if digest in first_by_hash:
            record['identical_to'] = first_by_hash[digest]
        elif digest:
            first_by_hash[digest] = path.name
        results.append(record)
    return results

def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executables', nargs='+', type=Path)
    parser.add_argument('--report', type=Path, help='Write a NEW JSON report; refuse overwrite')
    parser.add_argument('--require-mod-target', action='store_true',
                        help='Return code 3 unless ALL inputs match the supported January source')
    a = parser.parse_args(argv)
    if a.report and (a.report.exists() or a.report.resolve() in
                     {p.resolve() for p in a.executables}):
        parser.error('report must be new and cannot replace an executable')
    results = inspect_paths(a.executables)
    output = {'schema': 1, 'purpose': 'static_build_identification',
              'game_executed': False, 'game_modified': False, 'inputs': results}
    text = json.dumps(output, ensure_ascii=False, indent=2) + '\n'
    if a.report:
        with a.report.open('x', encoding='utf-8') as f:
            f.write(text)
    print(text, end='')
    if any(r['mod_status'] == 'INVALID_OR_UNREADABLE' for r in results):
        return 2
    if a.require_mod_target and any(r['mod_status'] != 'JANUARY_SOURCE_SUPPORTED' for r in results):
        return 3
    return 0

if __name__ == '__main__':
    sys.exit(main())
