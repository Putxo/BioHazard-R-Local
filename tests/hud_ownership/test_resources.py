import contextlib
import hashlib
import io
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import audit_hud_resources as audit

class ResourceAuditTests(unittest.TestCase):
    def test_empty_and_non_pe(self):
        for data in (b'', b'bad', b'MZ'+bytes(100)):
            with self.subTest(data=data[:4]),self.assertRaises(ValueError):audit.Image(data)
    def test_hash_required(self):
        for data in (b'MZ', bytes(512)):
            with self.assertRaises(ValueError):audit.audit(data)
    def test_wrong_source_creates_no_report(self):
        with tempfile.TemporaryDirectory() as td:
            src=Path(td)/'input';out=Path(td)/'report';src.write_bytes(b'not game')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(audit.main([str(src),'--report',str(out)]),2)
            self.assertFalse(out.exists());self.assertEqual(src.read_bytes(),b'not game')
    def test_existing_report_and_original_refused(self):
        with tempfile.TemporaryDirectory() as td:
            src=Path(td)/'input';out=Path(td)/'report';src.write_bytes(b'original');out.write_bytes(b'existing')
            for target in (src,out):
                with contextlib.redirect_stderr(io.StringIO()):
                    self.assertEqual(audit.main([str(src),'--report',str(target)]),2)
            self.assertEqual(src.read_bytes(),b'original');self.assertEqual(out.read_bytes(),b'existing')
    def test_dangling_report_link_refused(self):
        with tempfile.TemporaryDirectory() as td:
            src=Path(td)/'input';out=Path(td)/'report';src.write_bytes(b'original')
            out.symlink_to(Path(td)/'absent')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(audit.main([str(src),'--report',str(out)]),2)
            self.assertTrue(out.is_symlink());self.assertFalse((Path(td)/'absent').exists())
    def test_exact_private_original(self):
        name=os.environ.get('REV_JANUARY_ORIGINAL')
        if not name:self.skipTest('private original not supplied; never uploaded to CI')
        path=Path(name);before=path.read_bytes();result=audit.audit(before)
        self.assertEqual(result['checks'],52);self.assertFalse(result['gameplay_executed'])
        self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(),audit.ORIGINAL_SHA)
        changed=bytearray(before);changed[-1]^=1
        with self.assertRaises(ValueError):audit.audit(changed)

if __name__=='__main__':unittest.main(verbosity=2)
