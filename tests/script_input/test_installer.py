"""Hook and audit contract without loading a game image."""
from pathlib import Path
import sys,unittest
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'));sys.path.insert(0,str(ROOT/'scripts'))
import runtime_image as image
import audit_script_input as audit
class InstallerTests(unittest.TestCase):
    def test_callback_has_a_single_full_entry_replacement(self):
        hits=[h for h in image.hook_sites() if h[2]=='rev_script_input_gate']
        self.assertEqual(hits,[(0x02DEE8D0,bytes.fromhex('558bec81eccc000000'),'rev_script_input_gate',0xE9)])
        self.assertTrue(any(a==hits[0][0] and bytes.fromhex(b)==hits[0][1] for a,b,_ in audit.SITES))
    def test_audit_is_bound_to_original_and_has_unique_sites(self):
        with self.assertRaisesRegex(ValueError,'exact original'):audit.audit(b'unsupported image')
        self.assertEqual(len(audit.SITES),len({a for a,_,_ in audit.SITES}))
        self.assertIn((0x02DEE8FB,'c20400','thiscall ret4'),audit.SITES)
if __name__=='__main__':unittest.main(verbosity=2)
