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
import audit_hud_native_ops as audit

class NativeAuditTests(unittest.TestCase):
    def test_reject_other_image(self):
        for data in (b'',b'MZ'+bytes(500),b'not a game'):
            with self.subTest(size=len(data)),self.assertRaises(ValueError):audit.audit(data)
    def test_target_table_matches_cpp(self):
        source=(ROOT/'patches/hud_ownership/january_backend.cpp').read_text()
        rows=re.findall(r'\{(0x[0-9A-F]+(?:,0x[0-9A-F]+){7})\}',source)
        self.assertEqual(len(rows),3)
        for row,t in zip(rows,audit.TYPES):
            actual=tuple(int(v,16) for v in row.split(','))
            self.assertEqual(actual,(t[2],t[3],t[5],*t[7]))
    def test_report_alias_and_overwrite(self):
        with tempfile.TemporaryDirectory() as td:
            original=Path(td)/'input';report=Path(td)/'report.json'
            original.write_bytes(b'never modify');report.write_text('preserve')
            for destination in (original,report):
                with contextlib.redirect_stderr(io.StringIO()):
                    self.assertEqual(audit.main([str(original),'--report',str(destination)]),2)
            self.assertEqual(original.read_bytes(),b'never modify');self.assertEqual(report.read_text(),'preserve')
    def test_bad_input_no_output(self):
        with tempfile.TemporaryDirectory() as td:
            original=Path(td)/'input';report=Path(td)/'report.json';original.write_bytes(b'MZ')
            with contextlib.redirect_stderr(io.StringIO()):self.assertEqual(audit.main([str(original),'--report',str(report)]),2)
            self.assertFalse(report.exists())
    def test_dangling_destination(self):
        with tempfile.TemporaryDirectory() as td:
            original=Path(td)/'input';report=Path(td)/'report';original.write_bytes(b'MZ')
            report.symlink_to(Path(td)/'absent')
            with contextlib.redirect_stderr(io.StringIO()):self.assertEqual(audit.main([str(original),'--report',str(report)]),2)
            self.assertTrue(report.is_symlink());self.assertFalse(report.exists())
    def test_private_exact_original(self):
        name=os.environ.get('REV_JANUARY_ORIGINAL')
        if not name:self.skipTest('private original intentionally not available in CI')
        path=Path(name);before=path.read_bytes();report=audit.audit(before)
        self.assertEqual(report['checks'],69);self.assertEqual(report['input_sha256'],audit.SHA)
        self.assertFalse(report['gameplay_executed']);self.assertFalse(report['game_image_written'])
        with self.assertRaises(ValueError):audit.audit(before[:-1])
        modified=bytearray(before);modified[100]^=1
        with self.assertRaises(ValueError):audit.audit(bytes(modified))
        self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(),audit.SHA)
if __name__=='__main__':unittest.main(verbosity=2)
