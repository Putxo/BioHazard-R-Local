#!/usr/bin/env python3
"""Exact-January, read-only PauseHD input and cockpit commit evidence."""
import argparse
import json
import struct
from pathlib import Path
from audit_local_menu_owner import PE, SHA

CHECKS = (
    (0x02C31836, '6a008b4de0e8179cf6fe8945d46a008b4de0e8cb86fafe', 'Pause list reads +1A0 and +1AC'),
    (0x02C31FD7, '6a008b4dece87694f6fe8945e06a008b4dece82a7ffafe', 'Pause confirmation reads +1A0 and +1AC'),
    (0x01BD9F18, 'e973920d00', 'repeat wrapper thunk'),
    (0x01CB31B3, '8b45f88b8870090000518b4df8e8bb5ef3ff', 'repeat wrapper selects mStartPadNo'),
    (0x01BE9080, 'e96ba10c00', 'direct repeat getter thunk'),
    (0x01CB3213, '8b450869c0f80200008b4df88b8401ac010000', 'repeat getter indexes Pad[index]+1AC'),
    (0x01CB322C, 'c20400', 'direct getter callee cleanup'),
    (0x01CB31D8, 'c20400', 'wrapper callee cleanup'),
    (0x02C31A3C, '83c8082345d4', 'cancel/start consumes +1A0'),
    (0x02C31B15, '2345d4', 'confirm consumes +1A0'),
    (0x02C31B6D, '8b45c82510000100', 'up consumes +1AC'),
    (0x02C31BC8, '8b45c82540000400', 'down consumes +1AC'),
    (0x02CDD415, '8b45f88b4d08894824', 'common cockpit state commit: this+24=state'),
    (0x02CDD37C, '8b45f883782408740b', 'state6 tests previous state8'),
    (0x02CDD3E1, '8b45f883782406740b', 'state8 tests previous state6'),
)
PAUSE_CALLS = {
    0x02C3183B: ('rev_menu_gate_pause_1a0', 0x01B9B457),
    0x02C31848: ('rev_menu_gate_pause_1ac', 0x01BD9F18),
    0x02C31FDC: ('rev_menu_gate_pause_1a0', 0x01B9B457),
    0x02C31FE9: ('rev_menu_gate_pause_1ac', 0x01BD9F18),
}

def check_evidence(pe):
    rows = []
    for address, expected, label in CHECKS:
        value = bytes.fromhex(expected)
        if pe.read(address, len(value)) != value:
            raise ValueError(f'evidence mismatch at {address:#x}: {label}')
        rows.append({'va': f'0x{address:08X}', 'bytes': expected, 'label': label})
    for address, (bridge, target) in PAUSE_CALLS.items():
        if pe.read(address, 5) != b'\xe8' + struct.pack('<i', target-address-5):
            raise ValueError(f'call mismatch at {address:#x}')
    return rows

def audit(data):
    rows = check_evidence(PE(data))
    return {'status': 'PASS_READ_ONLY_EVIDENCE', 'sha256': SHA,
            'checks': len(rows) + len(PAUSE_CALLS), 'records': rows,
            'gameplay_executed': False, 'game_image_modified': False,
            'limits': ['Options widgets and nested state6/7 input remain unaudited.',
                       'Common commit gateway is source-only; runtime installation is pending.']}

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    try:
        result = audit(args.original.read_bytes())
        if args.report:
            with args.report.open('x', encoding='utf-8') as out:
                json.dump(result, out, indent=2)
                out.write('\n')
        print(json.dumps({k: v for k, v in result.items() if k != 'records'}))
    except (OSError, ValueError, struct.error) as error:
        raise SystemExit(f'ERROR: {error}')
