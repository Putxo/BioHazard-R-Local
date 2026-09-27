import json
import os
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import audit_pause_owner as audit


class PauseLifetimeAudit(unittest.TestCase):
    def test_incompatible_images_rejected(self):
        for data in (b'', b'MZ' + bytes(512)):
            with self.assertRaises(ValueError):
                audit.audit(data)

    def test_site_map_matches_verified_targets(self):
        data = json.loads((ROOT / 'patches/menu_routing/menu_sites.json').read_text(encoding='utf-8-sig'))
        sites = {int(row['va'], 16): row for row in data['sites']}
        self.assertEqual(len(data['sites']), len(sites))
        for address, (bridge, target) in audit.PAUSE_CALLS.items():
            self.assertEqual(sites[address]['bridge'], bridge)
            self.assertEqual(sites[address]['original'].lower(), f'call 0x{target:08x}')
        commit = sites[0x02CDD415]
        self.assertEqual(commit['kind'], 'inline_jump')
        self.assertEqual(commit['overwrite_size'], 9)
        self.assertEqual(commit['continuation'], '0x02CDD41E')
        self.assertEqual(commit['original_bytes'], '8b45f88b4d08894824')
        self.assertEqual(commit['bridge'], 'rev_menu_gate_state_commit')

    def test_private_image(self):
        path = os.environ.get('RE_REV_JANUARY')
        if not path:
            self.skipTest('set RE_REV_JANUARY to the local original; never upload it')
        report = audit.audit(Path(path).read_bytes())
        self.assertEqual(report['checks'], 19)
        self.assertFalse(report['game_image_modified'])
        self.assertFalse(report['gameplay_executed'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
