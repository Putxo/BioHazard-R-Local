#!/usr/bin/env python3
"""Component tests. The bounded interpreter never executes game/engine code.

Optional exact images enable byte/RTTI/reversibility tests. Without images,
CI exercises the actual embedded selector bytes and filesystem safety only.
"""
from __future__ import annotations
import argparse
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import contextlib
import io
import build_script_serial_guard as patch
from build_local_routing import PE, sha, pe_checksum

ORIGINAL_SHA='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
OUTPUT_SHA='71f5e70dc19c334d303ee29a686d65a72cd18b0c87b98472170bff961cb048f4'
ACTIVE=0x057D9188; TRACKER=0x057D9184
OWNER=0x10000000; ACTOR=0x20000000
VT=0x04DCF41C
COUNTS={'selector_scenarios':0,'image_assertions':0}
ARGS=None

def execute(code, memory, owner, entry=None):
    """Only this leaf's audited opcodes; absent memory or unknown opcodes fail."""
    pc=patch.HELPER_VA
    registers={'eax':0xAA55AA55,'ecx':owner,'edx':0x12345678,
               'ebx':11,'esi':12,'edi':13,'ebp':14,'esp':0x30000000}
    if entry is not None:
        if len(entry)!=9 or entry[0]!=0xE9 or entry[5:]!=b'\x90'*4:
            raise ValueError('bad callback trampoline')
        if ARGS is None:raise ValueError('entry test requires VA')
    initial=registers.copy(); reads=[]; zf=sf=False
    def read(address):
        reads.append(address)
        if address not in memory:raise ValueError(f'unmapped read {address:#x}')
        return memory[address]&0xFFFFFFFF
    def compare(a,b):
        r=(a-b)&0xFFFFFFFF
        return r==0,bool(r&0x80000000)
    for step in range(40):
        offset=pc-patch.HELPER_VA
        if not 0<=offset<len(code):raise ValueError('branch outside helper')
        data=code[offset:]; n=0
        if data.startswith(b'\x31\xC0'):
            registers['eax']=0;zf=True;sf=False;n=2
        elif data[:2] in (b'\x85\xC9',b'\x85\xD2'):
            v=registers['ecx' if data[1]==0xC9 else 'edx']
            zf,sf=compare(v,0);n=2
        elif data[:2]==b'\x83\x3D':
            address=struct.unpack_from('<I',data,2)[0]
            zf,sf=compare(read(address),data[6]);n=7
        elif data[:2]==b'\x81\x39':
            zf,sf=compare(read(registers['ecx']),struct.unpack_from('<I',data,2)[0]);n=6
        elif data[:2]==b'\x8B\x15':
            registers['edx']=read(struct.unpack_from('<I',data,2)[0]);n=6
        elif data[:2]==b'\x39\x91':
            zf,sf=compare(read(registers['ecx']+struct.unpack_from('<I',data,2)[0]),registers['edx']);n=6
        elif data[:2]==b'\x83\xBA':
            zf,sf=compare(read(registers['edx']+struct.unpack_from('<I',data,2)[0]),data[6]);n=7
        elif data[:2]==b'\x8B\x92':
            registers['edx']=read(registers['edx']+struct.unpack_from('<I',data,2)[0]);n=6
        elif data[0] in (0x74,0x75,0x78):
            take={0x74:zf,0x75:not zf,0x78:sf}[data[0]]
            pc+=2+(struct.unpack('b',data[1:2])[0] if take else 0);continue
        elif data[:3]==b'\x0F\x94\xC0':
            registers['eax']=(registers['eax']&0xFFFFFF00)|int(zf);n=3
        elif data[:3]==b'\xC2\x04\x00':
            registers['esp']+=8
            return registers,initial,reads
        else:raise ValueError(f'unsupported instruction {data[:8].hex()}')
        pc+=n
    raise ValueError('instruction limit exceeded')

