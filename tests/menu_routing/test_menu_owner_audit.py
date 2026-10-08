import sys,unittest,json,struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import audit_local_menu_owner as audit
import audit_options_input as options

class T(unittest.TestCase):
 def test_bad(self):
  with self.assertRaises(ValueError):audit.audit(b'MZ'+bytes(256))
 def test_private(self):
  candidates=[
   Path('/mnt/data/rev_analysis/BioRevHD 30-Enero-2013(1).exe'),
   Path('/mnt/data/BioRevHD 30-Enero-2013(2).exe')]
  p=next((x for x in candidates if x.exists()),None)
  if p is None:self.skipTest('private game image deliberately absent in CI')
  r=audit.audit(p.read_bytes());self.assertGreaterEqual(r['checks'],20)
 def test_options_map_matches_native_call_witnesses(self):
  root=Path(__file__).resolve().parents[2]
  rows=[x for x in json.loads((root/'patches/menu_routing/menu_sites.json').read_text())['sites'] if x['purpose'].startswith('Options ')]
  self.assertEqual(len(rows),25)
  fields={'0x01C5C120':'198','0x01B9B457':'1a0','0x01BD9F18':'1ac'}
  for row in rows:
   at=int(row['va'],16);target=row['original'].split()[1]
   self.assertEqual(row['bridge'],'rev_menu_gate_pause_'+fields[target])
   witness=next(bytes.fromhex(raw)[at-va:at-va+5] for va,raw,_ in options.SITES if va<=at and at+5<=va+len(bytes.fromhex(raw)))
   self.assertEqual(witness[0],0xE8)
   self.assertEqual(at+5+struct.unpack('<i',witness[1:])[0],int(target,16))
  with self.assertRaises(ValueError):options.audit(b'not an original image')
 def test_source_sites(self):
  root=Path(__file__).resolve().parents[2]
  s=(root/'patches/menu_routing/menu_sites.json').read_text()
  g=(root/'patches/menu_routing/menu_gateways.S').read_text()
  for x in ('0x01F3EF43','0x01F3EF92','0x02B6A491','0x02B6A49E','0x02B6DE91','0x02B6EFDE',
            '0x02C3183B','0x02C31848','0x02C31FDC','0x02C31FE9'):self.assertIn(x,s)
  for x in ('pause_open','pause_1a0','pause_1ac','submenu_open','submenu_198','submenu_1a0','submenu_actor'):self.assertIn(x,g)

if __name__=='__main__':unittest.main(verbosity=2)
