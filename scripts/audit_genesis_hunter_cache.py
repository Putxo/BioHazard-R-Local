"""SHA-pinned Hunter model cache bypass witnesses. Read only; never runs the game."""
import argparse,json,struct
from pathlib import Path
from audit_hud_native_ops import Image

def audit(data):
    im=Image(data);checks=0
    def exact(at,raw,label):
        nonlocal checks
        expected=bytes.fromhex(raw)
        if im.read(at,len(expected))!=expected:raise ValueError(f'{label} at {at:08X}')
        checks+=1
    for at,raw,label in (
        (0x034F1D80,'538bdc83ec0883e4f083c404558b6b04896c24048bec','model caller aligned frame and EBX argument base'),
        (0x034F1DAC,'894dfc','model receiver saved at EBP-4'),
        (0x034F1DAF,'8b4b08','native context at EBX+8'),
        (0x034F1EEA,'8bc8','resource predicate receiver'),
        (0x01FA2683,'8b45f88b484c83e1080f95c0','stock resource predicate reads bit 8'),
        (0x01FA2695,'c3','stock predicate has no arguments'),
        (0x034F1EF1,'0fb6c885c97407c745d405000000','true predicate selects uncached kind 5'),
        (0x034F1F2E,'837dd4050f8389020000','kind 5 branches to direct draw 034F21C1'),
        (0x034F1FF1,'83bc810002000000','cached command slot indexed by pass kind'),
        (0x034F20C2,'89848a00020000','cache stores completed command object'),
        (0x034F2164,'528b4b08','cached dispatch receives command and current context'),
        (0x034F21C1,'8bf48b45d8508b4df8518d55e052','uncached draw uses current arguments'),
        (0x034F21CF,'8b45fc8b88f800000051','uncached draw passes current model parts'),
        (0x034F21E2,'8b5308528b45fc8b108b4dfc8b426cffd0','uncached context and virtual 6C call'),
        (0x04D0A028,'078ebc01','concrete Hunter virtual 6C slot'),
        (0x034F21FA,'6831d51d17','shared post-draw continuation'),
    ):exact(at,raw,label)
    for at,target,opcode in (
        (0x034F1EEC,0x01BE0142,0xE8),(0x01BE0142,0x01FA2660,0xE9),
        (0x034F2168,0x01B8DE24,0xE8),(0x01B8DE24,0x03701190,0xE9),
        (0x01BC8E07,0x034F9F40,0xE9),
    ):exact(at,(bytes([opcode])+struct.pack('<i',target-at-5)).hex(),'call or thunk')
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':checks,'hook_sites':1,
            'hunter_view_materials_complete':False,'gameplay_executed':False}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path)
    print(json.dumps(audit(p.parse_args().image.read_bytes()),indent=2))
