"""Regressions against compiled mod bytes; no proprietary image required."""
import argparse
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import check_local_input as check


def tests(module, symbols):
    va, compiled = module['.lcfix']
    entry = symbols['hook_bind_actor']
    count = 0
    class Reader:
        def __init__(self):
            self.code = bytearray(compiled)
            self.site = check.rel(check.BIND_SITE, entry, 0xE9) + b'\x90'*8
            self.sections = [dict(name='.lcfix', va=va, virtual_size=len(compiled),
                                  raw_size=len(compiled), characteristics=0x60000020)]
        def read(self, at, n):
            if at == check.BIND_SITE and n == 13:return self.site
            if va <= at and at+n <= va+len(self.code):return bytes(self.code[at-va:at-va+n])
            raise ValueError('read outside compiled mod')
    def expect(reader, status):
        nonlocal count
        assert check.inspect_binder(reader)['status'] == status
        count += 1
    expect(Reader(), 'CPU3_BINDER_PRESENT')
    legacy = Reader();legacy.code[entry-va+36] = 2
    expect(legacy, 'LEGACY_CPU2_BUG')
    # Every nonrelocation byte is part of the audited control flow/identity
    # contract. Tampering must not result in a corrected-input verdict.
    relocation_bytes = {i for start in (3,74,84,99,104) for i in range(start,start+4)}
    for offset in range(check.BIND_SIZE):
        if offset in relocation_bytes:continue
        reader = Reader();reader.code[entry-va+offset] ^= 0x80
        try:check.inspect_binder(reader)
        except ValueError:count += 1
        else:raise AssertionError(f'accepted corrupt binder byte {offset}')
    # A different target of the native setter must also be rejected.
    for offset in (84,99,104):
        reader = Reader();reader.code[entry-va+offset] ^= 1
        try:check.inspect_binder(reader)
        except ValueError:count += 1
        else:raise AssertionError(f'accepted altered native target {offset}')
    for flags in (0xC0000040,0xE0000020,0x40000020):
        reader = Reader();reader.sections[0]['characteristics'] = flags
        try:check.inspect_binder(reader)
        except ValueError:count += 1
        else:raise AssertionError('accepted wrong section permissions')
    return count


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('module',type=Path)
    args=parser.parse_args()
    from build_local_routing import elf_module
    module,symbols=elf_module(args.module)
    print(f'PASS: {tests(module,symbols)} compiled-binder checks; game not executed')
