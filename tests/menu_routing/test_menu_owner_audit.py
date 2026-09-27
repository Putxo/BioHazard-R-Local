import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import audit_local_menu_owner as audit
class T(unittest.TestCase):
 def test_bad(self):
  with self.assertRaises(ValueError):audit.audit(b'MZ'+bytes(256))
 def test_private(self):
  p=Path('/mnt/data/rev_analysis/BioRevHD 30-Enero-2013(1).exe')
  if not p.exists():self.skipTest('private game image deliberately absent in CI')
  r=audit.audit(p.read_bytes());self.assertGreaterEqual(r['checks'],20)
 def test_source_sites(self):
  root=Path(__file__).resolve().parents[2]
  s=(root/'patches/menu_routing/menu_sites.json').read_text()
  g=(root/'patches/menu_routing/menu_gateways.S').read_text()
  for x in ('0x01F3EF43','0x01F3EF92','0x02B6A491','0x02B6A49E','0x02B6DE91','0x02B6EFDE'):self.assertIn(x,s)
  for x in ('pause_open','submenu_open','submenu_198','submenu_1a0','submenu_actor'):self.assertIn(x,g)
if __name__=='__main__':unittest.main(verbosity=2)
