#!/usr/bin/env python3
"""Focused bytecode interpretation and structural tests, never game execution.

The small interpreter supports only the instructions used by this selector.
It is not a general x86 emulator and does not execute the game's engine.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
import build_pickup_owner_fix as patch

ORIGINAL_SHA = '9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
ACTIVE, SUB = 0x057d9188, 0x057d9184

def interpret(code: bytes, this: int, memory: dict[int, int]) -> dict:
    pc, eax, edx, zf = 0, 0xdeadbeef, 0xabcdef01, False
    reads = []
    def read(address):
        reads.append(address)
        if address not in memory:
            raise ValueError(f'Unexpected memory access at {address:#x}')
        return memory[address] & 0xffffffff
    for _ in range(24):
        op = code[pc:pc+2]
        if op == b'\x31\xc0':
            eax, zf, pc = 0, True, pc + 2
        elif op == b'\x85\xc9':
            zf, pc = this == 0, pc + 2
        elif code[pc:pc+1] == b'\x74':
            displacement = struct.unpack('<b', code[pc+1:pc+2])[0]
            pc += 2 + (displacement if zf else 0)
        elif op == b'\x83\x3d':
            address = struct.unpack('<I', code[pc+2:pc+6])[0]
            immediate = struct.unpack('<b', code[pc+6:pc+7])[0] & 0xffffffff
            zf, pc = read(address) == immediate, pc + 7
        elif op == b'\x8b\x15':
            address = struct.unpack('<I', code[pc+2:pc+6])[0]
            edx, pc = read(address), pc + 6
        elif op == b'\x85\xd2':
            zf, pc = edx == 0, pc + 2
        elif op == b'\x39\x91':
            displacement = struct.unpack('<i', code[pc+2:pc+6])[0]
            zf, pc = read((this + displacement) & 0xffffffff) == edx, pc + 6
        elif code[pc:pc+3] == b'\x0f\x94\xc0':
            eax, pc = (eax & 0xffffff00) | int(zf), pc + 3
        elif code[pc:pc+1] == b'\xc2':
            cleanup = struct.unpack('<H', code[pc+1:pc+3])[0]
            return {'eax': eax, 'ecx': this, 'stack_delta': 4 + cleanup, 'reads': reads}
        else:
            raise ValueError(f'Unsupported instruction at offset {pc}: {code[pc:pc+4].hex()}')
    raise ValueError('Instruction limit exceeded')

def run(original: bytes, v13: bytes) -> dict:
    results = []
    def check(name, condition):
        if not condition:
            raise ValueError('Test failed: ' + name)
        results.append(name)
    def rejects(name, fn):
        try:
            fn()
        except (ValueError, struct.error):
            results.append(name)
        else:
            raise ValueError('Negative test unexpectedly passed: ' + name)

    check('original SHA pinned', patch.digest(original) == ORIGINAL_SHA)
    check('v13 SHA pinned', patch.digest(v13) == patch.V13)
    orig, base13 = patch.Image(original), patch.Image(v13)
    for vt, col, td, name in [
        (0x04d2fee4, 0x05325a60, 0x0545e070, '.?AVuItem@chara@game@app@@'),
        (0x04d926dc, 0x0534cf00, 0x0547c7dc, '.?AVuObjModel@obj_model@chara@game@app@@'),
    ]:
        actual = (orig.read(vt-4, 4), orig.read(col+12, 4), orig.read(td+8, len(name)+1))
        expected = (struct.pack('<I', col), struct.pack('<I', td), name.encode()+b'\0')
        check('RTTI owner ' + name, actual == expected)
    for name, va, data in [
        ('uItem selector binding', 0x0245f3f6, '68d30dc601'),
        ('uItem ActionCommand location', 0x0245f513, '81c1d00f0000'),
        ('uObjModel selector binding', 0x026d7216, '68b799ba01'),
        ('uObjModel F48 is initialized to integer 6', 0x026d440a, 'c780480f000006000000'),
        ('uItem F48 starts null', 0x0245ea1e, 'c780480f000000000000'),
    ]:
        check(name, orig.read(va, len(bytes.fromhex(data))) == bytes.fromhex(data))
    def target(image, va):
        b = image.read(va, 5)
        if b[0] not in (0xe8, 0xe9):
            raise ValueError('Not a direct CALL/JMP')
        return va + 5 + struct.unpack('<i', b[1:])[0]
    check('uItem thunk points to true selector', target(orig, 0x01c60dd3) == 0x024605d0)
    check('uObjModel thunk points to misplaced selector', target(orig, 0x01ba99b7) == 0x026d7ad0)
    check('v13 real pickup selector remains stock', base13.read(0x024605d0, 46) == patch.STOCK)
    check('v13 common model selector was changed', base13.read(0x026d7ad0, 46) == patch.MISPLACED)

    built, report13 = patch.build(v13)
    # Independent reconstruction of the two published v14 hunks for comparison.
    v14 = bytearray(v13)
    for va, expected, new in [(0x01c95300, b'\xcc'*35, patch.V14_HELPER),
                              (0x02591118, patch.OLD_DOOR, patch.NEW_DOOR)]:
        pos = base13.offset(va, len(expected))
        check(f'v14 precondition at {va:#x}', v14[pos:pos+len(expected)] == expected)
        v14[pos:pos+len(expected)] = new
    check('published v14 SHA reproduced', patch.digest(v14) == patch.V14)
    built14, report14 = patch.build(bytes(v14))
    check('v13 and v14 routes produce identical candidate', built == built14)
    check('candidate SHA pinned', patch.digest(built) == patch.CANDIDATE)
    check('unchanged PE file size', len(built) == len(v13))
    candidate = patch.Image(built)
    check('true pickup selector installed', candidate.read(0x024605d0,46) == patch.REPLACEMENT)
    check('common model selector restored exactly', candidate.read(0x026d7ad0,46) == orig.read(0x026d7ad0,46))
    check('v14 helper preserved exactly', candidate.read(0x01c95300,35) == patch.V14_HELPER)
    check('v14 callsite preserved exactly', candidate.read(0x02591118,14) == patch.NEW_DOOR)
    check('input getter fix retained', candidate.read(0x02db2b7d,6) == bytes.fromhex('8b4d08909090'))
    allowed = set()
    for va in (0x024605d0, 0x026d7ad0):
        pos = candidate.offset(va,46)
        allowed.update(range(pos,pos+46))
    differences = {i for i, (a,b) in enumerate(zip(v14,built)) if a != b}
    check('all differences confined to two selectors', differences <= allowed)
    check('91 effective changed bytes', len(differences) == 91)
    reversed_data = bytearray(built)
    for va in (0x024605d0, 0x026d7ad0):
        pos = candidate.offset(va,46)
        reversed_data[pos:pos+46] = v14[pos:pos+46]
    check('reverse correction restores v14 byte-for-byte', bytes(reversed_data) == bytes(v14))
    check('door-gimmick Self/pl gates remain untouched', candidate.read(0x02590040,0x610) == base13.read(0x02590040,0x610))
    cases = [
        ('null this, no memory accessed',0,{},0,[]),
        ('inactive local, tracker not read',0x10000,{ACTIVE:0},0,[ACTIVE]),
        ('null tracker, owner field not read',0x10000,{ACTIVE:1,SUB:0},0,[ACTIVE,SUB]),
        ('null candidate',0x10000,{ACTIVE:1,SUB:0x20000,0x10f48:0},0,[ACTIVE,SUB,0x10f48]),
        ('exact local Sub0',0x10000,{ACTIVE:1,SUB:0x20000,0x10f48:0x20000},1,[ACTIVE,SUB,0x10f48]),
        ('P1 candidate',0x10000,{ACTIVE:1,SUB:0x20000,0x10f48:0x30000},0,[ACTIVE,SUB,0x10f48]),
        ('other NPC candidate',0x10000,{ACTIVE:1,SUB:0x20000,0x10f48:0x40000},0,[ACTIVE,SUB,0x10f48]),
        ('nonzero local flag treated as bool',0x10000,{ACTIVE:2,SUB:0x20000,0x10f48:0x20000},1,[ACTIVE,SUB,0x10f48]),
    ]
    extracted = candidate.read(0x024605d0,len(patch.SELECTOR))
    for name, this, memory, expected, reads in cases:
        actual = interpret(extracted,this,memory)
        check('interpreted selector: ' + name,
              actual == {'eax':expected,'ecx':this,'stack_delta':8,'reads':reads})
    rejects('wrong source hash rejected',lambda: patch.build(original))
    rejects('corrupted v13 rejected',lambda: patch.build(v13[:-1]+bytes([v13[-1]^1])))
    rejects('repatching candidate rejected',lambda: patch.build(built))
    rejects('unexpected old bytes rejected',lambda: patch.replace(bytearray(v14),0x024605d0,b'\0',b'\x90'))
    rejects('length-changing patch rejected',lambda: patch.replace(bytearray(v14),0x024605d0,b'\0',b'\x90\x90'))
    rejects('unmapped VA rejected',lambda: candidate.read(0x00401000,4))
    rejects('unsupported bytecode rejected',lambda: interpret(b'\xcc',0,{}))
    rejects('truncated return rejected',lambda: interpret(b'\xc2',0,{}))
    return {'status':'PASS_FOCUSED_TESTS', 'passed':len(results), 'tests':results,
            'selector_interpretation_cases':len(cases),
            'gameplay_executed':False, 'native_game_code_executed':False,
            'test_method':'Structural checks plus a limited interpreter of the emitted selector instructions; not a general x86 emulator.',
            'source_sha256':ORIGINAL_SHA,'v13_sha256':patch.V13,
            'v14_sha256':patch.V14,'candidate_sha256':patch.CANDIDATE}

def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('original',type=Path)
    p.add_argument('v13',type=Path)
    p.add_argument('--report',type=Path)
    args = p.parse_args()
    try:
        if args.report and (args.report.resolve() in {args.original.resolve(),args.v13.resolve()} or args.report.exists()):
            raise ValueError('Report cannot overwrite inputs or an existing file')
        result = run(args.original.read_bytes(),args.v13.read_bytes())
        text = json.dumps(result,indent=2)+'\n'
        if args.report:
            args.report.write_text(text,encoding='utf-8')
        print(text,end='')
        return 0
    except (OSError,ValueError,struct.error) as exc:
        print(json.dumps({'status':'FAIL','reason':str(exc)}))
        return 2

if __name__ == '__main__':
    raise SystemExit(main())
