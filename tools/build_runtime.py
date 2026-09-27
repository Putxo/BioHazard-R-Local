#!/usr/bin/env python3
"""Link the full freestanding x86 module. Does not open, patch or run a game.

Use LLVM clang and ld.lld (Linux or Windows). Only original mod code is emitted.
"""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

ROOT=Path(__file__).resolve().parents[1]

def sources():
    return sorted((ROOT/'patches/hud_ownership').glob('*.cpp')) + sorted(
        (ROOT/'patches/hud_ownership').glob('*.S')) + [
        ROOT/'patches/menu_routing/menu_owner.cpp',
        ROOT/'patches/menu_routing/menu_gateways.S',
        ROOT/'patches/runtime/runtime.cpp', ROOT/'patches/runtime/native_host.cpp',
        ROOT/'patches/runtime/entry.S']

def build(out,clang='clang',linker='ld.lld'):
    out=out.resolve()
    out.mkdir(parents=True,exist_ok=True)
    if any(out.iterdir()):
        raise ValueError('output directory must be empty (no overwrite)')
    objects=[]
    hashes={}
    for i,source in enumerate(sources()):
        obj=out/f'{i:02d}-{source.stem}.o'
        cmd=[clang,'--target=i386-none-elf','-c',str(source),'-o',str(obj)]
        if source.suffix=='.cpp':
            cmd += ['-std=c++17','-Os','-Wall','-Wextra','-Werror',
                    '-ffreestanding','-fno-exceptions','-fno-rtti','-fno-builtin',
                    '-fno-stack-protector','-fno-pic','-fno-pie','-fno-threadsafe-statics',
                    '-fno-asynchronous-unwind-tables','-fno-unwind-tables',
                    '-mno-sse','-mno-mmx','-mstack-alignment=4']
        subprocess.run(cmd,check=True,cwd=ROOT)
        objects.append(str(obj))
        hashes[source.relative_to(ROOT).as_posix()]=hashlib.sha256(source.read_bytes()).hexdigest()
    target=out/'runtime.elf'
    subprocess.run([linker,'-m','elf_i386','--no-undefined','--fatal-warnings',
        '-T','patches/runtime/link.ld','-o',str(target),*objects],check=True,cwd=ROOT)
    report={'format':'rev-runtime-build-v1','gameplay_executed':False,
            'source_sha256':hashes,'elf_sha256':hashlib.sha256(target.read_bytes()).hexdigest(),
            'compiler':subprocess.check_output([clang,'--version'],text=True).splitlines()[0],
            'linker':subprocess.check_output([linker,'--version'],text=True).splitlines()[0]}
    (out/'build.json').write_text(json.dumps(report,indent=2)+'\n')
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('output',type=Path);p.add_argument('--clang',default='clang')
    p.add_argument('--linker',default='ld.lld');args=p.parse_args()
    print(json.dumps(build(args.output,args.clang,args.linker),indent=2))
