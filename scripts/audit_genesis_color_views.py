"""Static shared-color scheduler and per-view admission evidence."""
import argparse,json,struct
from pathlib import Path
from audit_hud_native_ops import Image

def audit(data):
    im=Image(data);count=0
    def exact(at,raw,label):
        nonlocal count
        expected=bytes.fromhex(raw)
        if im.read(at,len(expected))!=expected:raise ValueError(f'{label} at {at:08X}')
        count+=1
    for at,raw,label in (
        (0x0292B422,'c7009c7cdb04','concrete sGameFilters identity'),
        (0x0292B447,'81c1a0060000','embedded color filter offset'),
        (0x02939DA0,'c7008c84db04','concrete color filter identity'),
        (0x02930678,'0fb688b0620000','persistent Genesis color-disable control'),
        (0x02930681,'7416','disabled control skips draw enable setter'),
        (0x02B23339,'8888b0620000','Scanner global control write'),
        (0x01DEE416,'8b480c83e10783f902','update requires live state2'),
        (0x01DEE427,'c1e90a83e13f83e101','update uses bit0400 independently of draw bit0800'),
        (0x01DEE486,'8b480c83e10783f902','draw requires live state2'),
        (0x01DEE497,'c1e90a83e13f83e11283f912','draw requires both 0800 and 4000'),
        (0x0326870D,'2345d47459','immediate path intersects the context view'),
        (0x0326891D,'2345d4744f','scheduled path intersects the context view'),
        (0x037D13FE,'8b4d08','color draw receives cDraw context'),
        (0x037D141D,'0540040000','color draw parameters belong to its unit'),
    ):exact(at,raw,label)
    for at,target,opcode in (
        (0x032686DA,0x01C42635,0xE8),(0x032688EE,0x01C42635,0xE8),
        (0x01C42635,0x01DEE460,0xE9),
        (0x02930670,0x01C5EFA1,0xE8),(0x02930694,0x01B8D636,0xE8),
        (0x03268708,0x01BE2D20,0xE8),(0x03268918,0x01BE2D20,0xE8),
        (0x01BE6CBD,0x037D13F0,0xE9),(0x037D142B,0x01B97005,0xE8),
    ):exact(at,(bytes([opcode])+struct.pack('<i',target-at-5)).hex(),'native call or thunk')
    exact(0x04DB848C+0x2C,struct.pack('<I',0x01BE6CBD).hex(),'color draw virtual slot')
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':count,'gameplay_executed':False,
            'hunter_materials_isolated':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path)
    print(json.dumps(audit(p.parse_args().image.read_bytes()),indent=2))
