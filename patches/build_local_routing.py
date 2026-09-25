#!/usr/bin/env python3
"""Build the experimental January local-routing continuation from one exact base.

Never executes the game; requires GNU g++/as/ld for the 32-bit helper module.
The accepted base is the owner-corrected v14 candidate, not retail or an arbitrary
patched executable. Input/existing output files are never overwritten.
"""
from __future__ import annotations
import argparse
import array
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

BASE_SHA = 'e3c5c188782309a1683d27ade0ce9e38cf1c40219de9fb5d684b3d6fda2e285a'
BASE_SIZE = 60_748_800
ROOT = Path(__file__).resolve().parent / 'local_routing'
CODE_VA = 0x057E2000

def sha(data: bytes | bytearray) -> str:
    return hashlib.sha256(data).hexdigest()

def align(n: int, boundary: int) -> int:
    if boundary <= 0 or boundary & (boundary - 1):
        raise ValueError('invalid alignment')
    return (n + boundary - 1) & -boundary

class PE:
    def __init__(self, data: bytes | bytearray):
        self.data = data
        if len(data) < 0x100 or data[:2] != b'MZ':
            raise ValueError('not a PE image')
        self.pe = self.u32(0x3c)
        if self.pe + 24 > len(data) or data[self.pe:self.pe+4] != b'PE\0\0':
            raise ValueError('bad PE header')
        if self.u16(self.pe+4) != 0x14c:
            raise ValueError('expected i386')
        self.opt = self.pe+24
        if self.u16(self.opt) != 0x10b:
            raise ValueError('expected PE32')
        self.base = self.u32(self.opt+28)
        self.sa, self.fa = self.u32(self.opt+32), self.u32(self.opt+36)
        self.sh = self.opt+self.u16(self.pe+20)
        self.count = self.u16(self.pe+6)
        self.sections = []
        for i in range(self.count):
            pos = self.sh+i*40
            if pos+40 > len(data):
                raise ValueError('truncated section table')
            name = bytes(data[pos:pos+8]).rstrip(b'\0').decode('ascii')
            vs, rva, raw_size, raw = struct.unpack_from('<4I', data, pos+8)
            if raw_size and (raw < self.u32(self.opt+60) or raw+raw_size > len(data)):
                raise ValueError('invalid raw section extent')
            self.sections.append(dict(name=name, va=self.base+rva, virtual_size=vs,
                                      raw_size=raw_size, raw=raw, header=pos,
                                      characteristics=self.u32(pos+36)))
    def u16(self, off: int) -> int:
        return struct.unpack_from('<H', self.data, off)[0]
    def u32(self, off: int) -> int:
        return struct.unpack_from('<I', self.data, off)[0]
    def offset(self, va: int, size: int = 1) -> int:
        if size < 0:
            raise ValueError('negative read')
        for s in self.sections:
            delta = va-s['va']
            if delta >= 0 and delta+size <= s['raw_size']:
                return s['raw']+delta
        raise ValueError(f'VA not wholly file-backed: {va:#x} size={size}')
    def read(self, va: int, size: int) -> bytes:
        off = self.offset(va,size)
        return bytes(self.data[off:off+size])

def elf_module(path: Path) -> tuple[dict, dict]:
    """Read only the required ELF32 sections and symbols, without external packages."""
    data = path.read_bytes()
    if data[:7] != b'\x7fELF\x01\x01\x01' or struct.unpack_from('<H',data,18)[0] != 3:
        raise ValueError('helper must be little-endian ELF32 i386')
    shoff=struct.unpack_from('<I',data,32)[0]
    entsize,count,stridx=struct.unpack_from('<HHH',data,46)
    if entsize != 40 or shoff+count*40 > len(data):
        raise ValueError('bad helper section table')
    entries=[struct.unpack_from('<10I',data,shoff+i*40) for i in range(count)]
    names_entry=entries[stridx]
    names=data[names_entry[4]:names_entry[4]+names_entry[5]]
    result={}; symbols={}
    for e in entries:
        name=names[e[0]:].split(b'\0',1)[0].decode('ascii')
        if e[1] in (4,9) and e[5]:
            raise ValueError('unresolved relocations in linked module')
        if name in ('.lcfix','.lcdata'):
            blob=bytes(e[5]) if e[1]==8 else data[e[4]:e[4]+e[5]]
            if len(blob)!=e[5]: raise ValueError('truncated helper section')
            result[name]=(e[3],blob)
        if e[1]==2:
            strings_entry=entries[e[6]]
            strings=data[strings_entry[4]:strings_entry[4]+strings_entry[5]]
            for pos in range(e[4],e[4]+e[5],16):
                nm,val,size,info,other,index=struct.unpack_from('<IIIBBH',data,pos)
                name2=strings[nm:].split(b'\0',1)[0].decode('ascii')
                if name2 and index==0:
                    raise ValueError(f'unresolved symbol: {name2}')
                if name2: symbols[name2]=val
    if set(result)!={'.lcfix','.lcdata'} or result['.lcfix'][0]!=CODE_VA:
        raise ValueError('unexpected module layout')
    if any(result['.lcdata'][1]):
        raise ValueError('binding storage must initially be zero')
    return result,symbols

