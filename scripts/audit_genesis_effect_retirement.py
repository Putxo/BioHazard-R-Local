"""SHA-pinned cUnit deferred retirement and Scanner callback cleanup."""
import argparse,json
from pathlib import Path
from audit_hud_native_ops import Image
from audit_genesis_effect_graph import witnesses as graph_witnesses

def witnesses():
    yield from graph_witnesses()
    for at,data,label in (
        (0x01D84AC5,'8b480c83e10783f902','Native kill accepts state2'),
        (0x01D84AD3,'8b480c83e10783f901','Native kill accepts state1'),
        (0x01D84AE4,'83e1f883c903','Native kill marks state3'),
        (0x01D84AED,'894a0c','Native kill writes cUnit flags'),
        (0x03477549,'81e2ff03ffff81ca004c0000','Constructor activity field is bits10 through15'),
        (0x0293359D,'83784400742a','FilterSet destructor skips absent parent callback'),
        (0x0293357F,'837840007415','FilterSet destructor owns resource release'),
    ):
        yield at,bytes.fromhex(data),label

def audit(data):
    im=Image(data);count=0
    for at,expected,label in witnesses():
        if im.read(at,len(expected))!=expected:raise ValueError(f'{label} at {at:08X}')
        count+=1
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':count,'gameplay_executed':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path)
    print(json.dumps(audit(p.parse_args().image.read_bytes()),indent=2))
