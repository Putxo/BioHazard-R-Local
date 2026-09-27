"""SHA-pinned scanner state actor/pack witnesses. Read-only; no game execution."""
import argparse,json,struct
from pathlib import Path
from audit_hud_native_ops import Image
# Function order differs from the actual native state numbers.
STATES=[(0x02B2F2C0,0),(0x02B2F4A0,1),(0x02B2F680,2),(0x02B2F860,3),
        (0x02B2FA40,7),(0x02B2FC20,4),(0x02B2FE00,5),(0x02B2FFE0,6)]
def call(at,target):return b'\xe8'+struct.pack('<i',target-at-5)
def witnesses():
    for start,state in STATES:
        yield start+0x20,bytes.fromhex('894df8'),'Widget retained at EBP-8'
        yield start+0x2F,bytes.fromhex('c780e4020000')+struct.pack('<I',state),'Native state identity'
        yield start+0xD5,call(start+0xD5,0x01C2C7F4),'Native actor finder'
        yield start+0xDA,bytes.fromhex('8945ec837dec007418'),'Missing actor skips pack lookup'
        yield start+0xE6,call(start+0xE6,0x01BCC327),'Resolved actor supplies its pack'
        yield start+0xED,call(start+0xED,0x01C2A51C),'Pack supplies herb count'
        yield start+0xF2,bytes.fromhex('8b4df889814c030000'),'Count cached on the same scanner at34C'
    yield 0x0243CEB6,bytes.fromhex('8b80c8000000'),'Count getter reads pack+C8'
def audit(data):
    im=Image(data);count=0
    for at,expected,label in witnesses():
        if im.read(at,len(expected))!=expected:raise ValueError(f'{label} at {at:08X}')
        count+=1
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':count,'states':8,'gameplay_executed':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path)
    print(json.dumps(audit(p.parse_args().image.read_bytes()),indent=2))
