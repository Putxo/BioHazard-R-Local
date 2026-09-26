import unittest
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import audit_native_view_scope as audit
class T(unittest.TestCase):
 def test_bad(self):
  with self.assertRaises(ValueError):audit.audit(b'MZ'+bytes(64))
 def test_private(self):
  p=Path('/mnt/data/rev_analysis/BioRevHD 30-Enero-2013(1).exe')
  if not p.exists():self.skipTest('private game image deliberately absent in CI')
  r=audit.audit(p.read_bytes());self.assertEqual(r['checks'],15)
 def test_source_map(self):
  root=Path(__file__).resolve().parents[2]
  s=(root/'patches/hud_ownership/native_view_scope.cpp').read_text()
  cam=(root/'research/patches/camera_persistence_v8.S').read_text()
  for x in ('0x04F79014','0x05799D3C','0x04EC9AA8','View1Offset=0x30+0x190','0x158','0xBC'):
   self.assertIn(x,s)
  self.assertIn('mov byte ptr [edi+0x1D3], 3',cam)
  self.assertIn('push 1',cam)
if __name__=='__main__':unittest.main(verbosity=2)
