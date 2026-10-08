"""Check all scoped MOV descriptors; the ABI harness executes the mod gateways."""
from collections import Counter
from pathlib import Path
import sys,unittest
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'));sys.path.insert(0,str(ROOT/'scripts'))
import input_owner_sites as sites
import runtime_image
import audit_keyboard_owner as audit

class InputOwnerTests(unittest.TestCase):
    def test_all_native_register_pairs_have_the_correct_gateway(self):
        registers={'eax':0,'ecx':1,'edx':2,'esi':6}
        counts=Counter()
        for at,raw,name in sites.SITES:
            dest,base=name.split('_');counts[name]+=1
            self.assertEqual(bytes.fromhex(raw),bytes([0x8B,0x80+registers[dest]*8+registers[base],0x70,9,0,0]))
            self.assertIn((at,raw,'scoped load '+name),audit.SITES)
        self.assertEqual(counts,{'ecx_eax':93,'edx_ecx':60,'esi_eax':21,'ecx_edx':21,'eax_edx':1})
        self.assertEqual(196,len({at for at,_,_ in sites.SITES}))
    def test_installer_covers_scope_entries_and_every_index_load(self):
        mapped={at:(raw,name,op) for at,raw,name,op in runtime_image.hook_sites()}
        for at,raw,name,op in sites.hooks():self.assertEqual(mapped[at],(raw,name,op))
        for at,name in ((0x02DAC787,'device'),(0x02DAC8A1,'keyboard')):
            raw,symbol,op=mapped[at]
            self.assertEqual(symbol,'rev_input_update_'+name);self.assertEqual(op,0xE8)
            self.assertTrue(any(p==at and bytes.fromhex(b)==raw for p,b,_ in audit.SITES))
    def test_loads_are_confined_to_four_audited_routines(self):
        regions=((0x02DB3210,0x02DB33D3,2),(0x02DB3590,0x02DB561C,190),
                 (0x02DB3450,0x02DB349B,1),(0x02DAF2E0,0x02DAF389,3))
        for begin,end,count in regions:
            self.assertEqual(count,sum(begin<=at<end for at,_,_ in sites.SITES))
        with self.assertRaises(ValueError):audit.audit(b'unsupported image')

if __name__=='__main__':unittest.main(verbosity=2)