def compile_module(work: Path) -> tuple[dict,dict,dict]:
    sources={name:sha((ROOT/name).read_bytes()) for name in ('core.cpp','hooks.S','link.ld')}
    commands=[
        ['g++','-m32','-std=c++17','-Os','-ffreestanding','-fno-exceptions','-fno-rtti',
         '-fno-pic','-fno-pie','-fno-stack-protector','-fno-asynchronous-unwind-tables',
         '-fno-unwind-tables','-fno-builtin','-mno-sse','-mno-mmx',
         '-mpreferred-stack-boundary=2','-mincoming-stack-boundary=2',
         '-c',str(ROOT/'core.cpp'),'-o',str(work/'core.o')],
        ['as','--32',str(ROOT/'hooks.S'),'-o',str(work/'hooks.o')],
        ['ld','-m','elf_i386','-T',str(ROOT/'link.ld'),'-o',str(work/'routing.elf'),
         str(work/'core.o'),str(work/'hooks.o')]]
    for command in commands:
        subprocess.run(command,check=True,capture_output=True,text=True)
    module,symbols=elf_module(work/'routing.elf')
    provenance={'source_sha256':sources,'elf_sha256':sha((work/'routing.elf').read_bytes()),
                'compiler':subprocess.check_output(['g++','--version'],text=True).splitlines()[0],
                'commands':commands}
    return module,symbols,provenance

# Only these individual callsites change, never a global engine finder.
CALLS=[
 (0x025902C4,0x01C16468,'hook_begin','door: begin sensor selection'),
 (0x0259035D,0x01BDD159,'hook_availability','door: consider both actors availability'),
 (0x02590072,0x01C16468,'hook_wait_actor','door: selected WaitState actor'),
 (0x02590097,0x01BDD159,'hook_wait_allowed','door: preserve selection/eligibility agreement'),
 (0x025900B4,0x01C85962,'hook_wait_serial','door: propagate selected actor serial'),
 (0x02597D94,0x01C3C5F0,'hook_door_init','door: clear owner selection on initialization'),
 *[(v,0x01C07341,'lc_find_actor','door: exact serial fallback only at this callsite')
   for v in (0x02590DD3,0x02590FA0,0x0259681D,0x02596C02,0x02596F3F)],
 (0x0276D1BD,0x01C2C7F4,'hook_rescue_control','help: eligible opposite actor'),
 (0x0276D38E,0x01C2C7F4,'hook_rescue_control','help: same opposite actor supplies pad'),
 (0x02798F72,0x01BCA0C2,'hook_rescue_partner','help: same opposite actor supplies inventory'),
 (0x02798F9A,0x01C2C7F4,'hook_rescue_self','help: reciprocal inventory lookup'),
 (0x02706B37,0x01C7F26A,'hook_charge_group','action: actor-bound selector A'),
 (0x027088E9,0x01C7F26A,'hook_charge_group','action: actor-bound selector B'),
 (0x026E9B53,0x01C7F26A,'hook_component_group','component: delayed actor selector A'),
 (0x026EC5B3,0x01C7F26A,'hook_component_group','component: delayed actor selector B')]

def rel(src: int, dst: int, opcode: int=0xe8) -> bytes:
    return bytes([opcode])+struct.pack('<i',dst-(src+5))

def pe_checksum(data: bytes | bytearray, checksum_offset: int) -> int:
    copy=bytearray(data);struct.pack_into('<I',copy,checksum_offset,0)
    if len(copy)%2:copy.append(0)
    words=array.array('H');words.frombytes(copy)
    if sys.byteorder!='little':words.byteswap()
    total=sum(words)
    while total>>16:total=(total&0xffff)+(total>>16)
    return (total+len(data))&0xffffffff

