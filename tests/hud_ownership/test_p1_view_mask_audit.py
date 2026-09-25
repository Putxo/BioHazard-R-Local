import contextlib,io,tempfile,unittest
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import audit_p1_view_mask as audit
class T(unittest.TestCase):
 def test_reject_bad(self):
  with self.assertRaises(ValueError):audit.audit(b'MZ'+bytes(100))
 def test_private(self):
  p=Path('/mnt/data/BioRevHD 30-Enero-2013(1).exe')
  if not p.exists():self.skipTest('private game image deliberately absent in CI')
  r=audit.audit(p.read_bytes());self.assertEqual(r['checks'],16)
 def test_source_map(self):
  root=Path(__file__).resolve().parents[2]
  s=(root/'patches/hud_ownership/p1_view_mask.cpp').read_text()
  g=(root/'patches/hud_ownership/manager_gateways.S').read_text()
  for x in ('0x02B49DE3','0x02B49E5C','0x02B68724','0x02B687F0','0x5C','0x90','0x30','0x40'):
   self.assertIn(x,s)
  for x in ('rev_hud_p1_mask_begin','rev_hud_p1_mask_end','rev_hud_mask_exit_cockpit11','rev_hud_mask_exit_minimap11'):
   self.assertIn(x,g)
if __name__=='__main__':unittest.main(verbosity=2)
