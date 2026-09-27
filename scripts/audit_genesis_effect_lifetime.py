"""SHA-pinned scheduled Genesis effect boundaries; never executes a game."""
import argparse,json,struct
from pathlib import Path
from audit_hud_native_ops import Image

def call(at,target):return b'\xe8'+struct.pack('<i',target-at-5)
def witnesses():
    for at,vt in ((0x0293344E,0x04DB80CC),(0x02933576,0x04DB80CC),
                  (0x0293559E,0x04DB833C),(0x02935816,0x04DB833C),
                  (0x037DD309,0x04F9A7F4),(0x037DD4A2,0x04F9A7F4)):
        yield at,b'\xc7\x00'+struct.pack('<I',vt),'Observed concrete VT store'
    for at,target in ((0x02B294AE,0x01BF777A),(0x02B296A3,0x01C36254),
                      (0x02B29767,0x01C854C1),(0x02933506,0x01BF5ED9),
                      (0x029357A6,0x01B7DEFC),(0x037F89C1,0x01C052AD),
                      (0x02B2951B,0x01BF2BC6),(0x02B296D4,0x01BF2BC6),
                      (0x02B29798,0x01BF2BC6),(0x0293358B,0x01C3C159),
                      (0x02B2319C,0x01C8007A),(0x02B2324D,0x01C6D114)):
        yield at,call(at,target),'Native call'
    for at,target in ((0x01BF777A,0x02933420),(0x01C36254,0x02935570),
                      (0x01C854C1,0x037DD2F0),(0x01BF5ED9,0x02933550),
                      (0x01B7DEFC,0x029357F0),(0x01C052AD,0x037DD490)):
        yield at,b'\xe9'+struct.pack('<i',target-at-5),'Concrete constructor/destructor thunk'
    for at,data,label in (
        (0x02B2948D,'6a106a48','FilterSet allocation size'),
        (0x02B2967F,'6a1068d0000000','TVNoise allocation size'),
        (0x02B29743,'6a106850010000','Outline allocation size'),
        (0x02B294CE,'8988f0020000','Scanner FilterSet field'),
        (0x02B296F1,'89883c030000','Scanner TVNoise field'),
        (0x02B297B5,'898838030000','Scanner Outline field'),
        (0x0293344B,'8b45f8','FilterSet birth this'),
        (0x02933573,'8b45f8','FilterSet death this'),
        (0x0293559B,'8b45f8','TVNoise birth this'),
        (0x02935813,'8b45f8','TVNoise death this'),
        (0x037DD306,'8b45fc','Outline birth this'),
        (0x037DD49F,'8b45fc','Outline death this'),
        (0x029335B7,'8b4214ffd0','FilterSet callback occurs after death boundary'),
        (0x01D84BA6,'8b480cc1e90a83e13f83e108','Keep-alive bit is activity bit3'),
        (0x02B23339,'8888b0620000','Activation mutates singleton +62B0'),
        (0x02B232E9,'884865','Activation mutates other singleton +65'),
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
