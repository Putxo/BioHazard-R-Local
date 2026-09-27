#!/usr/bin/env python3
"""Read-only pinned scanner GUI/cache/pool witnesses. Does not run the game."""
import argparse,json
from pathlib import Path
from audit_hud_lifetime import Image
WITNESSES = [
    (0x02B1F74E,"6a008b4df8e89d8410ff8b4df8898158030000","Scanner cached resource mapping"),
    (0x02B1F761,"6a0c8b4df8e88a8410ff8b4df889815c030000","Scanner cached resource mapping"),
    (0x02B1F774,"6a028b4df8e8778410ff8b4df8898160030000","Scanner cached resource mapping"),
    (0x02B1F787,"6a038b4df8e8648410ff8b4df8898164030000","Scanner cached resource mapping"),
    (0x02B1F79A,"6a048b4df8e8518410ff8b4df8898168030000","Scanner cached resource mapping"),
    (0x02B1F7AD,"6a058b4df8e89a5916ff8b4df889816c030000","Scanner cached resource mapping"),
    (0x02B1F7C0,"6a068b4df8e82b8410ff8b4df8898170030000","Scanner cached resource mapping"),
    (0x02B1F7D3,"6a078b4df8e8188410ff8b4df8898174030000","Scanner cached resource mapping"),
    (0x02B1F7E6,"6a018b4df8e8058410ff8b4df8898180030000","Scanner cached resource mapping"),
    (0x02B1F7F9,"6a088b4df8e8f28310ff8b4df8898184030000","Scanner cached resource mapping"),
    (0x02B1F80C,"6a098b4df8e8df8310ff8b4df8898188030000","Scanner cached resource mapping"),
    (0x02B1F81F,"6a0a8b4df8e8cc8310ff8b4df889818c030000","Scanner cached resource mapping"),
    (0x02B1F832,"6a0b8b4df8e8b98310ff8b4df8898190030000","Scanner cached resource mapping"),
    (0x02B1F845,"6a0d8b4df8e8a68310ff8b4df889817c030000","Scanner cached resource mapping"),
    (0x02B1F858,"6a0e8b4df8e8938310ff8b4df8898178030000","Scanner cached resource mapping"),
    (0x02B1F86B,"6a008b4df8e869b306ff8b4df8898194030000","Scanner cached resource mapping"),
    (0x02B1F87E,"6a028b4df8e856b306ff8b4df8898198030000","Scanner cached resource mapping"),
    (0x02B1F891,"6a038b4df8e843b306ff8b4df889819c030000","Scanner cached resource mapping"),
    (0x02B1F8A4,"6a048b4df8e830b306ff8b4df88981a0030000","Scanner cached resource mapping"),
    (0x02B1F8B7,"6a098b4df8e81db306ff8b4df88981a4030000","Scanner cached resource mapping"),
    (0x02B1F8CA,"6a0a8b4df8e80ab306ff8b4df88981a8030000","Scanner cached resource mapping"),
    (0x02B1F8DD,"6a0b8b4df8e8f7b206ff8b4df88981ac030000","Scanner cached resource mapping"),
    (0x02B1F8F0,"6a018b4df8e8e4b206ff8b4df88981b8030000","Scanner cached resource mapping"),
    (0x02B1F903,"6a058b4df8e8d1b206ff8b4df88981bc030000","Scanner cached resource mapping"),
    (0x02B1F916,"6a068b4df8e8beb206ff8b4df88981c0030000","Scanner cached resource mapping"),
    (0x02B1F929,"6a078b4df8e8abb206ff8b4df88981c4030000","Scanner cached resource mapping"),
    (0x02B1F93C,"6a088b4df8e898b206ff8b4df88981c8030000","Scanner cached resource mapping"),
    (0x02B1F94F,"6a0c8b4df8e885b206ff8b4df88981b0030000","Scanner cached resource mapping"),
    (0x02B1F962,"6a0d8b4df8e872b206ff8b4df88981b4030000","Scanner cached resource mapping"),
    (0x02B1F987,"6a048b45f88b885c030000518b4df8e8989307ff8b55f88982cc030000","Scanner cached resource mapping"),
    (0x02B1F9A4,"6a038b45f88b885c030000518b4df8e87b9307ff8b55f88982d0030000","Scanner cached resource mapping"),
    (0x02B1F9C1,"6a028b45f88b885c030000518b4df8e85e9307ff8b55f88982d4030000","Scanner cached resource mapping"),
    (0x02B1F9DE,"6a0a8b45f88b885c030000518b4df8e8419307ff8b55f88982d8030000","Scanner cached resource mapping"),
    (0x02B1F9FB,"6a098b45f88b885c030000518b4df8e8249307ff8b55f88982dc030000","Scanner cached resource mapping"),
    (0x02B1FA18,"6a088b45f88b885c030000518b4df8e8079307ff8b55f88982e0030000","Scanner cached resource mapping"),
    (0x02B1FA35,"6a078b45f88b885c030000518b4df8e8ea9207ff8b55f88982e4030000","Scanner cached resource mapping"),
    (0x02B1FA52,"6a018b45f88b8870030000518b4df8e809690eff8b55f88982e8030000","Scanner cached resource mapping"),
    (0x02B1FA6F,"6a028b45f88b8874030000518b4df8e8ec680eff8b55f88982ec030000","Scanner cached resource mapping"),
    (0x02B1FA8C,"6a028b45f88b8878030000518b4df8e8cf680eff8b55f88982f0030000","Scanner cached resource mapping"),
    (0x02B1FAA9,"6a038b45f88b8878030000518b4df8e8b2680eff8b55f88982f4030000","Scanner cached resource mapping"),
    (0x02B0BD1F,"8b4df88b91f80000008b45088b0c8289","Node table indexed getter"),
    (0x02B0C2AF,"8b4df88b91f80000008b45088b0c8289","Nested node indexed getter"),
    (0x02BFF83F,"8b4508","Text cast preserves node address"),
    (0x02B0BC73,"8b4df881c120020000e845d507ff394508731a8b4508508b4df881c120020000e81fa708ff898530","Animation count and indexed access"),
    (0x01D943E3,"8b45f88b4004","Container count at4"),
    (0x01F9CD88,"8b45f88b48108b55088b0491","Container table at10"),
    (0x035DF330,"8b45e48b48088b55e48b42088b118bf48bc88b4204ffd03bf4e86e396a","Animation factory creates an instance"),
    (0x035DF367,"8b55dc528b4dfc81c120020000e81cc96afe","New animation inserted into GUI container220"),
    (0x02B1EF40,"6822cac4018b85f0feffff506a488b8dfcfeffff83c10851e8489b06ff","21 target entries with48 stride and8 cookie"),
    (0x02B1EFA2,"ba28000000","Second pool stride28"),
    (0x02B2D03A,"8b45086bc0488b4df80301","First pool address formula"),
    (0x02B2D83A,"8b45086bc0288b4df80301","Second pool address formula"),
    (0x02B1F5CC,"8b4df8e8c80e13ff8b4df881c124","Scanner destructor cleanup then private pools"),
    (0x02B1F116,"c605a02f590500","Constructor writes debug flag"),
 ]
def audit(data):
    im=Image(data)
    for a,h,label in WITNESSES:
        b=bytes.fromhex(h)
        if im.read(a,len(b))!=b:raise ValueError(label)
    return {"status":"PASS_STATIC_EVIDENCE_ONLY","checks":len(WITNESSES),"gameplay_executed":False}
if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("image",type=Path);a=p.parse_args()
    print(json.dumps(audit(a.image.read_bytes()),indent=2))
