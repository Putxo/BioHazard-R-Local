#!/usr/bin/env python3
"""Interpret only new assembly bridges, stubbing all engine/C++ calls.

This does not emulate the game, Windows, the C++ module, devices or rendering.
It checks bridge argument transport, stack cleanup, jumps and binder snapshots
using the actual machine-code bytes read from the generated PE.
"""
from __future__ import annotations
import argparse, importlib.util, json, struct
from pathlib import Path
SPEC=importlib.util.spec_from_file_location('builder',Path(__file__).parents[1]/'patches/build_local_routing.py')
M=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(M)
REGS=('eax','ecx','edx','ebx','esp','ebp','esi','edi')
ACTIVE=0x057D9188;SUB=0x057D9184;SENTINEL=0xEEEF0000
CHECKS=0

def check(condition,message='assertion'):
    global CHECKS
    CHECKS+=1
    if not condition:raise AssertionError(message)

class Machine:
    def __init__(self,pe,symbols):
        self.pe=pe;self.symbols=symbols;self.mem={};self.stubs={};self.calls=[];self.zf=False;self.ip=0
        self.r={k:0x100+i*0x100 for i,k in enumerate(REGS)}
        self.r['esp']=0x100000;self.r['ebp']=0x110000
    def write(self,address,value,size=4):
        for i in range(size):self.mem[(address+i)&0xffffffff]=(value>>(8*i))&255
    def read(self,address,size=4):
        value=0
        for i in range(size):
            at=(address+i)&0xffffffff
            if at not in self.mem:raise AssertionError(f'unmapped bridge read {at:#x}')
            value|=self.mem[at]<<(8*i)
        return value
    def push(self,value):
        self.r['esp']-=4;self.write(self.r['esp'],value)
    def pop(self):
        value=self.read(self.r['esp']);self.r['esp']+=4;return value
    def fetch(self,n):
        result=self.pe.read(self.ip,n);self.ip+=n;return result
    def imm(self,n,signed=False):return int.from_bytes(self.fetch(n),'little',signed=signed)
    def modrm(self):
        b=self.imm(1);mod=b>>6;reg=(b>>3)&7;rm=b&7
        if mod==3:return reg,('reg',REGS[rm])
        if rm==4:
            sib=self.imm(1);scale=1<<(sib>>6);idx=(sib>>3)&7;base=sib&7
            value=0 if idx==4 else self.r[REGS[idx]]*scale
            if base==5 and mod==0:value+=self.imm(4)
            else:value+=self.r[REGS[base]]
        elif rm==5 and mod==0:value=self.imm(4)
        else:value=self.r[REGS[rm]]
        if mod==1:value+=self.imm(1,True)
        if mod==2:value+=self.imm(4,True)
        return reg,('mem',value&0xffffffff)
    def get(self,operand,size=4):
        kind,key=operand
        return (self.r[key]&((1<<(8*size))-1)) if kind=='reg' else self.read(key,size)
    def put(self,operand,value,size=4):
        kind,key=operand;mask=(1<<(size*8))-1
        if kind=='reg':self.r[key]=(self.r[key]&~mask)|(value&mask)
        else:self.write(key,value,size)
    def stub(self,name,nargs=0,cleanup=0,expected=None,result=0x12345678,ecx=None,callback=None):
        address=self.symbols[name] if isinstance(name,str) else name
        def fn():
            args=[self.read(self.r['esp']+4+4*i) for i in range(nargs)]
            if expected is not None:check(args==list(expected),f'{name}: args {args} expected {expected}')
            if ecx is not None:check(self.r['ecx']==ecx,f'{name}: ecx')
            self.calls.append((address,args))
            returned=callback(self,args) if callback else result
            self.r['eax']=returned&0xffffffff;self.r['ecx']=0xC10BBE12;self.r['edx']=0xD10BBE12
            self.ip=self.pop();self.r['esp']+=cleanup
        self.stubs[address]=fn
    def run(self,name,call=True,limit=1000):
        startsp=self.r['esp'];old={k:self.r[k] for k in ('ebx','ebp','esi','edi')}
        if call:self.push(SENTINEL)
        self.ip=self.symbols[name]
        stops={SENTINEL,0x02590429,0x025903F3,0x02DF4FAE,0x02DF5022}
        for step in range(limit):
            if self.ip in stops:
                for k,v in old.items():check(self.r[k]==v,f'{name} changed {k}')
                return startsp,self.ip
            if self.ip in self.stubs:self.stubs[self.ip]();continue
            oldip=self.ip;opcode=self.imm(1)
            if 0x50<=opcode<=0x57:self.push(self.r[REGS[opcode-0x50]])
            elif 0x58<=opcode<=0x5f:self.r[REGS[opcode-0x58]]=self.pop()
            elif opcode in (0x6a,0x68):self.push(self.imm(1 if opcode==0x6a else 4,opcode==0x6a))
            elif opcode==0xff:
                reg,operand=self.modrm()
                if reg!=6:raise AssertionError('only PUSH FF supported')
                self.push(self.get(operand))
            elif opcode in (0x8b,0x89):
                reg,operand=self.modrm();regop=('reg',REGS[reg])
                if opcode==0x8b:self.put(regop,self.get(operand))
                else:self.put(operand,self.get(regop))
            elif opcode==0xa3:self.write(self.imm(4),self.r['eax'])
            elif opcode in (0xc6,0xc7):
                reg,operand=self.modrm();check(reg==0,'MOV extension');size=1 if opcode==0xc6 else 4
                value=self.imm(size);self.put(operand,value,size)
            elif opcode in (0x81,0x83):
                reg,operand=self.modrm();value=self.imm(4 if opcode==0x81 else 1,opcode==0x83)
                left=self.get(operand)
                if reg==7:self.zf=(left==(value&0xffffffff))
                elif reg==0:self.put(operand,left+value)
                else:raise AssertionError(f'unsupported immediate op {reg}')
            elif opcode==0x3b:
                reg,operand=self.modrm();self.zf=(self.r[REGS[reg]]==self.get(operand))
            elif opcode in (0x85,0x84):
                reg,operand=self.modrm();size=1 if opcode==0x84 else 4
                self.zf=(self.get(('reg',REGS[reg]),size)&self.get(operand,size))==0
            elif opcode in (0x74,0x75):
                displacement=self.imm(1,True)
                if self.zf==(opcode==0x74):self.ip+=displacement
            elif opcode in (0xe8,0xe9,0xeb):
                displacement=self.imm(1 if opcode==0xeb else 4,True);target=self.ip+displacement
                if opcode==0xe8:self.push(self.ip)
                self.ip=target
            elif opcode in (0xc3,0xc2):
                cleanup=self.imm(2) if opcode==0xc2 else 0
                self.ip=self.pop();self.r['esp']+=cleanup
            else:raise AssertionError(f'unsupported instruction {opcode:#x} at {oldip:#x}; no implicit pass')
        raise AssertionError('instruction budget exceeded')

