"""Static proof of native activity setters and shared activation side effects."""
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
        (0x02B23120,'558bec81eccc000000','native effect toggle frame'),
        (0x02B23265,'c20400','native effect toggle ret4'),
        (0x01E007D1,'c1e90a83e13f83c901','phase8 enable bit'),
        (0x01E00801,'83e1fe83e13fc1e10a','phase8 disable bit'),
        (0x01DC73E1,'c1e90a83e13f83c902','phase9 enable bit'),
        (0x01DC7411,'83e1fd83e13fc1e10a','phase9 disable bit'),
        (0x0210F441,'c1e90a83e13f83c910','phase11 enable bit'),
        (0x0210F471,'83e1ef83e13fc1e10a','phase11 disable bit'),
        (0x02B23339,'8888b0620000','shared game filter boolean write'),
        (0x02B232E9,'884865','shared Hunter mode write'),
    ):exact(at,raw,label)
    for at,target,opcode in (
        (0x01BE6781,0x02B23120,0xE9),
        (0x02B2315D,0x01C5EFA1,0xE8),(0x02B23170,0x01B8D636,0xE8),(0x02B23183,0x01B7B5E4,0xE8),
        (0x02B231BB,0x01C5EFA1,0xE8),(0x02B231CE,0x01B8D636,0xE8),(0x02B231E1,0x01B7B5E4,0xE8),
        (0x02B23200,0x01C5EFA1,0xE8),(0x02B23213,0x01B8D636,0xE8),(0x02B23226,0x01B7B5E4,0xE8),
        (0x01C5EFA1,0x01E007A0,0xE9),(0x01B8D636,0x01DC73B0,0xE9),(0x01B7B5E4,0x0210F410,0xE9),
        (0x02B2319C,0x01C8007A,0xE8),(0x02B2324D,0x01C6D114,0xE8),
        (0x01C8007A,0x02B23310,0xE9),(0x01C6D114,0x02B232C0,0xE9),
    ):exact(at,(bytes([opcode])+struct.pack('<i',target-at-5)).hex(),'native call or thunk')
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':count,'local_activity_mask':'0x4C00',
            'per_view_shared_filter_materials_complete':False,'gameplay_executed':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path)
    print(json.dumps(audit(p.parse_args().image.read_bytes()),indent=2))
