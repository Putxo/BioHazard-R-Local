"""Read-only SHA-pinned January scope witnesses. Never executes the image."""
import argparse,json
from pathlib import Path
from audit_hud_native_ops import Image
WITNESSES=[
    (0x02B4884C,"894854","Cockpit owns Scope at plus54"),
    (0x02B41AE0,"c7009455de04","Scope constructor installs exact vtable"),
    (0x02B41D04,"6a008b4df8e853050bff","Native initialization leaves Scope inactive"),
    (0x02B41D18,"6a008b4df8e82f3414ff8b4df88981a0020000","Cached panel zero"),
    (0x02B41D2B,"6a018b4df8e81c3414ff8b4df88981a4020000","Cached panel one"),
    (0x02B41D3E,"6a028b4df8e86eaf0bff8b4df88981a8020000","Cached text node two"),
    (0x02B41D67,"6a008b4df8e86d8e04ff8b4df88981ac020000","Cached animation zero"),
    (0x02B41E10,"894df8","Scope phase8 retains widget at EBP-8"),
    (0x02B41E36,"e8b9a90eff","Scope actor finder is thiscall with one predicate argument"),
    (0x02B41E3B,"8945ec8b4dece8473a12ff","Actor consumer has no null check"),
    (0x02B40DD6,"0540150000","Scope state is actor-specific plus1540"),
    (0x02B41EF0,"5f5e5b81c420010000","Revoked clone exits through original phase8 epilogue"),
    (0x02B41F03,"c3","Phase8 has no incoming stack arguments"),
    (0x02B420AA,"c20400","Scope draw receives one context argument"),
    (0x02B42293,"837d08007519","Null weapon selects Scope deactivation"),
    (0x02B4237C,"c20800","Scope activation consumes weapon and flag"),
    (0x0279D091,"0fb6450885c0","GUI activation uses the low byte, nonzero means open"),
    (0x0279D0A3,"8b4210ffd0","Weapon class is obtained through virtual slot10"),
    (0x0279D0B1,"e873eb42ff3d301b0580","Native class comparison distinguishes Genesis"),
    (0x01CA5716,"8b401c","DTI class identity is at plus1C"),
    (0x0279D0D4,"0fb6450c508b4dec518b4de0e896f545ff","Gun Scope receives weapon and second argument low byte"),
    (0x0279D10E,"6a016a008b4de0e861f545ff","Closing Scope supplies null weapon and flag1"),
    (0x02B42299,"6a008b4df8e8fa5613ff6a008b4df8e8b4ff0aff","Null weapon disables display and activity"),
    (0x02B422D5,"6a000fb6450c83e801f7d81bc083c00250","Animation selector uses the supplied flag"),
    (0x02B422E9,"8b89ac020000e87eb412ff","Activation changes the private cached animation"),
    (0x02B4235A,"8b88a4020000518b4df8e8e28012ff","Activation changes the private cached panel"),
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
