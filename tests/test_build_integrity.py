#!/usr/bin/env python3
"""File-level and CLI rejection checks; no game execution."""
from __future__ import annotations
import argparse, importlib.util, json, subprocess, sys, tempfile
from pathlib import Path
import struct
ROOT=Path(__file__).parents[1]
sp=importlib.util.spec_from_file_location('builder',ROOT/'patches/build_local_routing.py')
M=importlib.util.module_from_spec(sp);sp.loader.exec_module(M)

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('base',type=Path);ap.add_argument('candidate',type=Path);ap.add_argument('manifest',type=Path);ap.add_argument('--report',required=True,type=Path)
    a=ap.parse_args();base=a.base.read_bytes();candidate=a.candidate.read_bytes();report=json.loads(a.manifest.read_text())
    checks=0
    def check(value,msg):
        nonlocal checks
        checks+=1
        if not value:raise AssertionError(msg)
    check(M.sha(base)==M.BASE_SHA,'base hash');check(M.sha(candidate)==report['output_sha256'],'candidate hash')
    p=M.PE(base);q=M.PE(candidate)
    patched=bytearray(base)
    touched=[]
    for patch in report['patches']:
        off=patch['offset'];old=bytes.fromhex(patch['old']);new=bytes.fromhex(patch['new'])
        check(base[off:off+len(old)]==old,patch['label']+' original')
        check(candidate[off:off+len(new)]==new,patch['label']+' installed')
        touched.append((off,off+len(new)));patched[off:off+len(new)]=new
    check(patched==candidate[:len(base)],'exclusive declared changes')
    reverse=bytearray(candidate[:len(base)])
    for patch in report['patches']:
        old=bytes.fromhex(patch['old']);reverse[patch['offset']:patch['offset']+len(old)]=old
    check(reverse==base,'exact reverse')
    check(q.count==10,'section count');check(q.u32(q.opt+64)==M.pe_checksum(candidate,q.opt+64),'checksum')
    check(p.u32(p.opt+16)==q.u32(q.opt+16),'entrypoint')
    for i in range(16):
        check(p.data[p.opt+96+i*8:p.opt+104+i*8]==q.data[q.opt+96+i*8:q.opt+104+i*8],f'directory {i} preserved')
    for name,va,n in [('v14 door input',0x02591118,14),('v14 helper',0x01C95300,35),
                       ('coop item wrapper',0x01C95100,20),('ammo helper',0x01C95220,186),
                       ('pickup body',0x024646F0,0x2d0),('actor global finder',0x02DA3840,0x110),
                       ('global local index',0x02DA3050,0x40),('global isPlayer',0x01D77EB0,0x50),
                       ('action getter',0x02DB2B50,0x70),('main camera init',0x0203E3A9,0xBB)]:
        check(p.read(va,n)==q.read(va,n),name+' preserved')
    for va,target,sym,label in M.CALLS:
        got=q.read(va,5);check(got[0]==0xE8 and va+5+struct.unpack('<i',got[1:])[0]==int(report['symbols'][sym],16),label+' destination')
    negatives=[]
    with tempfile.TemporaryDirectory(prefix='rev-reject-') as temp:
        d=Path(temp);bad=d/'bad.exe';bad.write_bytes(b'MZ'+bytes(100))
        exists=d/'existing.exe';exists.write_bytes(b'preserve me')
        exists_report=d/'exists.json';exists_report.write_text('preserve report')
        cases=[('input_equals_output',a.base,a.base,d/'r1.json'),
               ('report_equals_input',a.base,d/'o2.exe',a.base),
               ('existing_output',a.base,exists,d/'r3.json'),
               ('existing_report',a.base,d/'o4.exe',exists_report),
               ('output_equals_report',a.base,d/'both',d/'both'),
               ('wrong_input',bad,d/'o6.exe',d/'r6.json')]
        symlink=d/'alias.exe';symlink.symlink_to(a.base.resolve())
        cases.append(('symlink_input_alias',a.base,symlink,d/'r7.json'))
        for name,src,out,rpt in cases:
            before={path:M.sha(path.read_bytes()) for path in (src,out,rpt) if path.exists()}
            proc=subprocess.run([sys.executable,str(ROOT/'patches/build_local_routing.py'),str(src),str(out),'--report',str(rpt)],capture_output=True,text=True)
            check(proc.returncode==2,name+' rejection status')
            for path,hash_before in before.items():check(M.sha(path.read_bytes())==hash_before,name+' file unchanged')
            for path in (out,rpt):
                if path not in before:check(not path.exists(),name+' no partial output')
            negatives.append(name)
        module,syms,prov=M.compile_module(d)
        rebuilt,r2=M.build(base,module,syms)
        check(rebuilt==candidate,'independent recompile/rebuild byte identity')
        # Reject a corrupt accepted base even when all hook locations are untouched.
        for offset in (0x1000,len(base)-1):
            corrupt=bytearray(base);corrupt[offset]^=1
            try:M.build(bytes(corrupt),module,syms)
            except ValueError:check(True,'corrupt base refused')
            else:check(False,'corrupt base accepted')
    result={'status':'PASS','assertions':checks,'cli_negative_scenarios':negatives,
            'deterministic_rebuild':True,'reverse_to_base':True,'candidate_sha256':M.sha(candidate),'gameplay_executed':False}
    if a.report.resolve() in (a.base.resolve(),a.candidate.resolve(),a.manifest.resolve()):raise ValueError('refuse input overwrite')
    with a.report.open('x') as h:json.dump(result,h,indent=2)
    print(json.dumps(result))
if __name__=='__main__':main()
