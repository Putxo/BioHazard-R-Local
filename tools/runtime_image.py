#!/usr/bin/env python3
"""Install the linked HUD/menu module into a separate, exact January candidate.

Never launches a process or edits the supplied image. Unsupported builds fail.
This installs the current implemented subset, not complete local co-op.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'patches'))
sys.path.insert(0,str(ROOT/'scripts'))
from build_local_routing import PE,align,pe_checksum,rel
from audit_lifetime_observer import SITES as LIFE_SITES
from audit_hud_manager_phases import SITES as MANAGER_SITES
from audit_pipeline_frame import SITES as PIPELINE_SITES
from check_local_input import inspect_binder
from pad_axis_sites import patches as axis_patches, inspect_axes
from pad_state_sites import patches as state_patches
from input_owner_sites import hooks as input_owner_hooks

def inline_input_sites():return [*axis_patches(),*state_patches()]

BASES={
    # Cpu(3) -> Pad(1), Network(2) preserved: GNU and LLVM 22.1.8.
    '31cbebd180bcc66da2afbdc57c928b22a47f956c53bb5ed12411a220206490ec',
    '152bce5dba9eb1a270d2fd392921883e772bfc42682b23497ae72452747814c7',
}
LEGACY_CPU2_BASES={
    '71f5e70dc19c334d303ee29a686d65a72cd18b0c87b98472170bff961cb048f4',
    '506ddc362abfa3f330b6d9d8dc75e4ca81c6b6f31bccb6568427899e26613b7b',
}

def sha(data):return hashlib.sha256(data).hexdigest()

def read_module(data):
    if len(data)<52 or data[:7]!=b'\x7fELF\x01\x01\x01' or struct.unpack_from('<HH',data,16)!=(2,3):
        raise ValueError('requires linked ELF32 i386')
    off=struct.unpack_from('<I',data,32)[0]
    size,count,index=struct.unpack_from('<HHH',data,46)
    if size!=40 or not count or index>=count or off+size*count>len(data):raise ValueError('bad section table')
    entries=[struct.unpack_from('<10I',data,off+size*i) for i in range(count)]
    def content(e):
        if e[1]==8:return bytes(e[5])
        if e[4]+e[5]>len(data):raise ValueError('truncated ELF section')
        return data[e[4]:e[4]+e[5]]
    def name(strings,index):
        if index>=len(strings):raise ValueError('bad string index')
        return strings[index:].split(b'\0',1)[0].decode('ascii')
    names=content(entries[index]);sections={};symbols={}
    for e in entries:
        n=name(names,e[0])
        if e[1] in (4,9) and e[5]:raise ValueError('unresolved ELF relocations')
        if e[2]&2 and e[5]:
            if n not in ('.revtext','.revdata') or n in sections:raise ValueError('unexpected allocated section '+n)
            if e[5]>0x100000:raise ValueError('module section too large')
            if e[2]&7!=(6 if n=='.revtext' else 3):raise ValueError('unexpected module permissions')
            sections[n]=(e[3],content(e))
        if e[1]==2:
            if e[9]!=16 or e[5]%16 or e[6]>=count:raise ValueError('bad symbol table')
            strings=content(entries[e[6]])
            raw=content(e)
            for p in range(0,len(raw),16):
                ni,val,sz,info,other,idx=struct.unpack_from('<IIIBBH',raw,p)
                n=name(strings,ni)
                if n and idx==0:raise ValueError('unresolved symbol '+n)
                if n:symbols[n]=val
    if set(sections)!={'.revtext','.revdata'} or sections['.revtext'][0]!=0x5800000:
        raise ValueError('unexpected module layout')
    a,b=sections['.revtext'];c,d=sections['.revdata']
    if c!=align(a+len(b),4096):raise ValueError('unexpected data address')
    return sections,symbols

def hook_sites():
    out=[(0x01EB73A0,bytes.fromhex('558bec81ecfc000000'),'rev_action_draw_gate',0xE9)]
    out.append((0x02DEE8D0,bytes.fromhex('558bec81eccc000000'),'rev_script_input_gate',0xE9))
    out.extend(input_owner_hooks())
    out.append((0x02DAC787,rel(0x02DAC787,0x01BF908E),'rev_input_update_device',0xE8))
    out.append((0x02DAC8A1,rel(0x02DAC8A1,0x01C2742F),'rev_input_update_keyboard',0xE8))
    for va in (0x02DB36A1,0x02DB36B6):
        out.append((va,rel(va,0x01C15CB6),'rev_pad_clear_gate',0xE8))
    for va,name in ((0x02C2C641,'current'),(0x02C2C64F,'reference')):
        out.append((va,rel(va,0x01B7D7CC),'rev_menu_gate_options_'+name,0xE8))
    out.append((0x02C2CF0E,rel(0x02C2CF0E,0x01C1A0D6),'rev_menu_gate_options_apply',0xE8))
    for va in (0x02DAE930,0x02DAE960,0x02DAE9CA,0x02DAE9FA,0x02DAEA5B,0x02DAEA89,0x02DAEBE6,0x02DAECF6,0x02DAEE06,0x02DAEF16):
        out.append((va,bytes.fromhex('8b8870090000'),'rev_menu_gate_options_ecx',0xE8))
    for va in (0x02DAE98C,0x02DAEA26):
        out.append((va,bytes.fromhex('8b9170090000'),'rev_menu_gate_options_edx',0xE8))
    out.append((0x0278CC40,bytes.fromhex('558bec81eccc000000'),'rev_local_think_mode_gate',0xE9))
    out.append((0x0279CFBD,bytes.fromhex('e958010000'),'rev_aim_visibility_gate',0xE9))
    out.append((0x02B41E36,rel(0x02B41E36,0x01C2C7F4),'rev_scope_actor_gate',0xE8))
    for va in (0x02B295C6,0x02B295FE):
        out.append((va,bytes.fromhex('8b4230ffd0'),'rev_genesis_filter_load_gate',0xE8))
    out.append((0x02B26B60,bytes.fromhex('8b4214ffd0'),'rev_genesis_complete_gate',0xE8))
    out.append((0x02B2F8ED,rel(0x02B2F8ED,0x01C41E92),'rev_genesis_scan_call_gate',0xE8))
    out.append((0x02B1FD94,rel(0x02B1FD94,0x01C0CE54),'rev_genesis_state_call_gate',0xE8))
    out.append((0x028206D2,rel(0x028206D2,0x01C315EC),'rev_genesis_notify_gate',0xE8))
    out.append((0x02823AA8,rel(0x02823AA8,0x01C919B5),'rev_genesis_remove_target_gate',0xE8))
    for va in (0x023CEA44,0x023D4449,0x027EBD87,0x027EBE4C,0x027EBEAA,0x027EBED8):
        out.append((va,rel(va,0x01BBECD1),'rev_genesis_detect_gate',0xE8))
    for va,target,name in ((0x0281FB8A,0x01C5741D,'detector_camera'),
                          (0x0282006F,0x01C2C7F4,'detector_actor'),
                          (0x028206F5,0x01B86273,'detector_widget'),
                          (0x02820738,0x01B86273,'detector_widget'),
                          (0x02820716,0x01C0582A,'candidate')):
        out.append((va,rel(va,target),'rev_genesis_'+name+'_gate',0xE8))
    for va,name in ((0x0281F510,'focus'),(0x0281DDE0,'position')):
        out.append((va,bytes.fromhex('558bec81eccc000000'),'rev_genesis_set_'+name+'_gate',0xE9))
    out.append((0x0281F560,bytes.fromhex('558bec81eccc000000'),'rev_genesis_target_focus_gate',0xE9))
    out.append((0x02823220,bytes.fromhex('558bec81eccc000000'),'rev_genesis_target_position_gate',0xE9))
    out.append((0x02B25D8B,rel(0x02B25D8B,0x01C5741D),'rev_genesis_camera_gate',0xE8))
    out.append((0x0281AE26,bytes.fromhex('8b45f85f5e'),'rev_genesis_target_born_gate',0xE9))
    out.append((0x02823A53,bytes.fromhex('8b45f8c7007085da04'),'rev_genesis_target_dying_gate',0xE9))
    for va,vt,name in ((0x0293344E,0x04DB80CC,'filter_born'),(0x02933576,0x04DB80CC,'filter_dying'),
                       (0x0293559E,0x04DB833C,'noise_born'),(0x02935816,0x04DB833C,'noise_dying'),
                       (0x037DD309,0x04F9A7F4,'outline_born'),(0x037DD4A2,0x04F9A7F4,'outline_dying')):
        out.append((va,b'\xc7\x00'+struct.pack('<I',vt),'rev_genesis_effect_'+name+'_gate',0xE9))
    for va,old,name in ((0x02933B20,'558bec81ece4000000','filter'),
                        (0x02935660,'558bec81eccc000000','noise'),
                        (0x037DE560,'538bdc83ec08','outline')):
        out.append((va,bytes.fromhex(old),'rev_genesis_'+name+'_draw_gate',0xE9))
    out.append((0x0326862E,rel(0x0326862E,0x01BCD42A),'rev_runtime_draw_schedule_gate',0xE8))
    for va,name in ((0x0248731E,'born'),(0x02487426,'dying')):
        out.append((va,bytes.fromhex('c700e48ed304'),'rev_genesis_weapon_'+name+'_gate',0xE9))
    out.append((0x02B20D99,bytes.fromhex('8988fc020000'),'rev_genesis_weapon_retain_gate',0xE9))
    for va in (0x02B20FD0,0x02B2144C,0x02B21465,0x02B2147E,0x02B21DF6,0x02B21E2F,0x02B2210A,0x02B22123,0x02B2213C,0x02B22305,0x02B22320,0x02B223F9,0x02B22412,0x02B2242B,0x02B22575,0x02B236CB,0x02B23986,0x02B23D96,0x02B24256,0x02B24410,0x02B25255,0x02B2526E,0x02B25461,0x02B2547A,0x02B255DA,0x02B25CD9,0x02B25CF2,0x02B26AB6,0x02B26ACF,0x02B26D9A,0x02B26DB3,0x02B27986,0x02B280BA,0x02B2821C,0x02B28235,0x02B282A6,0x02B282BF,0x02B284C7,0x02B28812,0x02B2885E):
        out.append((va,bytes.fromhex('8b88fc020000'),'rev_genesis_weapon_command_gate',0xE8))
    out.append((0x02B20D26,bytes.fromhex('8b88fc020000'),'rev_genesis_weapon_reactivate_gate',0xE8))
    out.append((0x02B23120,bytes.fromhex('558bec81eccc000000'),'rev_genesis_effect_toggle_gate',0xE9))
    for va in (0x032686DA,0x032688EE):
        out.append((va,rel(va,0x01C42635),'rev_genesis_color_draw_enabled_gate',0xE8))
    out.append((0x034F1EEC,rel(0x034F1EEC,0x01BE0142),'rev_genesis_hunter_cache_gate',0xE8))
    out.append((0x034FA2D2,rel(0x034FA2D2,0x01C3B12D),'rev_genesis_hunter_material_bind_gate',0xE8))
    out.append((0x034FA464,rel(0x034FA464,0x01B9E855),'rev_genesis_hunter_material_unbind_gate',0xE8))
    for va in (0x03268708,0x03268918):
        out.append((va,rel(va,0x01BE2D20),'rev_action_mask_gate',0xE8))
    for va,old,name in ((0x01EA61A7,'8d85e4feffff50','begin'),
                        (0x01EA6235,'7e05e958040000','decide'),
                        (0x01EA6679,'6a008d8df4feffff','commit'),
                        (0x01EA66B6,'8b45f88b8d00ffffff','finish')):
        out.append((va,bytes.fromhex(old),'rev_priority_'+name+'_gate',0xE9))
    for va,target,name in ((0x01EA6215,0x01BEEF99,'prepare'),(0x01EB7419,0x01C6FF63,'draw_getter')):
        out.append((va,rel(va,target),'rev_priority_'+name+'_gate',0xE8))
    for va in (0x0279900E,0x0279907A,0x027999CF,0x027ACE72,0x027B211C,0x027B21E9):
        out.append((va,rel(va,0x01C88B6C),'rev_hud_heal_gate',0xE8))
    for va in (0x02B1FDAA,0x02B20DF8,0x02B21038,0x02B211EF,0x02B212B8,0x02B216DC,0x02B217BC,0x02B218DC,0x02B21F91,0x02B21FC4,0x02B22360,0x02B260B1,0x02B269A9,0x02B2788C,0x02B27A2F,0x02B27AEE,0x02B27C06,0x02B27C72,0x02B28C41):
        out.append((va,rel(va,0x01B82362),'rev_genesis_read',0xE8))
    for va in (0x02B212A7,0x02B27C6A,0x02B28C39):
        out.append((va,rel(va,0x01BFA407),'rev_genesis_set',0xE8))
    for va in (0x02B268DB,0x02B2699E,0x02B26A42,0x02B26A79):
        out.append((va,rel(va,0x01BB2404),'rev_genesis_add',0xE8))
    for va in (0x02B214B8,0x02B2203D,0x02B2246B,0x02B225BB,0x02B27905,0x02B27C3D,
               0x02B2F395,0x02B2F575,0x02B2F755,0x02B2F935,
               0x02B2FB15,0x02B2FCF5,0x02B2FED5,0x02B300B5):
        out.append((va,rel(va,0x01C2C7F4),'rev_hud_scanner_self',0xE8))
    for va in (0x02B21003,0x02B21244,0x02B2178B,0x02B218AB,0x02B224A5,0x02B28C88):
        out.append((va,rel(va,0x01C07341),'rev_genesis_selected_actor',0xE8))
    for name,va,old,_ in LIFE_SITES:out.append((va,bytes.fromhex(old),'rev_life_gate_'+name,0xE9))
    for va,family,phase,old in MANAGER_SITES:out.append((va,bytes.fromhex(old),f'rev_hud_gate_{family}{phase}',0xE9))
    for (va,old,_),name in zip(PIPELINE_SITES,('begin','end')):out.append((va,bytes.fromhex(old),'rev_pipeline_gate_'+name,0xE9))
    for va,old,name in ((0x02B49E5C,'8b4df8e846f310ff','cockpit11'),(0x02B687F0,'5f5e5b81c40c010000','minimap11')):
        out.append((va,bytes.fromhex(old),'rev_hud_mask_exit_'+name,0xE9))
    for va,name in ((0x02B404C6,'reticle'),(0x02B3B6C8,'equipment'),(0x02B61F3C,'herb'),(0x02B42FA6,'subequipment')):
        out.append((va,rel(va,0x01C2C7F4),'rev_hud_'+name+'_self',0xE8))
    menu=json.loads((ROOT/'patches/menu_routing/menu_sites.json').read_text())
    for site in menu['sites']:
        va=int(site['va'],16);op,target=site['original'].split();opcode=0xE8 if op=='call' else 0xE9
        out.append((va,rel(va,int(target,16),opcode),site['bridge'],opcode))
    return out

def build(base,module):
    if sha(base) in LEGACY_CPU2_BASES:
        raise ValueError('legacy Cpu(2) binder leaves J2 in AI; rebuild the January base with Cpu(3)')
    if sha(base) not in BASES:raise ValueError('unsupported input SHA256; requires the exact cumulative January base')
    pe=PE(base);input_check=inspect_binder(pe)
    if input_check['status']!='CPU3_BINDER_PRESENT':raise ValueError('requires corrected Cpu(3) binder')
    sections,symbols=read_module(module)
    if (pe.base,pe.count,pe.sa,pe.fa)!=(0x400000,10,4096,512):raise ValueError('unsupported PE layout')
    if pe.u32(pe.opt+16)!=0x01C897E7-pe.base:raise ValueError('unexpected original entry')
    if pe.u16(pe.opt+70)&0x40 or not pe.u16(pe.pe+22)&1:raise ValueError('requires fixed image without relocations')
    if pe.sh+12*40>pe.u32(pe.opt+60) or any(base[pe.sh+10*40:pe.sh+12*40]):raise ValueError('no room for section headers')
    if max(s['raw']+s['raw_size'] for s in pe.sections)!=len(base):raise ValueError('unexpected overlay')
    if max(s['va']+max(s['virtual_size'],s['raw_size']) for s in pe.sections)>sections['.revtext'][0]:raise ValueError('module overlaps image')
    textva,text=sections['.revtext'];data=bytearray(base);writes=[]
    def patch(offset,old,new,label):
        if len(old)!=len(new) or bytes(data[offset:offset+len(old)])!=old:raise ValueError('expected bytes: '+label)
        if any(offset<p['offset']+len(bytes.fromhex(p['old'])) and p['offset']<offset+len(old) for p in writes):raise ValueError('overlapping patch: '+label)
        writes.append({'offset':offset,'old':old.hex(),'new':new.hex(),'label':label});data[offset:offset+len(old)]=new
    def header(offset,value,label):patch(offset,base[offset:offset+4],struct.pack('<I',value),label)
    for va,old,name,opcode in hook_sites():
        target=symbols.get(name,0)
        if len(old)<5 or not textva<=target<textva+len(text):raise ValueError('invalid hook target: '+name)
        patch(pe.offset(va,len(old)),old,rel(va,target,opcode)+b'\x90'*(len(old)-5),name)
    for va,old,new,label in inline_input_sites():
        patch(pe.offset(va,len(old)),old,new,label)
    records=[]
    for i,(name,flags) in enumerate((('.revtext',0x60000020),('.revdata',0xC0000040)),10):
        va,blob=sections[name];raw=align(len(data),pe.fa);size=align(len(blob),pe.fa)
        if va%pe.sa or not blob:raise ValueError('unaligned section')
        data.extend(bytes(raw-len(data))+blob+bytes(size-len(blob)))
        sh=struct.pack('<8s6I2HI',name.encode().ljust(8,b'\0'),len(blob),va-pe.base,size,raw,0,0,0,0,flags)
        patch(pe.sh+i*40,bytes(40),sh,name+' header')
        records.append({'name':name,'va':va,'size':len(blob),'raw_size':size,'sha256':sha(blob)})
    entry=symbols.get('rev_runtime_entry',0)
    if not textva<=entry<textva+len(text):raise ValueError('bad runtime entry')
    patch(pe.pe+6,struct.pack('<H',10),struct.pack('<H',12),'section count')
    header(pe.opt+16,entry-pe.base,'entry')
    header(pe.opt+4,pe.u32(pe.opt+4)+records[0]['raw_size'],'code size')
    header(pe.opt+8,pe.u32(pe.opt+8)+records[1]['raw_size'],'data size')
    header(pe.opt+56,align(records[1]['va']+records[1]['size']-pe.base,pe.sa),'image size')
    header(pe.opt+64,pe_checksum(data,pe.opt+64),'checksum')
    out=bytes(data)
    if inspect_binder(PE(out))!=input_check:raise ValueError('runtime changed the Sub0 input binder')
    axis_check=inspect_axes(PE(out))
    if axis_check["status"]!="PAD_ARGUMENTS_PRESENT":raise ValueError("raw axis routing not installed")
    restored=bytearray(out[:len(base)])
    for p in writes:restored[p['offset']:p['offset']+len(bytes.fromhex(p['old']))]=bytes.fromhex(p['old'])
    if restored!=base:raise ValueError('reversal failed')
    return out,{'status':'EXPERIMENTAL_INTEGRATED_SUBSET','input_sha256':sha(base),'output_sha256':sha(out),
        'module_sha256':sha(module),'hook_count':len(hook_sites()),'writes':writes,'sections':records,
        'local_input_binder':input_check,'local_input_axes':axis_check,'inline_patch_count':len(inline_input_sites()),'indexed_state_sites':len(list(state_patches())),
        'exact_reversal':True,'gameplay_executed':False,'complete_local_coop':False}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('input',type=Path);p.add_argument('module',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    paths=[a.input,a.module,a.output,a.report]
    if len({p.resolve() for p in paths})!=4 or a.output.exists() or a.report.exists():raise SystemExit('paths must be distinct; outputs must not exist')
    out,report=build(a.input.read_bytes(),a.module.read_bytes())
    with a.output.open('xb') as f:f.write(out)
    with a.report.open('x',encoding='utf8') as f:json.dump(report,f,indent=2)
    print(json.dumps({k:v for k,v in report.items() if k not in ('writes','sections')}))
