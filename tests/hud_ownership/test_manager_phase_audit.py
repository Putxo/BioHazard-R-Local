import contextlib
import hashlib
import io
import os
from pathlib import Path
import re
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
import audit_hud_manager_phases as audit

class AuditTests(unittest.TestCase):
    def test_private_source(self):
        path=os.environ.get('REV_JANUARY_ORIGINAL')
        if not path:self.skipTest('private source image not provided in CI')
        source=Path(path);before=hashlib.sha256(source.read_bytes()).hexdigest()
        result=audit.audit(source.read_bytes())
        self.assertEqual(result['checks'],31)
        self.assertFalse(result['hooks_installed'])
        self.assertEqual(before,hashlib.sha256(source.read_bytes()).hexdigest())
    def test_unknown_image(self):
        for data in (b'',b'MZ'+bytes(1024),bytes(1024)):
            with self.subTest(size=len(data)),self.assertRaises(ValueError):audit.audit(data)
    def test_source_site_tables(self):
        cpp=(ROOT/'patches/hud_ownership/manager_driver.cpp').read_text()
        asm=(ROOT/'patches/hud_ownership/manager_gateways.S').read_text()
        linker=(ROOT/'patches/hud_ownership/manager_continuations.ld').read_text()
        for address,family,phase,hex_bytes in audit.SITES:
            self.assertIn(f'0x{address:08X}',cpp)
            self.assertIn(f'0x{address:08X}',asm)
            pattern=rf'rev_hud_continue_{family}{phase}\s*=\s*0x([0-9a-fA-F]+)'
            match=re.search(pattern,linker);self.assertIsNotNone(match)
            self.assertEqual(int(match[1],16),address+len(bytes.fromhex(hex_bytes)))
        self.assertNotIn('0x02B687F0',cpp)
    def test_cli_preserves_input_and_no_error_output(self):
        with tempfile.TemporaryDirectory() as td:
            source=Path(td)/'source';report=Path(td)/'report'
            source.write_bytes(b'wrong source')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(audit.main([str(source),'--report',str(report)]),2)
                self.assertEqual(audit.main([str(source),'--report',str(source)]),2)
            self.assertEqual(source.read_bytes(),b'wrong source');self.assertFalse(report.exists())
    def test_existing_and_dangling_report(self):
        with tempfile.TemporaryDirectory() as td:
            source=Path(td)/'source';report=Path(td)/'report'
            source.write_bytes(b'wrong source');report.write_text('existing')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(audit.main([str(source),'--report',str(report)]),2)
            self.assertEqual(report.read_text(),'existing');report.unlink()
            report.symlink_to(Path(td)/'missing')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(audit.main([str(source),'--report',str(report)]),2)
            self.assertTrue(report.is_symlink());self.assertFalse(report.exists())
if __name__=='__main__':unittest.main(verbosity=2)
