"""Read-only SHA-pinned completion call/frame witnesses. Never executes a game."""
import argparse,json
from pathlib import Path
from audit_hud_native_ops import Image
WITNESSES=[
    (0x02B1FD8F,"6a018b4df8e8bbd00eff","Phase9 passes argument1 to the scanning state"),
    (0x02B2F8ED,"e8a02511ff","State dispatch calls the scanner processing body"),
    (0x02B2F9C2,"5f5e5b81c408010000","State exit restores three registers and108 locals"),
    (0x02B2F9D5,"c20400","State consumes the original byte argument stack slot"),
    (0x02B201BF,"528bcd508d15dc01b202e890aa08ff585a","Phase9 abort bypasses further consumers through balanced RTC tail"),
    (0x02B201D0,"5f5e8be55d8be35bc3","Phase9 restores its aligned caller stack"),
    (0x02B25A10,"538bdc83ec0883e4f083c40455","Scanner processing aligns its stack through EBX"),
    (0x02B25A24,"8bec81ece80300005657","Aligned frame has 3E8 locals and two saved registers"),
    (0x02B25A42,"894df8","Scanner widget is retained at EBP-8"),
    (0x02B26B50,"8b8538fcffff8b108bf48b8d38fcffff","Target and its vtable precede virtual completion"),
    (0x02B26B60,"8b4214ffd0","Completion gateway replaces virtual slot14 load and call"),
    (0x02B26B65,"3bf4e850c115ff","Normal return checks the original call stack"),
    (0x02B26B6C,"8b4dece8a09813ff8bc8e8a3cf13ff","Native caller reuses its cached entry after completion"),
    (0x02B26E7E,"528bcd508d15986eb202e8d13d08ff585a","Safe exit retains balanced native RTC validation"),
    (0x02B26E8F,"5f5e8be55d8be35bc3","Exit restores the aligned frame and original EBX stack"),
    (0x04DA8584,"5af3c701","Base target completion method"),
    (0x04DA85A8,"c159c301","First derived target completion method"),
    (0x04DA85CC,"bf92c301","Second derived target completion method"),
    (0x04DA85F0,"a89cc101","Third derived target completion method"),
    (0x04DA8614,"c880b801","Fourth derived target completion method"),
    (0x02823B16,"c7402002000000","Base completion changes shared world state to2"),
    (0x02823B6F,"8b483883e9018b55f8894a38","Derived scan allowance is decremented in shared target state"),
]
def audit(data):
    im=Image(data)
    for at,raw,label in WITNESSES:
        expected=bytes.fromhex(raw)
        if im.read(at,len(expected))!=expected:raise ValueError(label)
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':len(WITNESSES),'gameplay_executed':False}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path)
    print(json.dumps(audit(p.parse_args().image.read_bytes()),indent=2))
