#!/usr/bin/env python3
"""Execute only our 72-byte leaf selector in a synthetic Win32 process.

No game file is read, injected or executed. Engine services are not called.
An original test bridge records the actual registers and stack delta, then
restores the Python caller's nonvolatile registers. Requires 32-bit Windows.
"""
from __future__ import annotations
import argparse
import ctypes as C
import itertools
import json
import os
from pathlib import Path
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'patches'))
from build_script_serial_guard import HELPER, HELPER_SHA, HELPER_VA
from build_local_routing import sha

CODE=0x01C95000; GLOBALS=0x057D9000; OWNER=0x10000000
ACTOR=0x20000000; RESULT=0x30000000
ACTIVE=0x057D9188; TRACKER=0x057D9184; VT=0x04DCF41C
SEEDS={'ebx':0x11223344,'esi':0x55667788,'edi':0x1234FEDC,'ebp':0x76543210}

def bridge():
    d=bytearray(b'\x53\x56\x57\x55') # preserve Python's nonvolatiles
    d+=b'\x8B\x4C\x24\x14' # original owner argument -> ECX
    for opcode,name in ((0xBB,'ebx'),(0xBE,'esi'),(0xBF,'edi'),(0xBD,'ebp')):
        d+=bytes([opcode])+struct.pack('<I',SEEDS[name])
    d+=b'\x89\x25'+struct.pack('<I',RESULT+32) # save exact pre-argument ESP
    d+=b'\x6A\x00'
    d+=b'\xE8'+struct.pack('<i',HELPER_VA-(CODE+len(d)+5))
    # Capture EAX, ECX, EDX and each nonvolatile before restoring anything.
    for reg,off in ((0,0),(1,4),(2,8),(3,12),(6,16),(7,20),(5,24),(4,28)):
        d+=b'\x89'+bytes([(reg<<3)|5])+struct.pack('<I',RESULT+off)
    d+=b'\x8B\x25'+struct.pack('<I',RESULT+32)
    d+=b'\x5D\x5F\x5E\x5B\xC2\x04\x00'
    return bytes(d)

def run():
    if os.name!='nt' or C.sizeof(C.c_void_p)!=4:
        raise RuntimeError('requires a native Windows x86 Python interpreter')
    if len(HELPER)!=72 or sha(HELPER)!=HELPER_SHA:
        raise RuntimeError('selector identity mismatch')
    k=C.WinDLL('kernel32',use_last_error=True)
    k.VirtualAlloc.argtypes=[C.c_void_p,C.c_size_t,C.c_uint32,C.c_uint32]
    k.VirtualAlloc.restype=C.c_void_p
    k.VirtualFree.argtypes=[C.c_void_p,C.c_size_t,C.c_uint32];k.VirtualFree.restype=C.c_int
    k.VirtualProtect.argtypes=[C.c_void_p,C.c_size_t,C.c_uint32,C.POINTER(C.c_uint32)]
    k.VirtualProtect.restype=C.c_int
    k.GetCurrentProcess.restype=C.c_void_p
    k.FlushInstructionCache.argtypes=[C.c_void_p,C.c_void_p,C.c_size_t]
    k.FlushInstructionCache.restype=C.c_int
    regions=[]
    def require(ok,message):
        if not ok:raise RuntimeError(message+f'; WinError={C.get_last_error()}')
    def put(address,value):C.c_uint32.from_address(address).value=value&0xFFFFFFFF
    scenarios=0
    try:
        for address,size in ((CODE & ~0xFFFF,0x10000),(GLOBALS & ~0xFFFF,0x10000),(OWNER,0x2000),(ACTOR,0x2000),(RESULT,0x1000)):
            got=k.VirtualAlloc(address,size,0x3000,0x04) # committed RW, never RWX
            require(got==address,'could not allocate fixed synthetic region')
            regions.append(address)
        body=bridge();C.memmove(CODE,body,len(body));C.memmove(HELPER_VA,HELPER,len(HELPER))
        old=C.c_uint32()
        require(k.VirtualProtect(CODE,0x1000,0x20,C.byref(old)),'protect test code RX')
        require(k.FlushInstructionCache(k.GetCurrentProcess(),CODE,0x1000),'flush test code')
        call=C.WINFUNCTYPE(C.c_uint32,C.c_uint32)(CODE)
        cases=[]
        for local,mode,serial,bound in itertools.product((0,1,2),range(4),
                (0xFFFFFFFF,0,1,2,127,128,0x80000000),(0xFFFFFFFF,0,1,2,127,128,0x80000000)):
            cases.append(dict(local=local,mode=mode,serial=serial,bound=bound))
        cases.extend(({'owner':0},{'vt':0},{'vt':0x04DBD47C},{'vt':0x04DC7AD8},
                      {'vt':0x04E15EF4},{'tracker':0},{'context':0},{'context':0xDEADBEEF}))
        for case in cases:
            state=dict(owner=OWNER,vt=VT,local=1,mode=1,serial=1,bound=1,tracker=ACTOR,context=ACTOR)
            state.update(case)
            put(ACTIVE,state['local']);put(TRACKER,state['tracker']);put(OWNER,state['vt'])
            put(OWNER+0x1078,state['context']);put(OWNER+0x1074,state['bound'])
            put(ACTOR+0xE40,state['mode']);put(ACTOR+0xE3C,state['serial'])
            C.memset(RESULT,0xCC,36)
            returned=call(state['owner'])
            values=struct.unpack('<9I',C.string_at(RESULT,36))
            expected=int(bool(state['owner'] and state['vt']==VT and state['local'] and
                state['tracker'] and state['context']==state['tracker'] and state['mode']==1 and
                not state['serial']&0x80000000 and state['serial']==state['bound']))
            require(returned==expected and values[0]==expected,'wrong EAX '+str(state))
            require(values[1]==state['owner'],'ECX not preserved')
            require(values[3:7]==tuple(SEEDS[n] for n in ('ebx','esi','edi','ebp')),'nonvolatile register changed')
            require(values[7]==values[8],'incorrect ret 4 stack cleanup')
            scenarios+=1
        return dict(status='PASS',native_win32_executed=True,gameplay_executed=False,
            engine_code_executed=False,selector_scenarios=scenarios,helper_sha256=HELPER_SHA,
            bridge_sha256=sha(body),python_pointer_bits=32,
            checks=['actual EAX','actual ECX','actual EBX/ESI/EDI/EBP','actual ESP cleanup'],
            method='ctypes invokes original test bridge and original leaf selector in synthetic Win32 memory')
    finally:
        for address in reversed(regions):k.VirtualFree(address,0,0x8000)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    try:report=run()
    except Exception as exc:
        report=dict(status='FAIL',error=str(exc),gameplay_executed=False)
    with args.report.open('x',encoding='utf-8') as f:json.dump(report,f,indent=2);f.write('\n')
    print(json.dumps(report))
    raise SystemExit(0 if report['status']=='PASS' else 1)
