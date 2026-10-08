"""Validate the linked mod, without any game image or game execution."""
import os
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
import runtime_image as image

class InputBaseTests(unittest.TestCase):
    def test_old_cpu2_bases_fail_before_module_loading(self):
        for digest in image.LEGACY_CPU2_BASES:
            with self.subTest(digest=digest), patch.object(image,'sha',return_value=digest):
                with self.assertRaisesRegex(ValueError,'leaves J2 in AI'):
                    image.build(b'input placeholder',b'not an ELF')
        self.assertTrue(image.LEGACY_CPU2_BASES.isdisjoint(image.BASES))
    def test_old_suspension_losing_bases_fail_before_module_loading(self):
        for digest in image.LEGACY_REBIND_BASES:
            with self.subTest(digest=digest), patch.object(image,'sha',return_value=digest):
                with self.assertRaisesRegex(ValueError,'loses suspended J2 ownership'):
                    image.build(b'input placeholder',b'not an ELF')
        self.assertTrue(image.LEGACY_REBIND_BASES.isdisjoint(image.BASES))

class ImageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        path=os.environ.get('REV_RUNTIME_ELF')
        if not path:raise unittest.SkipTest('set REV_RUNTIME_ELF to the linked mod (not a game)')
        cls.data=Path(path).read_bytes()
    def test_all_gateways_are_in_executable_section(self):
        sections,symbols=image.read_module(self.data)
        va,code=sections['.revtext']
        sites=image.hook_sites()
        self.assertEqual(426,len(sites))
        occupied=set()
        for at,old,name,opcode in sites:
            self.assertIn(opcode,(0xE8,0xE9))
            self.assertGreaterEqual(len(old),5)
            self.assertGreaterEqual(symbols[name],va)
            self.assertLess(symbols[name],va+len(code))
            self.assertFalse(occupied.intersection(range(at,at+len(old))))
            occupied.update(range(at,at+len(old)))
        for at,old,new,name in image.inline_input_sites():
            self.assertEqual(len(old),len(new))
            self.assertFalse(occupied.intersection(range(at,at+len(old))))
            occupied.update(range(at,at+len(old)))
        for name in ('rev_hud_mask_exit_cockpit11','rev_hud_mask_exit_minimap11'):
            self.assertNotEqual(0,symbols[name])
    def test_rejects_undefined_mask_exit(self):
        # Reproduce the old nonallocated .note.GNU-stack placement: ld.lld
        # linked successfully but emitted the discarded gateway as SHN_UNDEF.
        d=bytearray(self.data);off=struct.unpack_from('<I',d,32)[0]
        size,count,_=struct.unpack_from('<HHH',d,46)
        entries=[struct.unpack_from('<10I',d,off+i*size) for i in range(count)]
        table=next(e for e in entries if e[1]==2);strings=entries[table[6]]
        names=d[strings[4]:strings[4]+strings[5]];changed=False
        for p in range(table[4],table[4]+table[5],16):
            ni=struct.unpack_from('<I',d,p)[0]
            if names[ni:].split(b'\0',1)[0]==b'rev_hud_mask_exit_cockpit11':
                struct.pack_into('<H',d,p+14,0);changed=True;break
        self.assertTrue(changed)
        with self.assertRaisesRegex(ValueError,'unresolved symbol rev_hud_mask_exit_cockpit11'):image.read_module(bytes(d))
    def test_rejects_bad_architecture_and_tables(self):
        for offset,value in ((4,2),(18,62),(46,0)):
            with self.subTest(offset=offset):
                d=bytearray(self.data);d[offset]=value
                with self.assertRaises(ValueError):image.read_module(bytes(d))
    def test_rejects_unpinned_game(self):
        with self.assertRaisesRegex(ValueError,'unsupported input SHA256'):image.build(bytes(2048),self.data)

if __name__=='__main__':unittest.main(verbosity=2)