def tests(pe,symbols):
    cases=0
    def fresh():return Machine(pe,symbols)
    for hook,target in [('hook_begin','lc_begin'),('hook_wait_actor','lc_wait_actor')]:
        c=fresh();c.write(c.r['ebp']-0x14,0x4560);c.stub(target,1,expected=[0x4560])
        sp,stop=c.run(hook);check(c.r['esp']==sp);check(c.r['eax']==0x12345678);cases+=1
    c=fresh();manager=c.r['ecx'];c.write(c.r['ebp']-0x20,0x7890)
    c.stub('lc_wait_serial',2,expected=[manager,0x7890],result=1)
    sp,stop=c.run('hook_wait_serial');check(c.r['esp']==sp);check(c.r['eax']==1);cases+=1
    for hook,target in [('hook_availability','lc_availability'),('hook_wait_allowed','lc_wait_allowed')]:
        c=fresh();actor=c.r['ecx'];c.write(c.r['ebp']-0x14,0x7770);c.push(3)
        c.stub(target,3,expected=[0x7770,actor,3],result=1)
        sp,stop=c.run(hook);check(c.r['esp']==sp+4);check(c.r['eax']==1);cases+=1
    for active,accepted in [(False,False),(True,False),(True,True)]:
        c=fresh();c.write(c.r['ebp']-0x14,0x5000);c.write(c.r['ebp']-0x50,0x6000);c.write(0x6000,0xA000)
        c.write(c.r['ebp']-0x35,0,1);c.stub('lc_is_managed',1,expected=[0x5000],result=int(active))
        c.stub('lc_consider',2,expected=[0x5000,0x6000],result=int(accepted))
        sp,stop=c.run('hook_candidate',False);check(c.r['esp']==sp)
        check(stop==(0x02590429 if active else 0x025903F3));check(c.read(c.r['ebp']-0x35,1)==int(active and accepted))
        if not active:check(c.r['eax']==0x6000 and c.r['edx']==0xA000)
        cases+=1
    c=fresh();owner=c.r['ecx'];c.push(33);c.push(22);c.push(11)
    c.stub(0x01C3C5F0,3,12,expected=[11,22,33],ecx=owner,result=0xCAFEBABE)
    c.stub('lc_reset_owner',1,expected=[owner])
    sp,stop=c.run('hook_door_init');check(c.r['esp']==sp+12);check(c.r['eax']==0xCAFEBABE);cases+=1
    for hook,lookup,core in [('hook_rescue_control',0x01C2C7F4,'lc_rescue_control'),
                            ('hook_rescue_partner',0x01BCA0C2,'lc_rescue_other'),
                            ('hook_rescue_self',0x01C2C7F4,'lc_rescue_other')]:
        c=fresh();manager=c.r['ecx'];c.write(c.r['ebp']-8,0x5566);c.push(0x11223344)
        c.stub(lookup,1,4,expected=[0x11223344],ecx=manager,result=0x6655)
        c.stub(core,2,expected=[0x5566,0x6655],result=0xABCDE)
        sp,stop=c.run(hook);check(c.r['esp']==sp+4);check(c.r['eax']==0xABCDE);cases+=1
    for hook,core,off in [('hook_charge_group','lc_install_actor_group',-0x14),
                          ('hook_component_group','lc_install_component_group',8)]:
        c=fresh();command=c.r['ecx'];c.write(c.r['ebp']+off,0x4520);c.push(0x12340)
        c.stub(core,3,expected=[command,0x12340,0x4520],result=command)
        sp,stop=c.run(hook);check(c.r['esp']==sp+4);check(c.r['eax']==command);cases+=1
    # Full clear/rebind bridge pair. No game code runs: conversion/ensure are stubs.
    for subtype,mode,prior_active,old_tracker,old_bound,new_actor,expected in [
        (True,2,0,0,0,0x6000,1), (True,1,1,0x6000,0x6000,0x6000,1),
        (True,1,0,0x6000,0x6000,0x6000,0), (True,1,1,0x7000,0x6000,0x6000,0),
        (True,1,1,0x6000,0x7000,0x6000,0), (True,3,1,0x6000,0x6000,0x6000,0),
        (True,0,1,0x6000,0x6000,0x6000,0), (True,0,1,0x6000,0x6000,0,0),
        (False,1,1,0x7000,0x7000,0x6000,1)]:
        c=fresh();owner=0x5000;c.write(owner,0x04E1649C if subtype else 0x04E1642C);c.write(owner+0x44,old_bound)
        c.write(c.r['ebp']-8,owner);c.write(c.r['ebp']-0x20,old_bound);c.write(ACTIVE,prior_active);c.write(SUB,old_tracker)
        c.r['eax']=owner;sp,stop=c.run('hook_bind_clear',False)
        check(stop==0x02DF4FAE and c.r['esp']==sp);check(c.read(owner+0x44)==0)
        if subtype:
            check(c.read(ACTIVE)==0 and c.read(SUB)==0)
            check(c.read(c.r['ebp']-0x118)==prior_active and c.read(c.r['ebp']-0x11c)==old_tracker)
        c.r['eax']=0xAB00
        if new_actor:c.write(new_actor+0xE40,mode)
        c.stub(0x01BEE332,ecx=0xAB00,result=new_actor)
        c.stub('lc_forget_all')
        def conversion(vm,args):vm.write(new_actor+0xE40,1);return new_actor
        c.stub(0x01BB8B60,1,4,expected=[1],ecx=new_actor,callback=conversion)
        c.stub(0x01C9504C)
        sp,stop=c.run('hook_bind_actor',False);check(stop==0x02DF5022 and c.r['esp']==sp)
        check(c.read(owner+0x44)==new_actor);check(c.read(ACTIVE)==expected)
        check(c.read(SUB)==(new_actor if subtype else old_tracker))
        conversions=sum(a==0x01BB8B60 for a,_ in c.calls)
        check(conversions==int(subtype and mode==2 and bool(new_actor)))
        cameras=sum(a==0x01C9504C for a,_ in c.calls)
        check(cameras==int(subtype and bool(expected)))
        cases+=1
    return cases

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('candidate',type=Path);ap.add_argument('manifest',type=Path);ap.add_argument('--report',type=Path)
    args=ap.parse_args();data=args.candidate.read_bytes();manifest=json.loads(args.manifest.read_text())
    check(M.sha(data)==manifest['output_sha256'],'candidate hash differs from manifest')
    pe=M.PE(data);symbols={k:int(v,16) for k,v in manifest['symbols'].items()}
    cases=tests(pe,symbols)
    result={'status':'PASS','bridge_scenarios':cases,'assertions':CHECKS,'candidate_sha256':M.sha(data),
            'engine_executed':False,'cpp_calls_stubbed':True,'model':'restricted new-bridge instruction interpreter; not whole-game emulation'}
    if args.report:
        if args.report.resolve() in (args.candidate.resolve(),args.manifest.resolve()):raise ValueError('refuse input overwrite')
        with args.report.open('x') as out:json.dump(result,out,indent=2)
    print(json.dumps(result))
if __name__=='__main__':main()
