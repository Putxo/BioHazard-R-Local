#!/usr/bin/env python3
"""Test only the new leaf's instructions and patch integrity. Never runs the game.

The interpreter rejects unsupported instructions and unmapped reads. No engine
functions are mocked: this leaf contains no engine calls. This is still not a
full CPU emulator or gameplay test. --base/--candidate/--original are local-only.
"""
from __future__ import annotations
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import build_script_member as patch

ACTIVE=0x057D9188
TRACKER=0x057D9184
OWNER=0x10000000
ACTOR=0x20000000
OTHER=0x30000000
SUB_VT=0x04DCF41C

class LeafMachine:
    def __init__(self, read_code, memory, owner):
        self.code=read_code
        self.mem=dict(memory)
        self.reg={'eax':0x12345678,'ecx':owner,'edx':0xdeadbeef,
                  'ebx':0x98765432,'esi':0x13579bdf,'edi':0x2468ace0,
                  'ebp':0x76543210,'esp':0x40000000}
        self.zf=False;self.sf=False;self.reads=[];self.steps=0
        self.mem[self.reg['esp']]=0xfeed1234
        self.mem[self.reg['esp']+4]=0xa5a5a5a5
    def read32(self,address):
        address &= 0xffffffff
        self.reads.append(address)
        if address not in self.mem:
            raise AssertionError(f'unmapped read at {address:#x}')
        return self.mem[address] & 0xffffffff
    def flags(self,value):
        value &= 0xffffffff
        self.zf=(value==0);self.sf=bool(value & 0x80000000)
    def run(self,start):
        ip=start
        for _ in range(80):
            self.steps+=1
            b=self.code(ip,10)
            if b[:2]==b'\x31\xc0':
                self.reg['eax']=0;self.flags(0);ip+=2
            elif b[:2] in (b'\x85\xc9',b'\x85\xd2'):
                self.flags(self.reg['ecx' if b[1]==0xc9 else 'edx']);ip+=2
            elif b[0] in (0x74,0x75,0x78):
                take=(self.zf if b[0]==0x74 else not self.zf if b[0]==0x75 else self.sf)
                ip+=2+(struct.unpack('b',b[1:2])[0] if take else 0)
            elif b[:2]==b'\x83\x3d':
                address=struct.unpack_from('<I',b,2)[0]
                immediate=struct.unpack('b',b[6:7])[0]
                self.flags(self.read32(address)-immediate);ip+=7
            elif b[:2]==b'\x81\x39':
                self.flags(self.read32(self.reg['ecx'])-struct.unpack_from('<I',b,2)[0]);ip+=6
            elif b[:2]==b'\x8b\x15':
                self.reg['edx']=self.read32(struct.unpack_from('<I',b,2)[0]);ip+=6
            elif b[:2]==b'\x83\xba':
                address=self.reg['edx']+struct.unpack_from('<i',b,2)[0]
                self.flags(self.read32(address)-struct.unpack('b',b[6:7])[0]);ip+=7
            elif b[:2]==b'\x39\x91':
                address=self.reg['ecx']+struct.unpack_from('<i',b,2)[0]
                self.flags(self.read32(address)-self.reg['edx']);ip+=6
            elif b[:2]==b'\x8b\x92':
                self.reg['edx']=self.read32(self.reg['edx']+struct.unpack_from('<i',b,2)[0]);ip+=6
            elif b[:3]==b'\x0f\x94\xc0':
                self.reg['eax']=(self.reg['eax'] & 0xffffff00)|int(self.zf);ip+=3
            elif b[0]==0xe9:
                ip=(ip+5+struct.unpack_from('<i',b,1)[0]) & 0xffffffff
            elif b[0]==0x90:
                ip+=1
            elif b[0]==0xc2:
                target=self.read32(self.reg['esp'])
                self.reg['esp']+=4+struct.unpack_from('<H',b,1)[0]
                if target!=0xfeed1234:raise AssertionError('wrong return address')
                return self.reg['eax']
            else:
                raise AssertionError(f'unsupported opcode {b.hex()} at {ip:#x}')
        raise AssertionError('instruction budget exceeded')

