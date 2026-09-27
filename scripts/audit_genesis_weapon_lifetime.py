"""Static uWpScanner lifetime and native Scanner retention witnesses."""
import argparse,json,struct
from pathlib import Path
from audit_hud_native_ops import Image

def audit(data):
    im=Image(data);count=0
    def exact(at,expected,label):
        nonlocal count
        if im.read(at,len(expected))!=expected:raise ValueError(f'{label} at {at:08X}')
        count+=1
    for at,raw,label in (
        (0x0248731E,'c700e48ed304','Concrete Scanner weapon birth VT store'),
        (0x02487426,'c700e48ed304','Concrete Scanner weapon death VT store'),
        (0x02487373,'b8a0a45705','Scanner weapon DTI getter'),
        (0x049AE2B0,'68301b058068b0120000','Scanner weapon DTI class and size'),
        (0x02B20D70,'558bec81ec6401000053565751','Scanner activation frame'),
        (0x02B20D93,'8b45f88b4d0c8988fc020000','Store second argument in Scanner+2FC'),
        (0x02B21097,'5f5e5b81c464010000','Activation early-abort epilogue'),
        (0x02B210A7,'8be55dc20800','Activation returns and removes two arguments'),
        (0x02B21493,'c780fc02000000000000','Scanner close clears retained weapon'),
        (0x02B20FD0,'8b88fc020000','Activation subsequently dereferences retained weapon'),
        (0x02B2144C,'8b88fc020000','Close subsequently dereferences retained weapon'),
        (0x05472638,'2e3f41567557705363616e6e65724063686172614067616d6540617070404000','uWpScanner RTTI name'),
    ):exact(at,bytes.fromhex(raw),label)
    for at,target,label in (
        (0x024873B6,0x01C622CD,'Scalar deleting destructor calls concrete body'),
        (0x024873C7,0x01C05A73,'Scalar deleting destructor conditionally frees allocation'),
    ):exact(at,b'\xe8'+struct.pack('<i',target-at-5),label)
    for at,target in ((0x01C622CD,0x02487400),):
        exact(at,b'\xe9'+struct.pack('<i',target-at-5),'Destructor body thunk')
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':count,'gameplay_executed':False,
            'storage_pinned_through_callbacks':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path)
    print(json.dumps(audit(p.parse_args().image.read_bytes()),indent=2))
