#!/usr/bin/env python3
"""Read-only identity, instruction and control-flow witnesses. Never builds a game.

The branch witness specifically prevents reintroducing the old begin hook after
PCS+50's early exit. This checks selected instructions, not a whole-program proof.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import sys

SHA = '9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
SIZE = 60_748_800
SITES = (
    ('npc_born',0x0277D4FB,'8b45f85f5e5b',0),
    ('cockpit_born',0x02B47907,'8b45f85f5e5b',0),
    ('minimap_born',0x02B67F51,'8b45f85f5e5b',0),
    ('npc_dying',0x0277DC70,'558bec81ecd8000000',1),
    ('player_dying',0x027B5D00,'558bec81eccc000000',1),
    ('cockpit_dying',0x02B47A30,'558bec81ecc8020000',1),
    ('minimap_dying',0x02B68040,'558bec81ecf4000000',1),
    ('main_dying',0x02DF5970,'558bec81eccc000000',4),
    ('sub_dying',0x02DF5F90,'558bec81eccc000000',4),
    ('bind_begin',0x02DF4F65,'8b45f80fb64850',2),
    ('bind_end',0x02DF504B,'528bcd508d157050df02',3),
)
RTTI = (
    (0x04D9B25C,0x05354D88,0x0547B534,b'.?AVuNpc@chara@game@app@@\0'),
    (0x04D9CDF4,0x05354E00,0x054A6AD8,b'.?AVuPlayer@chara@game@app@@\0'),
    (0x04DE5C14,0x0537C8E8,0x054CEFA4,b'.?AVuCockpitManagerMain@gui@game@app@@\0'),
    (0x04DE86FC,0x0537D3F4,0x054D0D50,b'.?AVuMiniMapManager@gui@game@app@@\0'),
    (0x04E1642C,0x0538C48C,0x054E20FC,b'.?AVuPcsPlayerMain@pcs@game@app@@\0'),
    (0x04E1649C,0x0538C4F8,0x054E2130,b'.?AVuPcsPlayerSub0@pcs@game@app@@\0'),
)

def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

class Image:
    def __init__(self, data: bytes):
        if len(data) != SIZE or sha(data) != SHA:
            raise ValueError('requires exact unmodified January executable')
        self.data = data
        pe = struct.unpack_from('<I',data,0x3C)[0]
        if data[:2] != b'MZ' or data[pe:pe+4] != b'PE\0\0':
            raise ValueError('invalid PE signature')
        opt=pe+24
        if struct.unpack_from('<H',data,pe+4)[0] != 0x14C or struct.unpack_from('<H',data,opt)[0] != 0x10B:
            raise ValueError('requires PE32 i386')
        self.base=struct.unpack_from('<I',data,opt+28)[0]
        count=struct.unpack_from('<H',data,pe+6)[0]
        table=opt+struct.unpack_from('<H',data,pe+20)[0]
        self.sections=[]
        for i in range(count):
            o=table+i*40
            vs,rva,n,raw=struct.unpack_from('<4I',data,o+8)
            if raw+n>len(data): raise ValueError('section outside file')
            self.sections.append((self.base+rva,n,raw))
    def read(self, address: int, size: int) -> bytes:
        if size<0: raise ValueError('negative size')
        for base,n,raw in self.sections:
            if base<=address and address+size<=base+n:
                o=raw+address-base
                return self.data[o:o+size]
        raise ValueError('address is not raw-backed: '+hex(address))

def source_map(root: Path) -> dict:
    folder=root/'patches/hud_ownership'
    asm=(folder/'lifetime_gateways.S').read_text()
    ld=(folder/'lifetime_continuations.ld').read_text()
    cpp=(folder/'lifetime_source.cpp').read_text()
    declarations={m[0]:(int(m[1],16),int(m[2])) for m in re.findall(
        r'^lifetime_gateway\s+(\w+),\s*(0x[0-9A-Fa-f]+),\s*(\d+)',asm,re.M)}
    continuations={name:int(value,16) for name,value in re.findall(
        r'rev_life_continue_(\w+)\s*=\s*(0x[0-9A-Fa-f]+);',ld)}
    events={(int(a,16),int(e)) for a,e in re.findall(r'\{(0x[0-9A-Fa-f]+),(\d+),LifeObject::',cpp)}
    if len(declarations)!=11 or len(continuations)!=11 or len(events)!=11:
        raise ValueError('unexpected observer source-map cardinality')
    for name,address,body,event in SITES:
        shape=0 if event==0 else 1 if event in (1,4) else event
        if declarations.get(name)!=(address,shape) or continuations.get(name)!=address+len(bytes.fromhex(body)) or (address,event) not in events:
            raise ValueError('observer/gateway/continuation mismatch: '+name)
    if not re.search(r'rev_life_binder_debug_table\s*=\s*0x02DF5070;',ld,re.I):
        raise ValueError('debug table mismatch')
    if 'movzx ecx,byte ptr [eax+0x50]' not in asm:
        raise ValueError('missing displaced byte-load replay')
    return {'status':'PASS_SOURCE_MAP','sites':len(SITES)}

def audit(data: bytes) -> dict:
    pe=Image(data); witnesses=[]
    def exact(address, expected, label):
        if pe.read(address,len(expected))!=expected:
            raise ValueError('witness differs: '+label)
        witnesses.append({'va':hex(address),'size':len(expected),'label':label,'sha256':sha(expected)})
    def ptr(address,value,label):exact(address,struct.pack('<I',value),label)
    def rel(address,target,opcode,label):exact(address,bytes([opcode])+struct.pack('<i',target-address-5),label)
    for name,address,body,event in SITES:
        exact(address,bytes.fromhex(body),'gateway '+name)
    for vt,col,td,name in RTTI:
        ptr(vt-4,col,'RTTI complete object locator')
        ptr(col+12,td,'RTTI type descriptor')
        exact(td+8,name,'RTTI '+name.decode().rstrip('\0'))
    for address,vt in ((0x0277C06E,0x04D9B25C),(0x027B579E,0x04D9CDF4),(0x02B477CE,0x04DE5C14),(0x02B67F2E,0x04DE86FC),(0x02DF588E,0x04E1642C),(0x02DF5EAE,0x04E1649C)):
        exact(address,b'\xC7\x00'+struct.pack('<I',vt),'constructor installs vtable')
    rel(0x027B5D61,0x01C3F4F8,0xE8,'derived destructor calls base thunk')
    rel(0x01C3F4F8,0x0277DC70,0xE9,'base destructor thunk')
    rel(0x01C16468,0x026D7EF0,0xE9,'cdecl Self getter thunk')
    exact(0x026D7F46,b'\xC3','Self getter no stack arguments')
    exact(0x02DF4F6C,bytes.fromhex('85c90f85cf000000'),'early-exit test and forward JNE')
    branch=pe.read(0x02DF4F6E,6)
    destination=0x02DF4F74+struct.unpack('<i',branch[2:])[0]
    if destination!=0x02DF5043: raise ValueError('early-exit destination changed')
    exact(destination,bytes.fromhex('8b4df8e8ce03e7fe'),'early exit rejoins before END')
    # Existing local-routing hooks remain separate, not silently overwritten.
    inherited=((0x02DF4FA7,7),(0x02DF5015,13))
    for _,address,body,_ in SITES:
        end=address+len(bytes.fromhex(body))
        for old,n in inherited:
            if address<old+n and old<end: raise ValueError('overlap with inherited binder hooks')
    exact(0x03268CB8,bytes.fromhex('7412'),'simulation pause bypass')
    exact(0x03268CCA,bytes.fromhex('eb73'),'paused path skips counter increment')
    exact(0x03268CCF,bytes.fromhex('8b9148d8330083c201'),'simulation counter low increment')
    exact(0x03268CD8,bytes.fromhex('8b814cd8330083d000'),'simulation counter carry')
    exact(0x03479BD1,bytes.fromhex('8b8048d83300'),'simulation counter getter')
    return {'status':'PASS_STATIC_WITNESSES','source_sha256':SHA,'source_size':SIZE,
        'witnesses':witnesses,'checks':len(witnesses),'sites':len(SITES),
        'begin_site':'0x02df4f65','old_begin_site_is_unsafe':'0x02df4f99',
        'early_exit_destination':hex(destination),'inherited_binder_hooks_disjoint':True,
        'hooks_installed':False,'gameplay_executed':False,
        'presentation_clock_implemented':False}

def main(argv=None) -> int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('original',type=Path)
    p.add_argument('--report',type=Path,required=True)
    p.add_argument('--source-root',type=Path)
    a=p.parse_args(argv); created=False
    try:
        if a.original.resolve()==a.report.resolve() or os.path.lexists(a.report):
            raise ValueError('report aliases input or already exists')
        data=a.original.read_bytes(); report=audit(data)
        if a.source_root:report['source_map']=source_map(a.source_root)
        if sha(a.original.read_bytes())!=SHA:raise ValueError('input changed during audit')
        with a.report.open('x',encoding='utf-8') as stream:
            created=True;json.dump(report,stream,indent=2);stream.write('\n')
        print(json.dumps({k:v for k,v in report.items() if k!='witnesses'}))
        return 0
    except (OSError,ValueError,struct.error) as exc:
        if created:a.report.unlink(missing_ok=True)
        print('ERROR: '+str(exc),file=sys.stderr);return 2
if __name__=='__main__':raise SystemExit(main())
