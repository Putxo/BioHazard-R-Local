#!/usr/bin/env python3
"""Recreate the exact portable patch from two hash-pinned local images.

Only minimal changed-byte ranges and original helper additions enter the
payload. Neither executable is copied, executed, uploaded or overwritten.
The generated payload is only for the experimental candidate documented here.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import sys
from apply import SOURCE_SHA, OUTPUT_SHA, PAYLOAD_SHA, sha


def make_payload(source: bytes, target: bytes) -> tuple[bytes, dict]:
    if len(source) != 60_748_800 or sha(source) != SOURCE_SHA:
        raise ValueError('wrong original January SHA-256/size')
    if len(target) != 60_755_456 or sha(target) != OUTPUT_SHA:
        raise ValueError('wrong experimental candidate SHA-256/size')
    writes=[]
    start=None
    for i,(a,b) in enumerate(zip(source,target)):
        if a != b and start is None:
            start=i
        elif a == b and start is not None:
            writes.append({'offset':start,'old':source[start:i].hex(),'new':target[start:i].hex()})
            start=None
    if start is not None:
        writes.append({'offset':start,'old':source[start:].hex(),'new':target[start:len(source)].hex()})
    payload={
        'format':'revlocal-bytepatch-v1','status':'EXPERIMENTAL_NOT_GAMEPLAY_VALIDATED',
        'source_sha256':SOURCE_SHA,'source_size':len(source),
        'output_sha256':OUTPUT_SHA,'output_size':len(target),'writes':writes,
        'append_hex':target[len(source):].hex(),
        'assembly_helper_sha256':'500a82d8071f5c40ece76397fee1bac97888768a3e9029d52222d4d933eb7896',
        'gameplay_executed':False,
        'provenance':'Original January -> existing local-routing 0c019d43 -> guarded script members; preserves the published local-routing code and data sections byte-for-byte.'}
    raw=(json.dumps(payload,indent=2)+'\n').encode()
    if sha(raw) != PAYLOAD_SHA:
        raise ValueError('generated payload does not match the verified distribution')
    return raw,{'payload_sha256':PAYLOAD_SHA,'ranges':len(writes),
                'changed_original_bytes':sum(len(r['old'])//2 for r in writes),
                'appended_bytes':len(target)-len(source),'gameplay_executed':False}


def main() -> int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('original',type=Path);p.add_argument('candidate',type=Path)
    p.add_argument('payload',type=Path)
    a=p.parse_args()
    try:
        if len({x.resolve() for x in (a.original,a.candidate,a.payload)}) != 3:
            raise ValueError('all paths must be different')
        if a.payload.exists() or a.payload.is_symlink():raise ValueError('payload already exists')
        raw,report=make_payload(a.original.read_bytes(),a.candidate.read_bytes())
        with a.payload.open('xb') as handle:handle.write(raw)
        print(json.dumps(report));return 0
    except (OSError,ValueError) as e:
        print('ERROR: '+str(e),file=sys.stderr);return 2
if __name__=='__main__':raise SystemExit(main())
