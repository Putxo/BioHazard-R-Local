import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import audit_genesis_hunter_materials as audit
class T(unittest.TestCase):
    def test_bad_image(self):
        with self.assertRaises(ValueError):audit.audit(b'MZ'+bytes(256))
    def test_private_exact_image(self):
        candidates=[
            Path('/mnt/data/rev_analysis/BioRevHD 30-Enero-2013(1).exe'),
            Path('/mnt/data/BioRevHD 30-Enero-2013(2).exe'),
        ]
        image=next((p for p in candidates if p.exists()),None)
        if image is None:self.skipTest('private January image absent in CI')
        result=audit.audit(image.read_bytes())
        self.assertGreaterEqual(result['checks'],27)
        self.assertTrue(result['hunter_material_parameters_per_view'])
        self.assertFalse(result['gameplay_executed'])
if __name__=='__main__':unittest.main(verbosity=2)
