"""Execute only replacement bytes with synthetic raw-pad readers, never game code."""
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from pad_axis_sites import patches, inspect_axes
from test_bridges import Machine
from pad_state_sites import patches as state_patches

class Reader:
    def __init__(self, code): self.code = code
    def read(self, at, size):
        if not 0 <= at <= at + size <= len(self.code): raise ValueError('outside test snippet')
        return self.code[at:at+size]

class AxisTests(unittest.TestCase):
    def transport(self, replacement, register, index, primary, values, flag):
        # The continuation is a synthetic call transporting the selected register.
        # Stub all downstream engine behavior; no bytes from an EXE are executed.
        code = replacement + (b'\x52' if register == 'edx' else b'\x51')
        code += b'\xe8' + (0x100 - 12).to_bytes(4, 'little') + b'\xc3'
        m = Machine(Reader(code), {'snippet': 0})
        m.write(m.r['ebp']+8, index)
        m.write(m.r['ecx']+0x970, primary)
        m.write(m.r['eax']+0x970, primary)
        before = dict(m.mem); m.zf = flag
        selected = []
        def raw(machine, args):
            selected.append(args[0])
            self.assertEqual(machine.zf, flag)
            return values[args[0]]
        m.stub(0x100, nargs=1, cleanup=4, callback=raw)
        sp, _ = m.run('snippet')
        self.assertEqual(m.r['esp'], sp)
        self.assertTrue(all(m.mem[k] == v for k,v in before.items()))
        return selected, m.r['eax']

    def test_independent_sticks_and_dpad_reads(self):
        for va, old, new, label in patches():
            reg = 'edx' if old[1] == 0x91 else 'ecx'
            for index in (0,1):
                for primary in (0,1):
                    for values in ((0,32767),(-32768,0),(1234,-5678),(0x2000,0x8000),(0x1000,0x4000)):
                        for flag in (False,True):
                            with self.subTest(site=hex(va),index=index,primary=primary,values=values,flag=flag):
                                selected, result = self.transport(new,reg,index,primary,values,flag)
                                self.assertEqual(selected,[index])
                                self.assertEqual(result,values[index]&0xffffffff)

    def test_reproduces_old_j2_neutral_and_mirrored_j1(self):
        for va,old,_,label in patches():
            reg = 'edx' if old[1] == 0x91 else 'ecx'
            for values in ((0,32767),(12345,0)):
                with self.subTest(site=hex(va),values=values):
                    selected,result = self.transport(old,reg,1,0,values,False)
                    self.assertEqual(selected,[0])
                    self.assertEqual(result,values[0])
                    self.assertNotEqual(result,values[1])

    def test_per_slot_state_queries(self):
        for va,old,new,label in state_patches():
            for index in (0,1):
                for primary in (0,1):
                    for values in ((0,1),(1,0)):
                        selected,result=self.transport(new,'ecx',index,primary,values,True)
                        self.assertEqual(selected,[index]);self.assertEqual(result,values[index])

    def test_inspector_detects_old_mixed_and_modified_sites(self):
        class Image:
            def __init__(self, corrected):
                self.data={at:(new if corrected else old) for at,old,new,_ in patches()}
            def read(self,at,n):return self.data[at][:n]
        self.assertEqual(inspect_axes(Image(False))['status'],'LEGACY_GLOBAL_AXES')
        self.assertEqual(inspect_axes(Image(True))['status'],'PAD_ARGUMENTS_PRESENT')
        for at,old,new,_ in patches():
            image=Image(True);image.data[at]=old
            self.assertEqual(inspect_axes(image)['status'],'MIXED_OR_UNKNOWN_AXES')
            for byte in range(6):
                image=Image(True);bad=bytearray(new);bad[byte]^=0x80;image.data[at]=bytes(bad)
                self.assertEqual(inspect_axes(image)['status'],'MIXED_OR_UNKNOWN_AXES')

if __name__=='__main__':unittest.main(verbosity=2)
