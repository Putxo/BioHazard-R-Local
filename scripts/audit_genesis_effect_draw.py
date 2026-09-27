"""January auxiliary draw entry and context evidence, static inspection only."""
import argparse,json,struct
from pathlib import Path
from audit_hud_native_ops import Image
from audit_native_view_scope import audit as view_audit
from audit_genesis_effect_views import witnesses as view_witnesses

def witnesses():
    yield from view_witnesses()
    for at,data,label in (
        (0x02933B20,'558bec81ece4000000','FilterSet relocated draw prologue'),
        (0x02935660,'558bec81eccc000000','TVNoise relocated draw prologue'),
        (0x037DE560,'538bdc83ec08','Outline relocated aligned-frame prologue'),
        (0x037DE566,'83e4f083c404558b6b04896c24048bec','Outline aligned-frame continuation'),
        (0x037BF088,'660f6e45d80f5bc0','Noise converts rect.left'),
        (0x037BF090,'f30f580514ffcb04f30f5e45f4','Noise normalizes rect.left plus half pixel'),
        (0x037BF0BC,'660f6e45dc0f5bc0','Noise converts rect.top'),
        (0x037BF0C4,'f30f580514ffcb04f30f5e45f0','Noise normalizes rect.top plus half pixel'),
        (0x037DE9FD,'0f57c0f30f2ac0','Outline converts view height'),
        (0x037DEA04,'f30f100ddc87cb04f30f5ec8','Outline uses reciprocal view height'),
    ):
        yield at,bytes.fromhex(data),label
    for at,target,label in (
        (0x0293568A,0x01C6A2B1,'TVNoise forwards context to base draw'),
        (0x037BF083,0x01BE516F,'Noise reads active cDraw rectangle'),
        (0x037BF3E3,0x01BE516F,'Noise texture sizing reads active rectangle'),
        (0x037DE9F1,0x01BE516F,'Outline reads active cDraw rectangle'),
        (0x037DE9F8,0x01BA6497,'Outline reads rectangle height'),
    ):
        yield at,b'\xe8'+struct.pack('<i',target-at-5),label

def audit(data):
    im=Image(data);count=view_audit(data)['checks']
    for at,expected,label in witnesses():
        if im.read(at,len(expected))!=expected:raise ValueError(f'{label} at {at:08X}')
        count+=1
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':count,'gameplay_executed':False,
            'shader_and_final_pixel_clipping_validated':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path)
    print(json.dumps(audit(p.parse_args().image.read_bytes()),indent=2))