def fixture(flag=1,owner=OWNER,vt=SUB_VT,tracker=ACTOR,mode=1,context=ACTOR,serial=1,stored=1):
    return {ACTIVE:flag,TRACKER:tracker,OWNER:vt,OWNER+0x1078:context,
            OWNER+0x1074:stored,ACTOR+0xE40:mode,ACTOR+0xE3C:serial}

def embedded_code(va,count):
    delta=va-patch.CAVE
    if 0<=delta<len(patch.HELPER):return (patch.HELPER[delta:]+b'\xcc'*count)[:count]
    for site in patch.CALLBACKS:
        if va==site:return (b'\xe9'+struct.pack('<i',patch.CAVE-site-5)+b'\x90'*count)[:count]
    raise AssertionError(f'unmapped code {va:#x}')

class Tests(unittest.TestCase):
    code=staticmethod(embedded_code)
    base=None;candidate=None;original=None
    def execute(self,owner,memory,start=patch.CAVE):
        m=LeafMachine(self.code,memory,owner)
        before=dict(m.reg)
        value=m.run(start)
        for register in ('ebx','esi','edi','ebp','ecx'):
            self.assertEqual(m.reg[register],before[register],register)
        self.assertEqual(m.reg['esp'],before['esp']+8,'ret 4 stack cleanup')
        return value,m
    def test_helper_identity(self):
        self.assertEqual(len(patch.HELPER),72)
        self.assertEqual(patch.digest(patch.HELPER),patch.HELPER_SHA)
    def test_guard_matrix(self):
        # Independent contract, exercised against decoded helper instructions.
        count=0
        for flag,vt,tracker,mode,context,serial,stored in itertools.product(
            (0,1,0xffffffff),(SUB_VT,0x04DCBFAC,0x04DBD47C,0x04E15EF4),
            (0,ACTOR),(0,1,2,3),(0,ACTOR,OTHER),(-1,1,127),(-1,1,127)):
            memory=fixture(flag=flag,vt=vt,tracker=tracker,mode=mode,
                           context=context,serial=serial,stored=stored)
            expected=int(flag!=0 and vt==SUB_VT and tracker==ACTOR and mode==1
                         and context==ACTOR and serial>=0 and serial==stored)
            actual,m=self.execute(OWNER,memory)
            self.assertEqual(actual,expected,(flag,vt,tracker,mode,context,serial,stored))
            if not flag:self.assertNotIn(OWNER,m.reads)
            if vt!=SUB_VT:self.assertNotIn(OWNER+0x1078,m.reads)
            if not tracker:self.assertNotIn(ACTOR+0xE40,m.reads)
            count+=1
        self.assertEqual(count,2592)
    def test_null_owner_without_globals(self):
        value,m=self.execute(0,{})
        self.assertEqual(value,0)
        self.assertEqual(m.reads,[0x40000000])
    def test_inactive_invalid_owner_no_dereference(self):
        value,m=self.execute(1,{ACTIVE:0})
        self.assertEqual(value,0)
        self.assertEqual(m.reads,[ACTIVE,0x40000000])
    def test_unrelated_type_does_not_read_extended_layout(self):
        value,m=self.execute(OWNER,{ACTIVE:1,OWNER:0x04E15EF4})
        self.assertEqual(value,0)
        self.assertEqual(m.reads,[ACTIVE,OWNER,0x40000000])
    def test_null_tracker_needs_no_actor_mapping(self):
        value,m=self.execute(OWNER,{ACTIVE:1,OWNER:SUB_VT,TRACKER:0})
        self.assertEqual(value,0)
        self.assertNotIn(OWNER+0x1078,m.reads)
    def test_network_does_not_read_serial(self):
        value,m=self.execute(OWNER,{ACTIVE:1,OWNER:SUB_VT,TRACKER:ACTOR,OWNER+0x1078:ACTOR,ACTOR+0xE40:3})
        self.assertEqual(value,0)
        self.assertNotIn(ACTOR+0xE3C,m.reads)
        self.assertNotIn(OWNER+0x1074,m.reads)
    def test_mismatched_context_avoids_invalid_tracker(self):
        value,m=self.execute(OWNER,{ACTIVE:1,OWNER:SUB_VT,TRACKER:1,OWNER+0x1078:ACTOR})
        self.assertEqual(value,0)
        self.assertNotIn(1+0xE40,m.reads)
    def test_context_pointer_is_not_dereferenced(self):
        memory=fixture(context=1)
        value,m=self.execute(OWNER,memory)
        self.assertEqual(value,0)
        self.assertNotIn(1,m.reads)
    def test_full_nonnegative_serial_range(self):
        for value in (0,17,127,128,0x7fffffff):
            actual,_=self.execute(OWNER,fixture(serial=value,stored=value))
            self.assertEqual(actual,1)
    def test_negative_serials_are_inactive(self):
        for value in (-1,0x80000000,0xffffffff):
            actual,_=self.execute(OWNER,fixture(serial=value,stored=value))
            self.assertEqual(actual,0)
    def test_context_rebind_and_mode_changes(self):
        for changes,expected in (({},1),({'stored':2},0),({'context':OTHER},0),
                                  ({'tracker':0},0),({'mode':2},0),({'mode':3},0),
                                  ({'flag':0},0),({},1)):
            actual,_=self.execute(OWNER,fixture(**changes))
            self.assertEqual(actual,expected)
    def test_both_real_entry_bridges(self):
        for address in patch.CALLBACKS:
            self.assertEqual(self.execute(OWNER,fixture(),address)[0],1)
            self.assertEqual(self.execute(OWNER,fixture(mode=3),address)[0],0)
    def test_structural_image(self):
        if self.base is None:self.skipTest('no local game files supplied')
        candidate,report=patch.build(self.base)
        self.assertEqual(candidate,self.candidate)
        self.assertEqual(report['output_sha256'],patch.OUTPUT_SHA)
        original=patch.Image(self.base);out=patch.Image(candidate)
        self.assertEqual(original.count,out.count)
        spans=[(p['offset'],p['offset']+len(bytes.fromhex(p['old']))) for p in report['patches']]
        cursor=0
        for begin,end in sorted(spans):
            self.assertEqual(self.base[cursor:begin],candidate[cursor:begin])
            cursor=end
        self.assertEqual(self.base[cursor:],candidate[cursor:])
        self.assertEqual(out.read(0x02DEE8D0,46),original.read(0x02DEE8D0,46))
        self.assertEqual(out.read(0x057E2000,0x810),original.read(0x057E2000,0x810))
        self.assertEqual(out.read(0x057E3000,0x1000),original.read(0x057E3000,0x1000))
        self.assertTrue(report['exact_reversal'])
        self.assertEqual(report['changed_bytes'],92)
    def test_original_rtti_and_bindings(self):
        if self.original is None:self.skipTest('no local original EXE supplied')
        self.assertEqual(patch.digest(self.original),'9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69')
        pe=patch.Image(self.original)
        u32=lambda p:struct.unpack('<I',pe.read(p,4))[0]
        for vt,td,name in (
            (0x04DBD47C,0x054B6DBC,b'.?AVcFsmAction@fsm@game@app@@'),
            (0x04DC7AD8,0x054BC418,b'.?AVcFsmActionPcs@fsm@game@app@@'),
            (SUB_VT,0x054C5294,b'.?AVcFsmActionPcsSub@fsm@game@app@@'),
            (0x04E15EF4,0x054E1DCC,b'.?AVuPcsInput@pcs@game@app@@')):
            col=u32(vt-4)
            self.assertEqual(u32(col+12),td)
            self.assertEqual(pe.read(td+8,len(name)+1),name+b'\0')
        for start,thunk,binding in ((0x02976C30,0x01BBB149,0x029707B1),
                                    (0x029FF600,0x01C7E054,0x029FD2E7),
                                    (0x02DEE8D0,0x01C0758A,0x02DEE540)):
            self.assertEqual(pe.read(thunk,5),b'\xe9'+struct.pack('<i',start-thunk-5))
            self.assertEqual(pe.read(binding,5),b'\x68'+struct.pack('<I',thunk))
            self.assertEqual(pe.read(start,9),patch.PREFIX)
            self.assertEqual(pe.read(start+0x20,14),bytes.fromhex('894df833c05f5e5b8be55dc20400'))
        self.assertEqual(pe.read(0x02A75BEE,6),b'\xc7\x00'+struct.pack('<I',SUB_VT))
        self.assertEqual(pe.read(0x02DCFBE9,6),bytes.fromhex('898870100000'))
        self.assertEqual(pe.read(0x02DCFBF5,6),bytes.fromhex('898874100000'))
        self.assertEqual(pe.read(0x02DCFC01,6),bytes.fromhex('898878100000'))
        self.assertEqual(pe.read(0x02DD0549,3),bytes.fromhex('890c82'))
    def test_safe_cli_rejections(self):
        if self.base is None:self.skipTest('no local baseline supplied')
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);src=root/'input.exe';src.write_bytes(self.base)
            out=root/'output.exe';report=root/'report.json';script=Path(patch.__file__)
            cases=[(src,src,report),(src,out,src)]
            for i,(inp,output,rep) in enumerate(cases):
                run=subprocess.run([sys.executable,str(script),str(inp),str(output),'--report',str(rep)],capture_output=True)
                self.assertEqual(run.returncode,2)
                self.assertEqual(patch.digest(src.read_bytes()),patch.BASE_SHA)
                self.assertFalse(out.exists());self.assertFalse(report.exists())
            out.write_bytes(b'keep')
            run=subprocess.run([sys.executable,str(script),str(src),str(out),'--report',str(report)],capture_output=True)
            self.assertEqual(run.returncode,2);self.assertEqual(out.read_bytes(),b'keep')
            out.unlink();report.write_bytes(b'keep-report')
            run=subprocess.run([sys.executable,str(script),str(src),str(out),'--report',str(report)],capture_output=True)
            self.assertEqual(run.returncode,2);self.assertEqual(report.read_bytes(),b'keep-report');self.assertFalse(out.exists())
            report.unlink();bad=root/'bad.exe';bad.write_bytes(b'MZ')
            run=subprocess.run([sys.executable,str(script),str(bad),str(out),'--report',str(report)],capture_output=True)
            self.assertEqual(run.returncode,2);self.assertFalse(out.exists());self.assertFalse(report.exists())

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--base',type=Path);ap.add_argument('--candidate',type=Path)
    ap.add_argument('--original',type=Path);ap.add_argument('--report',type=Path)
    args=ap.parse_args()
    if bool(args.base)!=bool(args.candidate):ap.error('--base and --candidate must be paired')
    if args.base:
        Tests.base=args.base.read_bytes();Tests.candidate=args.candidate.read_bytes()
        image=patch.Image(Tests.candidate)
        Tests.code=staticmethod(image.read)
    if args.original:Tests.original=args.original.read_bytes()
    result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(Tests))
    summary={'tests':result.testsRun,'failures':len(result.failures),'errors':len(result.errors),
             'skipped':len(result.skipped),'success':result.wasSuccessful(),
             'guard_combinations':2592,'instructions_under_test':72,
             'candidate_bytes_used':bool(args.candidate),'gameplay_executed':False,
             'limited_interpreter_not_full_cpu':True}
    if args.report:
        with args.report.open('x') as f:json.dump(summary,f,indent=2);f.write('\n')
    raise SystemExit(not result.wasSuccessful())
