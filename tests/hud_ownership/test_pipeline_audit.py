import contextlib
import importlib.util
import io
import os
from pathlib import Path
import shutil
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
SPEC=importlib.util.spec_from_file_location('audit_pipeline_frame',ROOT/'scripts/audit_pipeline_frame.py')
audit=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(audit)

class AuditTests(unittest.TestCase):
    def test_source_map(self):
        self.assertEqual(audit.audit_sources(ROOT)['sites'],2)
    def test_source_map_rejects_wrong_boundary(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);folder=root/'patches/hud_ownership';folder.mkdir(parents=True)
            for name in ('pipeline_clock.hpp','pipeline_gateways.S','pipeline_continuations.ld'):
                shutil.copyfile(ROOT/'patches/hud_ownership'/name,folder/name)
            path=folder/'pipeline_gateways.S'
            path.write_text(path.read_text().replace('0x02F506A0','0x033BFE56'))
            with self.assertRaises(ValueError):audit.audit_sources(root)
    def test_source_map_rejects_wrong_continuation(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);folder=root/'patches/hud_ownership';folder.mkdir(parents=True)
            for name in ('pipeline_clock.hpp','pipeline_gateways.S','pipeline_continuations.ld'):
                shutil.copyfile(ROOT/'patches/hud_ownership'/name,folder/name)
            path=folder/'pipeline_continuations.ld'
            path.write_text(path.read_text().replace('0x02F50F54','0x02F50F5E'))
            with self.assertRaises(ValueError):audit.audit_sources(root)
    def test_private_exact_original(self):
        original=os.environ.get('REVELATIONS_JANUARY_ORIGINAL')
        if not original:self.skipTest('private original deliberately absent in CI')
        data=Path(original).read_bytes();report=audit.audit(data)
        self.assertEqual(report['checks'],31)
        self.assertEqual(report['body']['sha256'],audit.BODY_SHA)
        self.assertEqual(Path(original).read_bytes(),data)
        image=audit.Image(data)
        for va,size in ((0xFFFFFFFF,8),(0x1000,4),(0x400000,-1)):
            with self.assertRaises(ValueError):image.read(va,size)
    def test_other_input(self):
        for data in (b'',b'MZ'+bytes(510),b'not a game'):
            with self.assertRaises(ValueError):audit.audit(data)
    def test_bad_input_no_output(self):
        with tempfile.TemporaryDirectory() as temp:
            source=Path(temp)/'source';out=Path(temp)/'report.json'
            source.write_bytes(b'unchanged')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(audit.main([str(source),'--report',str(out)]),2)
            self.assertFalse(out.exists());self.assertEqual(source.read_bytes(),b'unchanged')
    def test_no_overwrite_or_alias(self):
        with tempfile.TemporaryDirectory() as temp:
            source=Path(temp)/'source';out=Path(temp)/'report.json'
            source.write_bytes(b'source');out.write_bytes(b'existing')
            for destination in (out,source):
                with contextlib.redirect_stderr(io.StringIO()):
                    self.assertEqual(audit.main([str(source),'--report',str(destination)]),2)
            self.assertEqual(out.read_bytes(),b'existing');self.assertEqual(source.read_bytes(),b'source')
    def test_dangling_destination(self):
        with tempfile.TemporaryDirectory() as temp:
            source=Path(temp)/'source';out=Path(temp)/'report.json'
            source.write_bytes(b'source')
            try:out.symlink_to(Path(temp)/'absent')
            except OSError:self.skipTest('symlink unavailable')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(audit.main([str(source),'--report',str(out)]),2)
            self.assertTrue(out.is_symlink());self.assertFalse((Path(temp)/'absent').exists())
if __name__=='__main__':unittest.main(verbosity=2)
