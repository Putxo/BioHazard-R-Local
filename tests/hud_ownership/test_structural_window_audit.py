import unittest
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import audit_structural_window as audit
class T(unittest.TestCase):
 def test_bad(self):
  with self.assertRaises(ValueError):audit.audit(b'MZ'+bytes(64))
 def test_private(self):
  p=Path('/mnt/data/rev_analysis/BioRevHD 30-Enero-2013(1).exe')
  if not p.exists():self.skipTest('private game image deliberately absent in CI')
  r=audit.audit(p.read_bytes());self.assertEqual(r['checks'],12);self.assertTrue(r['cpu_render_worker_drained']);self.assertFalse(r['gpu_fence_proven'])
 def test_source_map(self):
  root=Path(__file__).resolve().parents[2]
  g=(root/'patches/hud_ownership/pipeline_gateways.S').read_text()
  s=(root/'patches/hud_ownership/structural_window.cpp').read_text()
  for x in ('rev_hud_structural_event','.if \\ending','test eax,eax'):
   self.assertIn(x,g)
  for x in ('PipelineEnd','clock_.quiescent()','task_.run','completed_frame()'):
   self.assertIn(x,s)
if __name__=='__main__':unittest.main(verbosity=2)
