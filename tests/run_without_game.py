#!/usr/bin/env python3
"""Public-source checks: host logic and restricted x86 bridges, no game files."""
from __future__ import annotations
import argparse, importlib.util, json, subprocess, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--sanitize',action='store_true');ap.add_argument('--i386-host',action='store_true');args=ap.parse_args()
    builder=load('builder',ROOT/'patches/build_local_routing.py');bridges=load('bridges',ROOT/'tests/test_bridges.py')
    with tempfile.TemporaryDirectory(prefix='rev-public-tests-') as temp:
        work=Path(temp);module,symbols,provenance=builder.compile_module(work)
        cmd=['g++','-std=c++17','-O1' if args.sanitize else '-O2','-Wall','-Wextra','-Werror','-fno-strict-aliasing']
        if args.sanitize:cmd+=['-g','-fno-omit-frame-pointer','-fsanitize=address,undefined']
        if args.i386_host:cmd+=['-m32']
        cmd += [str(ROOT/'tests/local_routing_host.cpp'),'-o',str(work/'host')]
        subprocess.run(cmd,check=True)
        host=json.loads(subprocess.check_output([str(work/'host')],text=True))
        class ModuleReader:
            def read(self,va,n):
                for address,blob in module.values():
                    if address<=va and va+n<=address+len(blob):return blob[va-address:va-address+n]
                raise ValueError('read outside compiled helper')
        cases=bridges.tests(ModuleReader(),symbols)
        result={'status':'PASS','host':host,'host_architecture':'i386' if args.i386_host else 'native',
                'sanitizers':args.sanitize,'bridge_scenarios':cases,'bridge_assertions':bridges.CHECKS,
                'game_binary_used':False,'gameplay_executed':False,
                'bridge_model':'only new assembly; engine and C++ calls stubbed',
                'source_sha256':provenance['source_sha256']}
        print(json.dumps(result,indent=2))
if __name__=='__main__':main()