class SelectorTests(unittest.TestCase):
    def check_case(self, local=1,owner=OWNER,vt=VT,tracker=ACTOR,context=ACTOR,mode=1,serial=1,bound=1,code=patch.HELPER):
        # Only map fields that a correctly ordered guard is permitted to read.
        memory={}
        if owner:
            memory[ACTIVE]=local
            if local:
                memory[owner]=vt
                if vt==VT:
                    memory[TRACKER]=tracker
                    if tracker:
                        memory[owner+0x1078]=context
                        if context==tracker:
                            memory[tracker+0xE40]=mode
                            if mode==1:
                                memory[tracker+0xE3C]=serial
                                if not serial&0x80000000:
                                    memory[owner+0x1074]=bound
        result,initial,reads=execute(code,memory,owner)
        expected=int(bool(owner and local and vt==VT and tracker and context==tracker and mode==1
                          and not serial&0x80000000 and serial==bound))
        self.assertEqual(result['eax'],expected)
        for name in ('ecx','ebx','esi','edi','ebp'):
            self.assertEqual(result[name],initial[name])
        self.assertEqual(result['esp'],initial['esp']+8)
        self.assertTrue(all(a in memory for a in reads))
        COUNTS['selector_scenarios']+=1
    def test_identity(self):
        self.assertEqual(len(patch.HELPER),72)
        self.assertEqual(sha(patch.HELPER),patch.HELPER_SHA)
    def test_null_and_layout_guards(self):
        for case in ({'owner':0},{'local':0},{'vt':0},{'vt':0x04DBD47C},
                     {'vt':0x04DC7AD8},{'vt':0x04E15EF4},{'tracker':0},
                     {'context':0},{'context':ACTOR+0x10000}):
            with self.subTest(case=case):self.check_case(**case)
    def test_mode_and_serial_matrix(self):
        for local,mode,serial,bound in itertools.product((0,1,2,0xFFFFFFFF),range(4),
             (0xFFFFFFFF,0,1,2,127,128,0x7FFFFFFF,0x80000000),
             (0xFFFFFFFF,0,1,2,127,128,0x7FFFFFFF,0x80000000)):
            with self.subTest(local=local,mode=mode,serial=serial,bound=bound):
                self.check_case(local=local,mode=mode,serial=serial,bound=bound)
    def test_unknown_and_outside_bytes_fail(self):
        with self.assertRaises(ValueError):execute(b'\xCC',{},0)
        with self.assertRaises(ValueError):execute(b'\x74\x7F',{},0)
    def test_unbound_and_rebound_same_pointer(self):
        for serial,bound in ((1,1),(1,0xFFFFFFFF),(1,2),(2,1),(2,2)):
            self.check_case(serial=serial,bound=bound)
    def test_assembly_matches(self):
        source=Path(__file__).resolve().parents[1]/'research/patches/script_member_serial_guard.S'
        if not source.is_file():self.skipTest('assembly source not mounted')
        import shutil
        if not shutil.which('as') or not shutil.which('objcopy'):self.skipTest('GNU tools unavailable')
        with tempfile.TemporaryDirectory() as td:
            obj=Path(td)/'helper.o';binary=Path(td)/'helper.bin'
            subprocess.run(['as','--32',str(source),'-o',str(obj)],check=True,capture_output=True)
            subprocess.run(['objcopy','-O','binary','-j','.text',str(obj),str(binary)],check=True,capture_output=True)
            self.assertEqual(binary.read_bytes(),patch.HELPER)

class CliTests(unittest.TestCase):
    def test_no_overwrite(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);src=root/'source';out=root/'out';report=root/'report'
            src.write_bytes(b'not the game');out.write_bytes(b'existing')
            calls=[(src,src,report),(src,out,report),(src,report,src),(src,report,report)]
            for a,b,c in calls:
                with self.subTest(paths=(a,b,c)),contextlib.redirect_stderr(io.StringIO()):
                    self.assertEqual(patch.main([str(a),str(b),'--report',str(c)]),2)
            self.assertEqual(src.read_bytes(),b'not the game');self.assertEqual(out.read_bytes(),b'existing')
            self.assertFalse(report.exists())
    def test_bad_source_and_dangling_symlink(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);src=root/'source';out=root/'out';report=root/'report'
            src.write_bytes(b'MZ'+bytes(254))
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(patch.main([str(src),str(out),'--report',str(report)]),2)
            self.assertFalse(out.exists());self.assertFalse(report.exists())
            try:out.symlink_to(root/'does-not-exist')
            except OSError:self.skipTest('symlink unavailable')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(patch.main([str(src),str(out),'--report',str(report)]),2)
            self.assertTrue(out.is_symlink());self.assertFalse(report.exists())

