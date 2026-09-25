#!/usr/bin/env python3
"""Reproduce the experimental January candidate directly from the original EXE.

Standard library only; Python 3.10+. Never runs or uploads the game. Never
replaces the original or an existing destination. --reverse writes a separate,
byte-exact original copy. This is not a claim of complete, playable local co-op.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import sys

SOURCE_SHA='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'
OUTPUT_SHA='71f5e70dc19c334d303ee29a686d65a72cd18b0c87b98472170bff961cb048f4'
PAYLOAD_SHA='deef32213898d607995c41b2986e5de4b864da4401c8763a6361fd68513d528d'

def sha(data: bytes | bytearray) -> str:
    return hashlib.sha256(data).hexdigest()

def load_payload(path: Path) -> dict:
    raw=path.read_bytes()
    if sha(raw)!=PAYLOAD_SHA:raise ValueError('payload is missing, changed or from another build')
    result=json.loads(raw)
    if (result['source_sha256'],result['output_sha256'])!=(SOURCE_SHA,OUTPUT_SHA):
        raise ValueError('unexpected payload identity')
    return result

def transform(source: bytes, payload: dict, reverse: bool=False) -> bytes:
    if payload.get('format')!='revlocal-bytepatch-v1':raise ValueError('unsupported payload format')
    input_key,output_key=('output','source') if reverse else ('source','output')
    if len(source)!=payload[input_key+'_size'] or sha(source)!=payload[input_key+'_sha256']:
        raise ValueError('wrong input SHA-256/size; the original January build is required')
    original_size=payload['source_size']
    if not isinstance(original_size,int) or original_size<0:raise ValueError('invalid source size')
    appended=bytes.fromhex(payload['append_hex'])
    if original_size+len(appended)!=payload['output_size']:raise ValueError('invalid appended extent')
    data=bytearray(source[:original_size])
    previous_end=0
    for record in payload['writes']:
        offset=record['offset'];old=bytes.fromhex(record['old']);new=bytes.fromhex(record['new'])
        if not isinstance(offset,int) or offset<previous_end or not old or len(old)!=len(new):
            raise ValueError('unordered, overlapping or length-changing write')
        end=offset+len(old)
        if end>original_size:raise ValueError('write exceeds original image')
        expected,replacement=(new,old) if reverse else (old,new)
        if data[offset:end]!=expected:raise ValueError(f'original-byte mismatch at {offset:#x}')
        data[offset:end]=replacement;previous_end=end
    if not reverse:data.extend(appended)
    result=bytes(data)
    if len(result)!=payload[output_key+'_size'] or sha(result)!=payload[output_key+'_sha256']:
        raise ValueError('result verification failed; no destination was written')
    return result

def main() -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input',type=Path)
    parser.add_argument('output',type=Path)
    parser.add_argument('--reverse',action='store_true')
    args=parser.parse_args()
    try:
        if args.input.resolve()==args.output.resolve():raise ValueError('input and output must differ')
        if args.output.exists() or args.output.is_symlink():raise ValueError('destination already exists')
        if not args.output.parent.is_dir():raise ValueError('destination directory does not exist')
        payload=load_payload(Path(__file__).with_name('payload.json'))
        result=transform(args.input.read_bytes(),payload,args.reverse)
        with args.output.open('xb') as f:f.write(result)
        print(json.dumps({'output':str(args.output),'sha256':sha(result),'size':len(result),
                          'restored_original':args.reverse,'gameplay_executed':False,
                          'status':'ORIGINAL_RESTORED' if args.reverse else 'EXPERIMENTAL_NOT_GAMEPLAY_VALIDATED'}))
        return 0
    except (OSError,ValueError,KeyError,TypeError) as error:
        print('ERROR: '+str(error),file=sys.stderr)
        return 2
if __name__=='__main__':raise SystemExit(main())
