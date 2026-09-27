"""Pinned January Genesis camera and producer boundaries; never executes input."""
from pathlib import Path
import argparse,json
from audit_genesis_progress import Image
WITNESSES=[
    (0x0281F510,"558bec81eccc000000","Focus setter displaced prologue"),
    (0x0281DDE0,"558bec81eccc000000","Position setter displaced prologue"),
    (0x0281F542,"c20400","Focus setter ret4"),
    (0x0281DE25,"c20400","Position setter ret4"),
    (0x0281FB92,"837dec007505e9300c0000","Detector permits absent camera before locals are constructed"),
    (0x028206F5,"e8795b36ff","Candidate scanner getter"),
    (0x02820738,"e8365b36ff","Status scanner getter"),
    (0x02820700,"83bde0fcffff007412","Candidate feed has scanner null guard"),
    (0x02820743,"83bdd4fcffff007476","Status feed has scanner null guard"),
    (0x0281FCD5,"6a046a00e82a0a39ff83c404","Shared filter keeps singleton index zero and bit four"),
    (0x0281F560,"558bec81eccc000000","Focus getter exact displaced prologue"),
    (0x02823220,"558bec81eccc000000","Position getter exact displaced prologue"),
    (0x0281F58F,"c3","Focus getter ret0"),
    (0x023CEA44,"e888027fff","Detector caller preserves its target-list owner"),
    (0x023D4449,"e883a87eff","Detector caller preserves its target-list owner"),
    (0x027EBD87,"e8452f3dff","Detector caller preserves its target-list owner"),
    (0x027EBE4C,"e8802e3dff","Detector caller preserves its target-list owner"),
    (0x027EBEAA,"e8222e3dff","Detector caller preserves its target-list owner"),
    (0x027EBED8,"e8f42d3dff","Detector caller preserves its target-list owner"),
    (0x0282006F,"e880c740ff","Detector selected actor finder"),
    (0x02823243,"8b45f883c010508b4d08e884b737ff8b4508","Vector getter copies into destination"),
    (0x02823268,"c20400","Position getter ret4"),
    (0x01CAE245,"8b45f8f30f10056c6dcb04f30f11400c","Vector copy constructor sets fourth component"),
    (0x01CA77D5,"8b45f8f30f10056c6dcb04f30f11400c","Vector assignment sets fourth component"),
    (0x04CB6D6C,"00000000","Canonical Vector3 fourth component is zero"),
    (0x02823A53,"8b45f8c7007085da04","Target destructor revocation before owner clear"),
    (0x02B25D8B,"e88d1613ff","Scanner camera call"),
    (0x02B25A42,"894df8","Aligned scanner method saves widget at EBP-8"),
    (0x02B25D90,"8945a4837da4000f8459010000","Caller null check"),
    (0x01CA5DDE,"a13c9d7905","Camera singleton"),
    (0x01F10786,"8b80e00c0000","Primary camera slot"),
    (0x01F107C6,"8b80e40c0000","Secondary camera slot"),
    (0x0205F2DE,"c7001c2bcf04","CameraManage vtable"),
    (0x02066AE4,"8b45f88b4d08894874","CameraManage actor target"),
    (0x0281FB8A,"e88e7843ff","Global detector still selects primary camera"),
    (0x02820716,"e80f513eff","Candidate feed"),
    (0x028207B9,"e878fc44ff","Detector status feed"),
    (0x0281F539,"894824","Per-target focus state write"),
    (0x0281F586,"8b4024","Per-target focus state read"),
    (0x0281B429,"894820","Per-target persistent state write"),
    (0x028232A6,"8b4020","Per-target persistent state read"),
    (0x0281DE07,"8b4df883c110e80ba33bff","Per-target position update"),
    (0x0281ADCE,"c7007085da04","Target constructor"),
    (0x0281AE26,"8b45f85f5e5b81c4cc0000003bece8837e46ff8be55d","Target constructor epilogue"),
    (0x02823AA8,"e808df46ff","Unconditional target base cleanup"),
    (0x02823AA0,"e8d40539ff","Stock scanner target removal"),
    (0x02B2A14F,"c20400","Native removal ret4"),
    (0x028207AC,"8b8584feffff508b8dd4fcffff","Status integer argument from EBP-17C"),
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