def build(base: bytes, module: dict, symbols: dict) -> tuple[bytes,dict]:
    if len(base)!=BASE_SIZE or sha(base)!=BASE_SHA:
        raise ValueError('input is not the exact owner-corrected January v14 base')
    pe=PE(base)
    if (pe.base,pe.count,pe.sa,pe.fa)!=(0x400000,8,0x1000,0x200):
        raise ValueError('unexpected base PE layout')
    if pe.u16(pe.opt+70)&0x40 or not pe.u16(pe.pe+22)&1:
        raise ValueError('this fixed-address patch requires the original non-ASLR stripped-relocation image')
    if pe.u32(pe.opt+96+4*8) or pe.u32(pe.opt+96+5*8):
        raise ValueError('unexpected certificate/relocation table')
    if pe.sh+10*40>pe.u32(pe.opt+60) or any(base[pe.sh+8*40:pe.sh+10*40]):
        raise ValueError('insufficient unused section headers')
    original_end=max(s['raw']+s['raw_size'] for s in pe.sections)
    if original_end!=len(base) or pe.base+pe.u32(pe.opt+56)!=CODE_VA:
        raise ValueError('unexpected overlay or virtual extent')
    data=bytearray(base);patches=[];occupied=[]
    def patch_file(offset: int, old: bytes, new: bytes, label: str, va: int | None=None):
        if len(old)!=len(new) or bytes(data[offset:offset+len(old)])!=old:
            raise ValueError(f'expected-byte mismatch: {label} at {offset:#x}')
        if any(offset<b and offset+len(old)>a for a,b in occupied):
            raise ValueError(f'overlapping patch: {label}')
        occupied.append((offset,offset+len(old)));data[offset:offset+len(new)]=new
        patches.append(dict(label=label,offset=offset,va=va,old=old.hex(),new=new.hex()))
    def patch_va(va:int,old:bytes,new:bytes,label:str):
        patch_file(pe.offset(va,len(old)),old,new,label,va)
    for va,target,symbol,label in CALLS:
        patch_va(va,rel(va,target),rel(va,symbols[symbol]),label)
    patch_va(0x025903EE,bytes.fromhex('8b45b08b10'),rel(0x025903EE,symbols['hook_candidate'],0xe9),
             'door: exact eligible sensor candidates, preserve stock off')
    for va,old,new,label in [
        (0x02DF4F33,'81ec14010000','81ec1c010000','binder frame allocation'),
        (0x02DF4F3D,'8dbdecfeffff','8dbde4feffff','binder debug-fill start'),
        (0x02DF4F43,'b945000000','b947000000','binder debug-fill count'),
        (0x02DF505F,'81c414010000','81c41c010000','binder frame release')]:
        patch_va(va,bytes.fromhex(old),bytes.fromhex(new),label)
    for va,old,symbol in [
        (0x02DF4FA7,bytes.fromhex('e9b4c39aff9090'),'hook_bind_clear'),
        (0x02DF5015,bytes.fromhex('e976c39aff9090909090909090'),'hook_bind_actor')]:
        patch_va(va,old,rel(va,symbols[symbol],0xe9)+b'\x90'*(len(old)-5),symbol)
    for va,symbol,expected in [
        (0x027A27B0,'lc_actor_pad',bytes.fromhex('558bec81eccc000000')),
        (0x024605D0,'lc_item_member',bytes.fromhex('31c085c9741c')),
        (0x026D7AD0,'lc_model_member',bytes.fromhex('558bec81eccc000000'))]:
        # All entries have no branch into the replaced prefix in the audited ABI.
        patch_va(va,expected,rel(va,symbols[symbol],0xe9)+b'\x90'*(len(expected)-5),symbol)
    section_report=[]
    code_increment=data_increment=0
    for index,(name,flags) in enumerate([('.lcfix',0x60000020),('.lcdata',0xC0000040)],8):
        va,blob=module[name]; raw=align(len(data),pe.fa);size=align(len(blob),pe.fa)
        if not blob or va%pe.sa:raise ValueError('unaligned/empty helper section')
        data.extend(bytes(raw-len(data)));data.extend(blob);data.extend(bytes(size-len(blob)))
        header=struct.pack('<8s6I2HI',name.encode().ljust(8,b'\0'),len(blob),va-pe.base,
                           size,raw,0,0,0,0,flags)
        patch_file(pe.sh+index*40,bytes(40),header,'new section '+name)
        if name=='.lcfix':code_increment+=size
        else:data_increment+=size
        section_report.append(dict(name=name,va=va,raw=raw,raw_size=size,virtual_size=len(blob),sha256=sha(blob)))
    def h32(off:int,value:int,label:str):
        patch_file(off,base[off:off+4],struct.pack('<I',value),label)
    patch_file(pe.pe+6,struct.pack('<H',8),struct.pack('<H',10),'PE section count')
    h32(pe.opt+4,pe.u32(pe.opt+4)+code_increment,'PE SizeOfCode')
    h32(pe.opt+8,pe.u32(pe.opt+8)+data_increment,'PE SizeOfInitializedData')
    end=max(s['va']+s['virtual_size'] for s in section_report)
    h32(pe.opt+56,align(end-pe.base,pe.sa),'PE SizeOfImage')
    h32(pe.opt+64,pe_checksum(data,pe.opt+64),'PE checksum')
    # Validate final file extents, permissions and no simultaneous W+X sections.
    output=bytes(data);outpe=PE(output)
    for s in outpe.sections[-2:]:
        if s['characteristics']&0xA0000000==0xA0000000:raise ValueError('new section is W+X')
    if outpe.u32(pe.opt+64)!=pe_checksum(output,pe.opt+64):raise ValueError('checksum mismatch')
    restored=bytearray(output[:len(base)])
    for p in patches:restored[p['offset']:p['offset']+len(bytes.fromhex(p['old']))]=bytes.fromhex(p['old'])
    if bytes(restored)!=base:raise ValueError('reversal did not reproduce exact base')
    report={'status':'EXPERIMENTAL_BUILD_STRUCTURALLY_VERIFIED','input_sha256':BASE_SHA,
            'output_sha256':sha(output),'input_size':len(base),'output_size':len(output),
            'runtime_validated':False,'gameplay_executed':False,'win32_execution_tested':False,
            'fixed_image_base':hex(pe.base),'new_sections':section_report,'patches':patches,
            'symbols':{k:hex(v) for k,v in symbols.items() if k.startswith(('lc_','hook_'))},
            'changed_existing_bytes':sum(a!=b for a,b in zip(base,output)),
            'appended_bytes':len(output)-len(base),'exact_reversal_to_base':True,
            'limits':['No engine/game execution.','Sensor/callback scheduling and actor lifetime still need runtime testing.',
                      'No complete campaign, HUD, menu, forced-camera or checkpoint validation.',
                      'Uses a fixed-address January debug image, not retail.']}
    return output,report

