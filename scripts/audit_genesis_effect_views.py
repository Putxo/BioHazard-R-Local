"""Read-only January effect draw routing evidence; never executes the game."""
import argparse,json,struct
from pathlib import Path
from audit_hud_native_ops import Image
from audit_genesis_effect_graph import witnesses as graph_witnesses

def witnesses():
    yield from graph_witnesses()
    for at,data,label in (
        (0x0279D1C6,'8b400cc1e81025ff030000','Native ten-bit unit view mask'),
        (0x0326870D,'2345d47459','Immediate draw intersects context mask'),
        (0x0326891D,'2345d4744f','Scheduled draw intersects context mask'),
        (0x02933810,'8b4218ffd0','FilterSet update callback slot6'),
        (0x02933980,'8b421cffd0','FilterSet update callback slot7'),
        (0x02933A70,'8b4220ffd0','FilterSet update callback slot8'),
        (0x02933B60,'8b4224ffd0','FilterSet draw callback slot9'),
        (0x02933BB1,'8b450850','FilterSet forwards render context to child'),
    ):
        yield at,bytes.fromhex(data),label
    for at in (0x03268708,0x03268918):
        yield at,b'\xe8'+struct.pack('<i',0x01BE2D20-at-5),'Existing unit-mask hook'
    noop=bytes.fromhex('558bec81eccc000000535657518dbd34ffffffb933000000b8ccccccccf3ab59894df85f5e5b8be55dc20400')
    for i,(thunk,target) in enumerate(((0x01C80485,0x029343E0),(0x01BAFE43,0x02934420),
                                      (0x01C67CB4,0x02934460),(0x01BEDFA9,0x029344A0))):
        yield 0x04DE4564+0x18+4*i,struct.pack('<I',thunk),'Scanner callback vtable phase'
        yield thunk,b'\xe9'+struct.pack('<i',target-thunk-5),'Scanner callback phase thunk'
        yield target,noop,'Complete no-op Scanner callback phase, ret4'

def audit(data):
    im=Image(data);count=0
    for at,expected,label in witnesses():
        if im.read(at,len(expected))!=expected:raise ValueError(f'{label} at {at:08X}')
        count+=1
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':count,'gameplay_executed':False}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path)
    print(json.dumps(audit(p.parse_args().image.read_bytes()),indent=2))
