import unittest
from pathlib import Path

class SourceMap(unittest.TestCase):
    def test_production_bindings(self):
        root=Path(__file__).resolve().parents[2]
        h=(root/'patches/hud_ownership/structural_coordinator.hpp').read_text()
        c=(root/'patches/hud_ownership/structural_coordinator.cpp').read_text()
        for text in ('LifetimeSource& source_', 'Lifecycle& life_',
                     'JanuaryBackend& backend_', 'ManagerDriver& driver_',
                     'JanuaryAllocationLedger& ledger_'):
            self.assertIn(text,h)
        for text in ('source_.capture', 'life_.prepare', 'life_.publish',
                     'backend_.configure', 'driver_.stop', 'ledger_.retained'):
            self.assertIn(text,c)

    def test_original_slots_and_alias(self):
        root=Path(__file__).resolve().parents[2]
        c=(root/'patches/hud_ownership/structural_coordinator.cpp').read_text()
        for text in ('0x90', '0x5C', '0x30', '0x40'):
            self.assertIn(text,c)

    def test_no_installer_or_process_patch(self):
        root=Path(__file__).resolve().parents[2]
        c=(root/'patches/hud_ownership/structural_coordinator.cpp').read_text().lower()
        for text in ('writeprocessmemory','virtualprotect','createprocess','loadlibrary'):
            self.assertNotIn(text,c)

    def test_attach_is_deliberately_absent(self):
        root=Path(__file__).resolve().parents[2]
        c=(root/'patches/hud_ownership/structural_coordinator.cpp').read_text()
        self.assertNotIn('.attach(',c)
        self.assertNotIn('driver_.attach',c)

if __name__=='__main__':
    unittest.main(verbosity=2)
