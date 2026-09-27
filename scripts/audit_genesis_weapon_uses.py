"""SHA-pinned static proof for Scanner retained-weapon call-site gates."""
import argparse,json,struct
from pathlib import Path
from audit_hud_native_ops import Image

# Two push imm8 arguments, MOV EAX,[EBP-8], then the six-byte retained load.
# Getter has no stack arguments; each sound command removes eight bytes.
SITES=(
    (0x02B20FD0,'6a016a008b45f8','8b88fc020000e8c1b20eff8bc8e86a410cff'),
    (0x02B2144C,'6a046a008b45f8','8b88fc020000e845ae0eff8bc8e8b3c216ff'),
    (0x02B21465,'6a056a008b45f8','8b88fc020000e82cae0eff8bc8e89ac216ff'),
    (0x02B2147E,'6a066a008b45f8','8b88fc020000e813ae0eff8bc8e881c216ff'),
    (0x02B21DF6,'6a056a008b45f8','8b88fc020000e89ba40eff8bc8e844330cff'),
    (0x02B21E2F,'6a046a008b45f8','8b88fc020000e862a40eff8bc8e80b330cff'),
    (0x02B2210A,'6a046a008b45f8','8b88fc020000e887a10eff8bc8e8f5b516ff'),
    (0x02B22123,'6a056a008b45f8','8b88fc020000e86ea10eff8bc8e8dcb516ff'),
    (0x02B2213C,'6a076a008b45f8','8b88fc020000e855a10eff8bc8e8fe2f0cff'),
    (0x02B22305,'6a066a008b45f8','8b88fc020000e88c9f0eff8bc8e8352e0cff'),
    (0x02B22320,'6a076a008b45f8','8b88fc020000e8719f0eff8bc8e81a2e0cff'),
    (0x02B223F9,'6a046a008b45f8','8b88fc020000e8989e0eff8bc8e806b316ff'),
    (0x02B22412,'6a056a008b45f8','8b88fc020000e87f9e0eff8bc8e8edb216ff'),
    (0x02B2242B,'6a086a008b45f8','8b88fc020000e8669e0eff8bc8e80f2d0cff'),
    (0x02B22575,'6a096a008b45f8','8b88fc020000e81c9d0eff8bc8e8c52b0cff'),
    (0x02B236CB,'6a026a008b45f8','8b88fc020000e8c68b0eff8bc8e86f1a0cff'),
    (0x02B23986,'6a036a008b45f8','8b88fc020000e80b890eff8bc8e8b4170cff'),
    (0x02B23D96,'6a026a008b45f8','8b88fc020000e8fb840eff8bc8e8a4130cff'),
    (0x02B24256,'6a036a008b45f8','8b88fc020000e83b800eff8bc8e8e40e0cff'),
    (0x02B24410,'6a026a008b45f8','8b88fc020000e8817e0eff8bc8e82a0d0cff'),
    (0x02B25255,'6a046a008b45f8','8b88fc020000e83c700eff8bc8e8aa8416ff'),
    (0x02B2526E,'6a056a008b45f8','8b88fc020000e823700eff8bc8e8ccfe0bff'),
    (0x02B25461,'6a046a008b45f8','8b88fc020000e8306e0eff8bc8e89e8216ff'),
    (0x02B2547A,'6a056a008b45f8','8b88fc020000e8176e0eff8bc8e8858216ff'),
    (0x02B255DA,'6a026a008b45f8','8b88fc020000e8b76c0eff8bc8e860fb0bff'),
    (0x02B25CD9,'6a046a008b45f8','8b88fc020000e8b8650eff8bc8e8267a16ff'),
    (0x02B25CF2,'6a056a008b45f8','8b88fc020000e89f650eff8bc8e80d7a16ff'),
    (0x02B26AB6,'6a046a008b45f8','8b88fc020000e8db570eff8bc8e8496c16ff'),
    (0x02B26ACF,'6a056a008b45f8','8b88fc020000e8c2570eff8bc8e8306c16ff'),
    (0x02B26D9A,'6a046a008b45f8','8b88fc020000e8f7540eff8bc8e8656916ff'),
    (0x02B26DB3,'6a056a008b45f8','8b88fc020000e8de540eff8bc8e84c6916ff'),
    (0x02B27986,'6a096a008b45f8','8b88fc020000e80b490eff8bc8e8b4d70bff'),
    (0x02B280BA,'6a066a008b45f8','8b88fc020000e8d7410eff8bc8e8455616ff'),
    (0x02B2821C,'6a066a008b45f8','8b88fc020000e875400eff8bc8e8e35416ff'),
    (0x02B28235,'6a076a008b45f8','8b88fc020000e85c400eff8bc8e8ca5416ff'),
    (0x02B282A6,'6a066a008b45f8','8b88fc020000e8eb3f0eff8bc8e8595416ff'),
    (0x02B282BF,'6a076a008b45f8','8b88fc020000e8d23f0eff8bc8e8405416ff'),
    (0x02B284C7,'6a026a008b45f8','8b88fc020000e8ca3d0eff8bc8e873cc0bff'),
    (0x02B28812,'6a086a008b45f8','8b88fc020000e87f3a0eff8bc8e828c90bff'),
    (0x02B2885E,'6a086a008b45f8','8b88fc020000e8333a0eff8bc8e8dcc80bff'),
 )

