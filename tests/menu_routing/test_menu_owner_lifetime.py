import json,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import audit_menu_owner_lifetime as audit
class T(unittest.TestCase):
 def test_bad(self):
  with self.assertRaises(ValueError):audit.audit(b'MZ'+bytes(256))
 def test_private(self):
  candidates=[Path('/mnt/data/BioRevHD 30-Enero-2013(2).exe'),Path('/mnt/data/rev_analysis/BioRevHD 30-Enero-2013(1).exe')]
  p=next((x for x in candidates if x.exists()),None)
  if p is None:self.skipTest('private game image deliberately absent in CI')
  self.assertEqual(audit.audit(p.read_bytes())['checks'],8)
 def test_source_map(self):
  root=Path(__file__).resolve().parents[2]
  sites=json.loads((root/'patches/menu_routing/menu_sites.json').read_text())
  row=next(x for x in sites['sites'] if x['va']=='0x01C365BF')
  self.assertEqual(row['bridge'],'rev_menu_gate_state_transition')
  gates=(root/'patches/menu_routing/menu_gateways.S').read_text()
  self.assertIn('rev_menu_gate_state_transition',gates)
  self.assertIn('rev_menu_stock_state_transition',gates)
if __name__=='__main__':unittest.main(verbosity=2)
