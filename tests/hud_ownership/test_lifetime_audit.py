import contextlib
import hashlib
import importlib.util
import io
import os
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('lifetime_audit', ROOT/'scripts/audit_lifetime_observer.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

class AuditTests(unittest.TestCase):
    def test_bad_identity(self):
        with self.assertRaises(ValueError): audit.audit(b'MZ'+bytes(4096))
    def test_bad_input_no_output(self):
        with tempfile.TemporaryDirectory() as td:
            source=Path(td)/'source'; report=Path(td)/'report.json'
            source.write_bytes(b'not a game image')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(audit.main([str(source),'--report',str(report)]),2)
            self.assertEqual(source.read_bytes(),b'not a game image')
            self.assertFalse(report.exists())
    def test_no_overwrite_or_alias(self):
        with tempfile.TemporaryDirectory() as td:
            source=Path(td)/'source';report=Path(td)/'report.json'
            source.write_bytes(b'input');report.write_bytes(b'existing')
            for destination in (source,report):
                with self.subTest(destination=destination),contextlib.redirect_stderr(io.StringIO()):
                    self.assertEqual(audit.main([str(source),'--report',str(destination)]),2)
            self.assertEqual(source.read_bytes(),b'input');self.assertEqual(report.read_bytes(),b'existing')
    def test_dangling_destination(self):
        with tempfile.TemporaryDirectory() as td:
            source=Path(td)/'source';report=Path(td)/'report.json'
            source.write_bytes(b'input')
            try: report.symlink_to(Path(td)/'absent')
            except OSError: self.skipTest('symlink not available')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(audit.main([str(source),'--report',str(report)]),2)
            self.assertTrue(report.is_symlink());self.assertEqual(source.read_bytes(),b'input')
    def test_correct_begin_precedes_branch(self):
        entry=next(s for s in audit.SITES if s[0]=='bind_begin')
        self.assertEqual(entry[1],0x02DF4F65)
        self.assertEqual(entry[1]+len(bytes.fromhex(entry[2])),0x02DF4F6C)
        self.assertNotIn(0x02DF4F99,[s[1] for s in audit.SITES])
    def test_source_map(self):
        if not (ROOT/'patches/hud_ownership/lifetime_source.cpp').exists():
            self.skipTest('full repository source not mounted locally; checked in CI')
        self.assertEqual(audit.source_map(ROOT)['sites'],11)
    def test_source_map_rejects_old_begin(self):
        folder=ROOT/'patches/hud_ownership'
        if not (folder/'lifetime_source.cpp').exists():
            self.skipTest('full repository source not mounted locally; checked in CI')
        with tempfile.TemporaryDirectory() as td:
            dest=Path(td)/'patches/hud_ownership';dest.mkdir(parents=True)
            for name in ('lifetime_gateways.S','lifetime_continuations.ld','lifetime_source.cpp'):
                text=(folder/name).read_text()
                if name=='lifetime_gateways.S':text=text.replace('0x02DF4F65,2','0x02DF4F99,2')
                (dest/name).write_text(text)
            with self.assertRaises(ValueError):audit.source_map(Path(td))
    def test_private_exact_original(self):
        name=os.environ.get('REV1_JANUARY_ORIGINAL')
        if not name:self.skipTest('private game image deliberately absent in CI')
        path=Path(name);before=hashlib.sha256(path.read_bytes()).hexdigest()
        result=audit.audit(path.read_bytes())
        self.assertEqual(result['checks'],46)
        self.assertEqual(result['sites'],11)
        self.assertTrue(result['inherited_binder_hooks_disjoint'])
        self.assertFalse(result['hooks_installed'])
        self.assertEqual(before,audit.SHA)
        self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(),before)

if __name__=='__main__':unittest.main(verbosity=2)