def audit(data):
    im=Image(data);count=0
    def exact(at,raw,label):
        nonlocal count
        expected=bytes.fromhex(raw)
        if im.read(at,len(expected))!=expected:raise ValueError(f'{label} at {at:08X}')
        count+=1
    for at,prefix,pair in SITES:
        exact(at-7,prefix,'two pending arguments and Scanner EAX')
        exact(at,pair,'retained load, getter, ECX result and ret8 command')
    for at,raw,label in (
        (0x02B20D23,'8b45f88b88fc020000518b5508528b4df8','reactivation wrapper arguments'),
        (0x02B20D39,'5f5e5b81c4cc000000','wrapper normal epilogue'),
        (0x02B20D4C,'c20400','wrapper ret4'),
        (0x020F3476,'81c1c40e0000','weapon sound member'),
        (0x04E311B8,'301f3905','sound vtable complete-object locator'),
        (0x05391F3C,'60964e05','sound RTTI type descriptor'),
        (0x054E9668,'2e3f41567542696f536f756e644d6f74696f6e536540736f756e644067616d6540617070404000','uBioSoundMotionSe RTTI name'),
        (0x02EE1FFE,'c700bc11e304','sound constructor vtable store'),
        (0x027EA6AB,'6a106818010000','sound aligned allocation size 118'),
        (0x027EA6C9,'8b8d0cfeffff','constructed sound allocation receiver'),
        (0x027EA6D4,'898568fdffff','save constructed sound'),
        (0x027EA6E6,'8b8568fdffff508b4df881c1c40e0000','assign sound to weapon EC4 reference'),
        (0x020F3494,'c3','sound getter no-argument return'),
        (0x020F3512,'8b45f88b00','sound smart-reference pointer load'),
        (0x020F352A,'c3','reference getter no-argument return'),
        (0x02EE30BF,'c20800','first sound command ret8'),
        (0x02EE3771,'c20800','second sound command ret8'),
    ):exact(at,raw,label)
    for at,target,opcode in (
        (0x01C0C29C,0x020F3450,0xE9),(0x020F347C,0x01C561E9,0xE8),
        (0x027EA6CF,0x01BFEB2E,0xE8),(0x01BFEB2E,0x02EE1FD0,0xE9),
        (0x027EA6F6,0x01B92FE1,0xE8),(0x01B92FE1,0x027F5DD0,0xE9),
        (0x02487316,0x01BA460B,0xE8),(0x01BA460B,0x02484CF0,0xE9),
        (0x02484D16,0x01BDA67A,0xE8),(0x01BDA67A,0x0249D990,0xE9),
        (0x01C561E9,0x020F34B0,0xE9),(0x01BE514C,0x02EE3010,0xE9),
        (0x01C8D711,0x02EE36E0,0xE9),(0x02B20D34,0x01B9E86E,0xE8),
    ):exact(at,(bytes([opcode])+struct.pack('<i',target-at-5)).hex(),'native call or thunk')
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':count,'command_uses':len(SITES),
            'retained_component':'app::game::sound::uBioSoundMotionSe','reactivation_wrappers':1,'storage_pinned_through_callbacks':False,'gameplay_executed':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path)
    print(json.dumps(audit(p.parse_args().image.read_bytes()),indent=2))