def unused_paths(paths: list[Path]) -> None:
    resolved=[p.resolve() for p in paths]
    if len(set(resolved))!=len(resolved):raise ValueError('input/output/report paths must be different')
    for p in paths[1:]:
        if p.exists():raise ValueError(f'output already exists: {p}')
        if not p.parent.is_dir():raise ValueError(f'output directory does not exist: {p.parent}')

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('input',type=Path);ap.add_argument('output',type=Path)
    ap.add_argument('--report',type=Path,required=True)
    args=ap.parse_args()
    try:
        unused_paths([args.input,args.output,args.report])
        base=args.input.read_bytes()
        if len(base)!=BASE_SIZE or sha(base)!=BASE_SHA:raise ValueError('wrong input build/SHA-256')
        with tempfile.TemporaryDirectory(prefix='rev-local-routing-') as temp:
            module,symbols,provenance=compile_module(Path(temp))
            output,report=build(base,module,symbols)
            report['provenance']=provenance
        # Serialize before creating either destination. Exclusive create is race-safe.
        report_bytes=(json.dumps(report,indent=2)+'\n').encode()
        with args.output.open('xb') as handle:handle.write(output)
        try:
            with args.report.open('xb') as handle:handle.write(report_bytes)
        except Exception:
            args.output.unlink();raise
        print(json.dumps({k:report[k] for k in ('status','output_sha256','output_size','changed_existing_bytes','appended_bytes','runtime_validated')}))
        return 0
    except (OSError,ValueError,struct.error,subprocess.CalledProcessError) as exc:
        print(f'ERROR: {exc}',file=sys.stderr)
        if isinstance(exc,subprocess.CalledProcessError) and exc.stderr:print(exc.stderr,file=sys.stderr)
        return 2
if __name__=='__main__':raise SystemExit(main())
