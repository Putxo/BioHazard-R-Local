"""SHA-pinned filter resource/copy contracts. Read-only; never runs a game."""
import argparse,json,struct
from pathlib import Path
from audit_hud_native_ops import Image

def call(at,target):return b'\xe8'+struct.pack('<i',target-at-5)
def jump(at,target):return b'\xe9'+struct.pack('<i',target-at-5)
def witnesses():
    for at in (0x02B295C6,0x02B295FE):
        yield at,bytes.fromhex('8b4230ffd0'),'Resource virtual loader hook'
    for at,target in ((0x02B2961A,0x01B94BF7),(0x02B29628,0x01C3C159),
                      (0x02933C83,0x01BD91A3),(0x029296AA,0x01C7A567),
                      (0x029296C7,0x01C5E8BC),(0x02929AFA,0x01C28424),
                      (0x02929B11,0x01B81E3F),(0x02929FA2,0x01B81E76),
                      (0x0292A0A7,0x01C40EB1),(0x02FD8C3A,0x01C5FC3F),
                      (0x0325C60B,0x01BC7FBB),(0x0325C631,0x01BE7311)):
        yield at,call(at,target),'Native call contract'
    for at,target in ((0x01C5E8BC,0x02929AC0),(0x01C28424,0x01D935B0),
                      (0x01C4B4F1,0x0292A050),(0x01C12E2B,0x02929F50),
                      (0x01C3C159,0x03278650),(0x01BC7FBB,0x03261500)):
        yield at,jump(at,target),'Pinned thunk'
    for at,data,label in (
        (0x02B29482,'894df8','Scanner setup this at EBP-8'),
        (0x02933C71,'894840','SetResource retains the supplied pointer'),
        (0x02933CCC,'8b4214ffd0','SetResource initializes each child'),
        (0x0325F568,'8b55f08b424883c001','Cached loader resource AddRef'),
        (0x0325F57F,'8b45f0e91a010000','Cached resource returned before flag dispatch'),
        (0x0325F6AE,'c20c00','Native loader ret12'),
        (0x029296A3,'6a106880000000','Resource allocation 80 bytes aligned16'),
        (0x02929AEE,'c700347adb04','Resource vtable'),
        (0x02929B09,'6a018b4df883c168','Array deletes its objects'),
        (0x01D935E7,'c7400400000000','Empty array count'),
        (0x01D935DE,'c7007872cc04','Owned array vtable'),
        (0x01D935F1,'c7400800000000','Empty array capacity'),
        (0x01D93602,'c7401000000000','Empty array table'),
        (0x01D9FC09,'88480c','Array deletion flag low byte'),
        (0x02933906,'83c168','Resource array offset68'),
        (0x01D05716,'8b4004','Array count offset4'),
        (0x01D93D3B,'8b4810','Array pointer offset10'),
        (0x01D93D41,'8b0491','Array indexes pointer entries'),
        (0x03277D0C,'c7414801000000','New resource reference count1'),
        (0x03277D20,'c7405400000000','New resource accounting size0'),
        (0x03277D54,'c7415800000000c7415c00000000','New resource cache key0'),
        (0x0326156E,'3b4d08750f','Cache removal compares actual resource pointer'),
        (0x02FD8BA9,'c7006429e404','Borrowed stream vtable'),
        (0x02FD8BDF,'894810','Borrowed stream flags'),
        (0x02FD8BF2,'894804','Borrowed stream buffer'),
        (0x02FD8BFB,'89420c','Borrowed stream capacity'),
        (0x02FD8C1D,'c20c00','Borrowed stream constructor ret12'),
        (0x02FD8D25,'83e104','Stream destructor frees only owning flag4'),
        (0x02FD8F63,'895014','Stream write overflow flag'),
        (0x02929F98,'8b4d0851','Load passes its stream argument'),
        (0x02929F8F,'8b45f450','Load passes preconstructed destination'),
        (0x0292A08D,'6840ce58056a00','Save pins resource DTI and options'),
        (0x02929EEA,'83c16851685853ce04','Serialized property binds the owned unit array'),
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
