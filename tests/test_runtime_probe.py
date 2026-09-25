#!/usr/bin/env python3
"""Sampler tests with fake process memory; Windows smoke test reads this process only."""
from __future__ import annotations
import ctypes, importlib.util, os
from pathlib import Path
import struct, unittest
spec=importlib.util.spec_from_file_location('probe',Path(__file__).parents[1]/'tools/runtime_probe.py')
p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)

class Memory:
    def __init__(self):self.bytes={};self.tracker_reads=0;self.flip=False
    def put(self,address,data):self.bytes.update({address+i:value for i,value in enumerate(data)})
    def read(self,address,n):
        if address==p.TRACKER:
            self.tracker_reads+=1
            if self.flip and self.tracker_reads>1:return struct.pack('<II',0,0)
        try:return bytes(self.bytes[address+i] for i in range(n))
        except KeyError as exc:raise OSError('unmapped fixture') from exc
    def u32(self,address,value):self.put(address,struct.pack('<I',value))

def fixture():
    m=Memory();sub=0x100000;main=0x120000;camera=0x200000;first=0x210000;second=0x220000;pad=0x300000
    m.put(p.TRACKER,struct.pack('<II',sub,1));m.u32(p.CAMERA,camera);m.u32(p.GAMEPAD,pad)
    for addr,serial,pack in ((sub,1,0x400000),(main,0,0x410000)):
        m.put(addr+0xE3C,struct.pack('<ii',serial,1));m.put(addr+0x40,struct.pack('<fff',1,2,3))
        m.put(addr+0x1670,struct.pack('<ffffff',.1,.2,.3,.4,.5,.6));m.u32(addr+0x1524,pack);m.u32(pack+0xC8,3)
    m.put(camera+0xCE0,struct.pack('<II',first,second));m.u32(first+0x74,main);m.u32(second+0x74,sub)
    m.put(camera+0x40,bytes([1,0,0,2,0]));m.put(camera+0x1D0,bytes([1,0,0,3,0]))
    m.u32(pad+0x970,0);m.put(pad+0x668,bytes(0x180))
    return m

class ProbeTests(unittest.TestCase):
    def test_two_actors(self):
        result=p.Sampler(fixture()).sample()
        self.assertTrue(result['local_split_structure_observed']);self.assertFalse(result['gameplay_certified'])
        self.assertEqual(result['sub0']['herbs'],3);self.assertEqual(result['self_camera_target_actor']['serial'],0)
    def test_network_is_not_local(self):
        m=fixture();m.put(0x100000+0xE40,struct.pack('<i',3))
        self.assertFalse(p.Sampler(m).sample()['local_split_structure_observed'])
    def test_offline_flag_off(self):
        m=fixture();m.u32(p.TRACKER+4,0)
        self.assertFalse(p.Sampler(m).sample()['local_split_structure_observed'])
    def test_camera_target_mismatch(self):
        m=fixture();m.u32(0x220000+0x74,0x120000)
        self.assertFalse(p.Sampler(m).sample()['local_split_structure_observed'])
    def test_camera_missing(self):
        m=fixture();m.u32(p.CAMERA,0)
        result=p.Sampler(m).sample();self.assertIsNone(result['camera']);self.assertFalse(result['local_split_structure_observed'])
    def test_actor_missing(self):
        m=fixture();m.u32(p.TRACKER,0)
        result=p.Sampler(m).sample();self.assertIsNone(result['sub0']);self.assertFalse(result['local_split_structure_observed'])
    def test_binding_changes(self):
        m=fixture();m.flip=True
        result=p.Sampler(m).sample();self.assertTrue(result['binding_changed_during_sample']);self.assertFalse(result['local_split_structure_observed'])
    def test_unreadable_actor(self):
        m=fixture();m.u32(p.TRACKER,0x500000)
        result=p.Sampler(m).sample();self.assertIn('read_error',result['sub0']);self.assertFalse(result['local_split_structure_observed'])
    def test_debug_fill_pointer(self):
        self.assertFalse(p.pointer(0xCCCCCCCC));self.assertFalse(p.pointer(1));self.assertFalse(p.pointer(0))
    def test_no_nan_json(self):
        self.assertEqual(p.floats(struct.pack('<ff',float('nan'),float('inf'))),[None,None])
    def test_minimal_read_access(self):self.assertEqual(p.READ_ACCESS,0x1010)
    @unittest.skipUnless(os.name=='nt','kernel32 smoke test requires Windows')
    def test_real_windows_reader_own_process_only(self):
        value=ctypes.create_string_buffer(b'REV-READ-ONLY-SMOKE')
        with p.WindowsReader(os.getpid()) as reader:
            self.assertTrue(reader.image_path().is_file())
            self.assertEqual(reader.read(ctypes.addressof(value),len(value)),value.raw)
        self.assertIsNone(reader.handle)
        reader.close() # Idempotent; no second CloseHandle.

if __name__=='__main__':unittest.main()
