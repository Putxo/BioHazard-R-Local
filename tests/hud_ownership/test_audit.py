#!/usr/bin/env python3
"""Filesystem refusal tests; optional BHR_ORIGINAL enables private-image checks."""
import contextlib
import io
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import audit_hud_lifetime as module

class AuditTests(unittest.TestCase):
    def test_refuses_bad_images(self):
        for data in (b'',b'MZ'+bytes(1024),b'not a game'):
            with self.assertRaises(ValueError):module.audit(data)
    def test_no_overwrite_or_alias(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);source=root/'original.dat';report=root/'report.json'
            source.write_bytes(b'source');report.write_bytes(b'existing')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(module.main([str(source),'--report',str(source)]),2)
                self.assertEqual(module.main([str(source),'--report',str(report)]),2)
            self.assertEqual(source.read_bytes(),b'source')
            self.assertEqual(report.read_bytes(),b'existing')
    def test_bad_input_leaves_no_report(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);source=root/'invalid.dat';report=root/'report.json'
            source.write_bytes(b'invalid')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(module.main([str(source),'--report',str(report)]),2)
            self.assertFalse(report.exists());self.assertEqual(source.read_bytes(),b'invalid')
    def test_dangling_symlink_not_followed(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);source=root/'original.dat';report=root/'report';target=root/'absent'
            source.write_bytes(b'source')
            try:report.symlink_to(target)
            except OSError:self.skipTest('symlinks unavailable')
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(module.main([str(source),'--report',str(report)]),2)
            self.assertTrue(report.is_symlink());self.assertFalse(target.exists())
    def test_exact_private_image(self):
        path=os.environ.get('BHR_ORIGINAL')
        if not path:self.skipTest('original not supplied; never uploaded to CI')
        data=Path(path).read_bytes();report=module.audit(data)
        self.assertEqual(report['checks'],94)
        self.assertFalse(report['hud_installed'])
        self.assertEqual(len(report['cockpit_list_slots']),21)
        self.assertNotIn(0x8c,report['cockpit_list_slots'])
        for modified in (data[:-1],data[:1024]+bytes([data[1024]^1])+data[1025:]):
            with self.assertRaises(ValueError):module.audit(modified)
        self.assertEqual(Path(path).read_bytes(),data)

if __name__=='__main__':unittest.main(verbosity=2)
