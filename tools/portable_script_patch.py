#!/usr/bin/env python3
"""Build or apply a hash-pinned local patch manifest. Never executes the game.

The manifest is distributed with the experimental candidate. Building it needs
only the user's exact original and the independently verified candidate; it
never uploads either. Applying it needs standard Python 3, not a compiler.
"""
from __future__ import annotations
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import sys
import zlib

SOURCE_SHA='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
OUTPUT_SHA='71f5e70dc19c334d303ee29a686d65a72cd18b0c87b98472170bff961cb048f4'
SOURCE_SIZE=60_748_800
OUTPUT_SIZE=60_755_456

def sha(data):return hashlib.sha256(data).hexdigest()

def make_manifest(source,output):
    if len(source)!=SOURCE_SIZE or sha(source)!=SOURCE_SHA:raise ValueError('wrong original')
    if len(output)!=OUTPUT_SIZE or sha(output)!=OUTPUT_SHA:raise ValueError('wrong candidate')
    # Only differing bytes are represented, never whole unmodified game regions.
    records=[];start=None
    for i in range(len(source)):
        changed=source[i]!=output[i]
        if changed and start is None:start=i
        if start is not None and (not changed or i==len(source)-1):
            end=i if not changed else i+1
            records.append([start,source[start:end].hex(),output[start:end].hex()]);start=None
    return dict(format='BioHazard-R-Local-patch-1',source_sha256=SOURCE_SHA,
        output_sha256=OUTPUT_SHA,source_size=SOURCE_SIZE,output_size=OUTPUT_SIZE,
        status='EXPERIMENTAL_NOT_GAMEPLAY_VALIDATED',changes=records,
        append_zlib_base64=base64.b64encode(zlib.compress(output[len(source):],9)).decode('ascii'))

def apply(source,manifest):
    if len(source)!=SOURCE_SIZE or sha(source)!=SOURCE_SHA:
        raise ValueError('requires the exact unmodified January 2013 executable')
    if (manifest.get('format'),manifest.get('source_sha256'),manifest.get('output_sha256'),
        manifest.get('source_size'),manifest.get('output_size')) != (
        'BioHazard-R-Local-patch-1',SOURCE_SHA,OUTPUT_SHA,SOURCE_SIZE,OUTPUT_SIZE):
        raise ValueError('unrecognized manifest identity')
    data=bytearray(source);last_end=0
    for record in manifest['changes']:
        if not isinstance(record,list) or len(record)!=3:raise ValueError('bad patch record')
        start,old_hex,new_hex=record
        old,new=bytes.fromhex(old_hex),bytes.fromhex(new_hex)
        if not isinstance(start,int) or not old or len(old)!=len(new) or start<last_end or start+len(old)>len(source):
            raise ValueError('unordered, overlapping or out-of-range patch')
        if data[start:start+len(old)]!=old:raise ValueError('original bytes mismatch')
        data[start:start+len(new)]=new;last_end=start+len(old)
    compressed=base64.b64decode(manifest['append_zlib_base64'],validate=True)
    if len(compressed)>OUTPUT_SIZE-SOURCE_SIZE+1024:raise ValueError('oversized payload')
    decoder=zlib.decompressobj()
    extra=decoder.decompress(compressed,OUTPUT_SIZE-SOURCE_SIZE+1)
    if len(extra)!=OUTPUT_SIZE-SOURCE_SIZE or not decoder.eof or decoder.unused_data:
        raise ValueError('invalid appended payload')
    data.extend(extra)
    if len(data)!=OUTPUT_SIZE or sha(data)!=OUTPUT_SHA:raise ValueError('candidate hash mismatch')
    # Independently reverse before permitting an output file to be created.
    reverse=bytearray(data[:SOURCE_SIZE])
    for start,old,new in manifest['changes']:reverse[start:start+len(bytes.fromhex(old))]=bytes.fromhex(old)
    if bytes(reverse)!=source:raise ValueError('reversibility failed')
    return bytes(data)

def exclusive_write(path,payload):
    created=False
    try:
        with path.open('xb') as f:
            created=True;f.write(payload);f.flush();os.fsync(f.fileno())
    except Exception:
        if created:path.unlink(missing_ok=True)
        raise

def main(argv=None):
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode',choices=('make','apply'))
    p.add_argument('original',type=Path)
    p.add_argument('output',type=Path,help='candidate for make; new candidate destination for apply')
    p.add_argument('--manifest',required=True,type=Path)
    a=p.parse_args(argv)
    try:
        paths=(a.original,a.output,a.manifest)
        if len({v.resolve() for v in paths})!=3:raise ValueError('all paths must be different')
        destination=a.manifest if a.mode=='make' else a.output
        if os.path.lexists(destination):raise ValueError('destination exists; overwrite refused')
        source=a.original.read_bytes()
        if a.mode=='make':
            manifest=make_manifest(source,a.output.read_bytes())
            if apply(source,manifest)!=a.output.read_bytes():raise ValueError('manifest roundtrip failed')
            payload=(json.dumps(manifest,separators=(',',':'))+'\n').encode('utf8')
        else:
            if a.manifest.stat().st_size>2_000_000:raise ValueError('manifest too large')
            payload=apply(source,json.loads(a.manifest.read_text(encoding='utf8')))
        exclusive_write(destination,payload)
        print(json.dumps(dict(status='OK_EXPERIMENTAL',output=str(destination),sha256=sha(payload),
                              gameplay_validated=False)))
        return 0
    except (OSError,ValueError,KeyError,TypeError,zlib.error) as e:
        print('ERROR: '+str(e),file=sys.stderr);return 2
if __name__=='__main__':raise SystemExit(main())
