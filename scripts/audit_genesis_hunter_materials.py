"""SHA-pinned Hunter per-view material witnesses. Read-only; never launches the game."""
import argparse,json,struct
from pathlib import Path
from audit_hud_native_ops import Image

def audit(data):
    im=Image(data);checks=0
    def exact(at,raw,label):
        nonlocal checks
        expected=bytes.fromhex(raw)
        if im.read(at,len(expected))!=expected:raise ValueError(f'{label} at {at:08X}')
        checks+=1
    for at,raw,label in (
        (0x01C1705C,'e97f865e00','Hunter manager-slot predicate thunk'),
        (0x021FF6FE,'8b450850e893459cff83c4048338000f95c0','manager slot must contain an instance'),
        (0x021FE030,'f30f1080741a0000','Hunter base alpha comes from +1A74'),
        (0x021FE040,'6a00e81590a1ff83c4040fb6c085c0743d','Genesis alpha override requires manager slot'),
        (0x021FE051,'6a00e8862fa8ff83c4048bc8e86753a3ff0fb6c085c07425','manager +65 gates Genesis alpha'),
        (0x021FE069,'f30f108568ffffff0f2e05dc87cb049ff6c4447a10','override requires exact alpha 1.0'),
        (0x021FE07E,'f30f10057cbfd004','Genesis Hunter alpha is 0.99'),
        (0x021FE08E,'f30f1005dc87cb04f30f5c8568fffffff30f5905f4afcd04','first RGB vector uses (1-alpha)*0.75'),
        (0x021FE0BA,'f30f1005dc87cb04f30f5c8568fffffff30f118550ffffff','second RGB vector uses 1-alpha'),
        (0x021FE0D2,'f30f1005dc87cb04f30f5c8568fffffff30f5905acb0cc04','third RGB vector uses (1-alpha)*0.8'),
        (0x021FE6D4,'68f411806c','Hunter color material hash 6C8011F4'),
        (0x021FE704,'e88b77a3ff','color vector 44 setter'),
        (0x021FE723,'e86c77a3ff','color vector 0 setter'),
        (0x021FE742,'e84d77a3ff','color vector 40 setter'),
        (0x021FE79D,'682b32caef','Hunter alpha material hash EFCA322B'),
        (0x021FE7CC,'e85eec97ff','alpha index1 setter'),
        (0x021F3893,'8b45f88b80fc000000','model part count is +FC'),
        (0x034EFE3F,'8b55fc8b82f80000008b4d08','model part array is +F8'),
        (0x036C8997,'8b4508506a018b4dfce8774c52fe','parameter resolver requests group 1'),
        (0x036CE52D,'8b848188000000','group count table begins at +88; group1 is +8C'),
        (0x036CE21D,'8b448a78','group index table begins at +78; group1 is +7C'),
        (0x036CE2B3,'8b048a8945d48b4dd46bc90c8b55fc034a30','entry index *12 plus part+30 base'),
        (0x036CE2C8,'8b45d08b08c1e91481e1ff0f0000','material key uses upper 12-bit field'),
        (0x034FA2AB,'837dbc007c088b4b08e89c456afe','material change unbind remains native'),
        (0x034FA2CB,'8b45b0508b4b08e8560e74fe','material bind boundary'),
        (0x034FA45B,'837dbc007c088b4b08e8ec436afe','final material unbind boundary'),
        (0x03700AE4,'8b55088951688b45fc8b4df48948588b','bind stores material and native bind result in draw context'),
    ):exact(at,raw,label)
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':checks,'hook_sites':2,
            'hunter_material_parameters_per_view':True,
            'hunter_visibility_state_per_view':False,
            'gameplay_executed':False}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path)
    print(json.dumps(audit(p.parse_args().image.read_bytes()),indent=2))