class ExactImageTests(unittest.TestCase):
    def test_images(self):
        if not ARGS.original or not ARGS.base or not ARGS.candidate:self.skipTest('local game images not supplied')
        original=ARGS.original.read_bytes();base=ARGS.base.read_bytes();output=ARGS.candidate.read_bytes()
        pe=PE(original);bp=PE(base);op=PE(output)
        def ck(a,b):self.assertEqual(a,b);COUNTS['image_assertions']+=1
        ck(sha(original),ORIGINAL_SHA);ck(sha(base),patch.BASE_SHA);ck(sha(output),OUTPUT_SHA)
        rebuilt,report=patch.build(base);ck(rebuilt,output);ck(len(output),patch.BASE_SIZE)
        ck(op.sections,bp.sections)
        # RTTI owner chain and setter producer, checked on local original bytes.
        for vt,col,td,name in (
            (0x04DCF41C,0x05374250,0x054C5294,b'.?AVcFsmActionPcsSub@fsm@game@app@@'),
            (0x04DBD47C,0x05365424,0x054B6DBC,b'.?AVcFsmAction@fsm@game@app@@'),
            (0x04DC7AD8,0x0536AEF0,0x054BC418,b'.?AVcFsmActionPcs@fsm@game@app@@')):
            ck(pe.read(vt-4,4),struct.pack('<I',col));ck(pe.read(col+12,4),struct.pack('<I',td))
            ck(pe.read(td+8,len(name)),name)
        for va,data in ((0x02A75BEE,'c7001cf4dc04'),(0x02DCFBF5,'898874100000'),
                        (0x02DCFC01,'898878100000')):
            ck(pe.read(va,len(bytes.fromhex(data))),bytes.fromhex(data))
        for va,thunk in ((0x029707B1,0x01BBB149),(0x029FD2E7,0x01C7E054)):
            ck(pe.read(va,5),b'\x68'+struct.pack('<I',thunk))
        for va in patch.ENTRY_VAS:
            ck(pe.read(va,len(patch.STOCK_BODY)),patch.STOCK_BODY)
            prefix=op.read(va,9);ck(prefix[0],0xE9)
            ck(va+5+struct.unpack('<i',prefix[1:5])[0],patch.HELPER_VA)
            ck(prefix[5:],b'\x90'*4)
            # Execute the actual helper extracted from the output.
            self.assertEqual(execute(op.read(patch.HELPER_VA,72),{},0)[0]['eax'],0)
        ck(op.read(patch.HELPER_VA,72),patch.HELPER)
        ck(op.read(0x02DEE8D0,46),bp.read(0x02DEE8D0,46))
        for section in bp.sections[-2:]:
            o=section['raw'];n=section['raw_size'];ck(output[o:o+n],base[o:o+n])
        reverse=bytearray(output);mask=bytearray(len(base))
        for change in report['changes']:
            o=change['offset'];n=change['size'];reverse[o:o+n]=bytes.fromhex(change['old']);mask[o:o+n]=b'\1'*n
        ck(bytes(reverse),base)
        ck(sum(x!=y for x,y in zip(base,output)),report['changed_bytes'])
        ck(all(x==y or allowed for x,y,allowed in zip(base,output,mask)),True)
        ck(op.u32(op.opt+64),pe_checksum(output,op.opt+64))

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('original','base','candidate','report'):parser.add_argument('--'+key,type=Path)
    ARGS=parser.parse_args()
    suite=unittest.defaultTestLoader.loadTestsFromModule(sys.modules[__name__])
    result=unittest.TextTestRunner(verbosity=2).run(suite)
    report=dict(status='PASS' if result.wasSuccessful() else 'FAIL',tests_run=result.testsRun,
        skipped=len(result.skipped),failures=len(result.failures),errors=len(result.errors),**COUNTS,
        helper_sha256=sha(patch.HELPER),gameplay_executed=False,native_win32_executed=False,
        method='bounded instruction interpreter of the embedded/extracted leaf; no engine calls executed')
    if ARGS.report:
        with ARGS.report.open('x',encoding='utf-8') as f:json.dump(report,f,indent=2);f.write('\n')
    print(json.dumps(report))
    raise SystemExit(0 if result.wasSuccessful() else 1)
