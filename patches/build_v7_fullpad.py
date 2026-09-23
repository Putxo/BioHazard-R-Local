#!/usr/bin/env python3
from pathlib import Path
import hashlib, json

V6_INPUT=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP P2 SUB0 PADMODE v6.exe')
V6_COMBINED=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v6 SUB0 PADMODE NATIVE SPLIT.exe')
ORIG=Path('/mnt/data/BioRevHD 30-Enero-2013.exe')
OUT_INPUT=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP P2 FULLPAD v7.exe')
OUT_COMBINED=Path('/mnt/data/BioRevHD 30-Enero-2013 LOCAL COOP v7 FULLPAD NATIVE SPLIT.exe')
MAN=Path('/mnt/data/rev1_local_coop_research/p2-fullpad-v7-manifest.json')

TEXT_VA=0x01B79000
TEXT_RAW=0x400
EXPECTED_V6_INPUT='83554753d1d6a86bf256627830f509a313871a847c87338b61da0e3c0a2c0914'
EXPECTED_V6_COMBINED='18a429daa0855a7e6de6af91358d867970beee9a99671913303166f8b39a540d'

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def off(va): return va-TEXT_VA+TEXT_RAW

def ranges(a,b):
    out=[]; s=None; n=min(len(a),len(b))
    for i in range(n):
        if a[i]!=b[i] and s is None:s=i
        elif a[i]==b[i] and s is not None:out.append((s,i));s=None
    if s is not None:out.append((s,n))
    if len(a)!=len(b):out.append((n,max(len(a),len(b))))
    return out

SITES=[
0x02DAEB66,
0x02DAF7E5,0x02DAF7EF,0x02DAF675,0x02DAF67F,
0x02DAE200,0x02DAE238,
0x02DB0605,0x02DB060F,0x02DB0325,0x02DB032F,0x02DB0495,0x02DB049F,
0x02DB0915,0x02DB091F,0x02DB0845,0x02DB084F,0x02DAF02D,
0x02DAE650,0x02DAE688,0x01CB2FA6,0x02DB2285,
0x02DB0EC5,0x02DB0ECF,0x02DB0F95,0x02DB0F9F,0x02DB1065,0x02DB106F,
0x02DB1135,0x02DB113F,0x02DB1EB6,
0x02DAF8B5,0x02DAF8BF,
0x02DB0B85,0x02DB0B8F,0x02DB0C55,0x02DB0C5F,0x02DB0AB5,0x02DB0ABF,
0x02DB00B5,0x02DB00BF,0x02DB07A5,0x02DB2325,0x02DB23C5,
0x02DB09E5,0x02DB09EF,0x02DB0255,0x02DB025F,0x02DB03E6,0x02DB03F0,
0x02DB0556,0x02DB0560,
0x02DAE370,0x02DAE3A8,
0x02DAF4D5,0x02DAF4DF,
0x02DAE7C0,0x02DAE7F8,
0x01CB3106,
0x02DB0D25,0x02DB0D2F,
0x02DAFB25,0x02DAFB2F,
]

ALREADY_STANDARD=[0x02DAF404,0x02DAF40E,0x02DAFFE5,0x02DAFFEF]
ALREADY_ANALOG=[0x02DB15B7,0x02DB15C8,0x02DB1787,0x02DB1798,0x02DB1957,0x02DB1968,0x02DB1CD7,0x02DB1CE8]

assert len(SITES)==63
assert len(set(SITES))==63
assert sha(V6_INPUT)==EXPECTED_V6_INPUT
assert sha(V6_COMBINED)==EXPECTED_V6_COMBINED

patch_records=[]

def patch_blob(src:Path,out:Path):
    d=bytearray(src.read_bytes())
    for va in SITES:
        o=off(va)
        old=bytes(d[o:o+6])
        if len(old)!=6 or old[0]!=0x8B or old[2:6]!=bytes.fromhex('70090000') or (old[1]>>6)!=2:
            raise RuntimeError(f'{src.name} unexpected mStartPadNo load @ {va:#x}: {old.hex()}')
        reg=(old[1]>>3)&7
        new=bytes([0x8B,0x45|(reg<<3),0x08,0x90,0x90,0x90])
        d[o:o+6]=new
        if src==V6_INPUT:
            patch_records.append({'va':hex(va),'file_offset':hex(o),'old':old.hex(),'new':new.hex(),'dest_reg':reg})
    out.write_bytes(d)

patch_blob(V6_INPUT,OUT_INPUT)
patch_blob(V6_COMBINED,OUT_COMBINED)

base=ORIG.read_bytes(); i=OUT_INPUT.read_bytes(); c=OUT_COMBINED.read_bytes()
ri=ranges(base,i); rc=ranges(base,c)
manifest={
 'source':ORIG.name,'source_sha256':sha(ORIG),
 'base_v6_input_sha256':EXPECTED_V6_INPUT,
 'base_v6_combined_sha256':EXPECTED_V6_COMBINED,
 'input_output':OUT_INPUT.name,'input_sha256':sha(OUT_INPUT),
 'combined_output':OUT_COMBINED.name,'combined_sha256':sha(OUT_COMBINED),
 'status':'STATICALLY VERIFIED ONLY - gameplay not executed in this environment',
 'coverage':{
   'npc_selector_direct_calls_found':129,
   'standard_mStartPadNo_sites_total_in_relevant_consumer_methods':67,
   'standard_sites_already_fixed_in_v6':len(ALREADY_STANDARD),
   'additional_standard_sites_fixed_by_v7':len(SITES),
   'aligned_analog_sites_already_fixed_in_v6':len(ALREADY_ANALOG),
   'total_selector_load_sites_fixed_for_relevant_npc_input_path':67+len(ALREADY_ANALOG)
 },
 'design':{
   'identity_and_mode':'inherits v6: exact SubPlayer0 uNpc; canonical Cpu(2)->Pad(1); Network untouched.',
   'selector':'exact Sub0 returns index 1; all other NPCs return 0.',
   'fullpad':'restores selector argument in all mStartPadNo-collapsed sGamePad/action APIs identified as consumers of the uNpc selector, not just movement/aim/run.',
   'camera':'combined output inherits native camera split v4.'
 },
 'additional_patch_records':patch_records,
 'diff':{
   'input_same_size':len(i)==len(base),'input_different_bytes':sum(e-s for s,e in ri),'input_ranges':len(ri),
   'combined_same_size':len(c)==len(base),'combined_different_bytes':sum(e-s for s,e in rc),'combined_ranges':len(rc)
 },
 'limitations':[
   'Runtime validation is still required.',
   'The 129 selector call sites include several uses where the selector is tested/stored rather than immediately passed to an sGamePad API; these were preserved and do not require mStartPadNo replacement.',
   'This patch covers every mStartPadNo load found in the identified selector-consuming input methods, but other UI/menu systems outside the NPC gameplay path may have separate player-index assumptions.',
   'HUD, inventory/pause ownership and scripted/QTE behavior remain to validate.'
 ]
}
MAN.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('input',OUT_INPUT,manifest['input_sha256'])
print('combined',OUT_COMBINED,manifest['combined_sha256'])
print('additional sites',len(SITES),'total selector loads covered',manifest['coverage']['total_selector_load_sites_fixed_for_relevant_npc_input_path'])
print('diff input',manifest['diff']['input_different_bytes'],manifest['diff']['input_ranges'])
print('diff combined',manifest['diff']['combined_different_bytes'],manifest['diff']['combined_ranges'])
print('manifest',MAN)
