import unittest
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import audit_allocation_ledger as a
class T(unittest.TestCase):
 def test_reject_bad(self):
  with self.assertRaises(ValueError):a.audit(b'MZ'+bytes(100))
 def test_private(self):
  p=Path('/mnt/data/rev_analysis/BioRevHD 30-Enero-2013(1).exe')
  if not p.exists():self.skipTest('private game image deliberately absent in CI')
  self.assertEqual(a.audit(p.read_bytes())['checks'],6)
 def test_source_specs_match_backend(self):
  root=Path(__file__).resolve().parents[2]
  ledger=(root/'patches/hud_ownership/allocation_ledger.cpp').read_text()
  backend=(root/'patches/hud_ownership/january_backend.cpp').read_text()
  for row in (
   '0x2E0,0x01C11706,0x01C19CCB,0x01C40C04',
   '0x370,0x01C15775,0x01B7D1F0,0x01C0C74C',
   '0x2C0,0x01C7855A,0x01C6392A,0x01BAB4F6'):
   self.assertIn(row,ledger);self.assertIn(row,backend)
if __name__=='__main__':unittest.main(verbosity=2)
