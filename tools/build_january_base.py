#!/usr/bin/env python3
"""Rebuild the exact January base from the original and published mod sources.

Only reads the supplied game. Intermediates stay under a new output directory.
Does not launch a game, install into Steam, download tools or upload binaries.
"""
from pathlib import Path
import argparse
import ast
import contextlib
import hashlib
import io
import json
import re
import struct
import subprocess
import sys
from types import SimpleNamespace
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'patches'))
import build_pickup_owner_fix as pickup
import build_local_routing as routing
import build_script_serial_guard as scripts
SOURCE_SHA='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'

class LegacyBuild:
    def __init__(self,original,out,clang,linker):
        self.original=original.resolve();self.out=out.resolve()
        self.clang=clang;self.linker=linker;self.definitions={}
    def mapped(self,s):
        name=Path(s).name
        if name=='BioRevHD 30-Enero-2013.exe':return self.original
        if name=='rev1_local_coop_research':return self.out
        if name.endswith('.S'):return ROOT/'research/patches'/name
        return self.out/name
    def command(self,cmd,**kwargs):
        if cmd[0]=='as':
            src=next(Path(x) for x in cmd[1:] if x.endswith('.S'))
            obj=cmd[cmd.index('-o')+1];text=src.read_text();symbols=[]
            # LLVM does not accept an absolute numeric target as Intel call/jmp.
            # Keep it as an external relocation, resolved to the same VA by ld.
            def equ(m):
                symbols.append((m[1],m[2]));return '.extern '+m[1]
            text=re.sub(r'^\s*\.equ\s+(\w+),\s*(0x[0-9a-fA-F]+)\s*$',equ,text,flags=re.M)
            def absolute(m):
                name='_absolute_'+m[2];symbols.append((name,m[2]));return m[1]+' '+name
            text=re.sub(r'\b(call|jmp)\s+(0x[0-9a-fA-F]+)\b',absolute,text)
            adapted=self.out/(src.stem+'-llvm.S');adapted.write_text(text)
            self.definitions[obj]=symbols
            return subprocess.run([self.clang,'--target=i386-none-elf','-c',str(adapted),'-o',obj],**kwargs)
        if cmd[0]=='ld':
            extra=[]
            for obj in cmd:
                for name,value in self.definitions.get(obj,[]):extra+=['--defsym',name+'='+value]
            idx=cmd.index('-Ttext');address=cmd[idx+1]
            script=self.out/'helper.ld'
            # Preserve deliberately unaligned caves, e.g. camera_v4 at 0203E3BD.
            script.write_text('SECTIONS { .text '+address+' : SUBALIGN(1) { *(.text) } /DISCARD/ : { *(.note*) } }')
            args=cmd[1:idx]+['-T',str(script)]+cmd[idx+2:]
            return subprocess.run([self.linker,*args,*extra],**kwargs)
        if cmd[0]=='objcopy':
            if cmd[1:5]!=['-O','binary','-j','.text']:raise ValueError('unexpected objcopy request')
            d=Path(cmd[5]).read_bytes();off=struct.unpack_from('<I',d,32)[0]
            size,count,index=struct.unpack_from('<HHH',d,46)
            entries=[struct.unpack_from('<10I',d,off+i*size) for i in range(count)]
            n=entries[index];names=d[n[4]:n[4]+n[5]]
            e=next(e for e in entries if names[e[0]:].split(b'\0',1)[0]==b'.text')
            Path(cmd[6]).write_bytes(d[e[4]:e[4]+e[5]])
            return subprocess.CompletedProcess(cmd,0)
        raise ValueError('unexpected legacy tool: '+cmd[0])
    def assemble(self,source,va,name):
        obj=str(self.out/(name+'.o'));elf=str(self.out/(name+'.elf'));blob=str(self.out/(name+'.bin'))
        self.command(['as','--32',str(source),'-o',obj],check=True)
        self.command(['ld','-m','elf_i386','-Ttext',hex(va),'-o',elf,obj],check=True)
        self.command(['objcopy','-O','binary','-j','.text',elf,blob],check=True)
    def run(self,name):
        adapter=self
        class Relocate(ast.NodeTransformer):
            def visit_Constant(self,node):
                if isinstance(node.value,str) and node.value.startswith('/mnt/data'):
                    return ast.copy_location(ast.Constant(str(adapter.mapped(node.value))),node)
                return node
            def visit_Import(self,node):
                node.names=[n for n in node.names if n.name!='subprocess']
                return node if node.names else None
        path=ROOT/'patches'/name
        tree=Relocate().visit(ast.parse(path.read_text()));ast.fix_missing_locations(tree)
        output=io.StringIO()
        with contextlib.redirect_stdout(output):
            exec(compile(tree,str(path),'exec'),{'__file__':str(path),'__name__':'legacy_build',
                'subprocess':SimpleNamespace(run=self.command)})
        (self.out/(name+'.log')).write_text(output.getvalue())
        print(name+' PASS',flush=True)

def build(original,out,clang,linker):
    source=original.read_bytes()
    if len(source)!=60748800 or hashlib.sha256(source).hexdigest()!=SOURCE_SHA:
        raise ValueError('requires unmodified January 30 2013 with exact SHA256')
    out=out.resolve()
    if out.exists():raise ValueError('output directory must not exist')
    out.mkdir(parents=True)
    legacy=LegacyBuild(original,out,clang,linker)
    legacy.assemble(ROOT/'patches/camera_split_v4.S',0x0203E3BD,'camera_split_v4')
    legacy.assemble(ROOT/'research/patches/pickup_sub0_canonical_v11.S',0x01C95120,'pickup_sub0_v11')
    for name in ('build_v6_sub0_padmode.py','build_v7_fullpad.py','build_v9_clean_persistent_split.py',
                 'build_v10_itembox_from_v9.py','build_v11_sub0_pickup.py',
                 'build_v12_sub0_pickup_pad2.py','build_v13_ammo_relief.py'):
        legacy.run(name)
    v13=(out/'BioRevHD 30-Enero-2013 LOCAL COOP v13 SYMMETRIC AMMO RELIEF.exe').read_bytes()
    corrected,fix_report=pickup.build(v13)
    code=out/'routing-module';code.mkdir()
    module,symbols,provenance=routing.compile_module(code,clang,linker)
    local,routing_report=routing.build(corrected,module,symbols)
    final,script_report=scripts.build(local)
    target=out/'january-script-base.exe'
    with target.open('xb') as f:f.write(final)
    report={'source_sha256':SOURCE_SHA,'output_sha256':routing.sha(final),'gameplay_executed':False,
            'pickup':fix_report,'routing':routing_report,'compiler':provenance,'script':script_report,
            'recovered_pickup_source_commit':'7f7998fc2ab5e765fec243f3742971a7b8274145'}
    (out/'base-build.json').write_text(json.dumps(report,indent=2)+'\n')
    return target,report

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('original',type=Path);p.add_argument('output_directory',type=Path)
    p.add_argument('--clang',default='clang');p.add_argument('--linker',default='ld.lld');a=p.parse_args()
    target,report=build(a.original,a.output_directory,a.clang,a.linker)
    print(json.dumps({'output':str(target),'sha256':report['output_sha256'],'gameplay_executed':False}))
