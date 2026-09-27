#!/usr/bin/env python3
"""Hash-pinned January priority-loop evidence; read-only, no game execution."""
import argparse
import json
import struct
from pathlib import Path
from audit_hud_lifetime import Image

SITES=(
    (0x01EA61A7,'8d85e4feffff50','begin after native priority/flags initialization'),
    (0x01EA6235,'7e05e958040000','signed priority decision and stock break'),
    (0x01EA6679,'6a008d8df4feffff','end candidate before stop-bit test'),
    (0x01EA66B6,'8b45f88b8d00ffffff','publish member-zero priority'),
)

def audit(data):
    im=Image(data);records=[]
    def exact(va,h,label):
        expected=bytes.fromhex(h)
        if im.read(va,len(expected))!=expected:raise ValueError(label)
        records.append({'va':hex(va),'label':label})
    def call(va,target,label):
        exact(va,(b'\xe8'+struct.pack('<i',target-va-5)).hex(),label)
    for site in SITES:exact(*site)
    for site in (
        (0x01EA6192,'c78500ffffff00000000','priority local starts at zero'),
        (0x01EA619C,'8d8df0feffff','flags wrapper at stack-110'),
        (0x01D11143,'8b4df883c104','flags storage at wrapper+4'),
        (0x01CDC526,'c70000000000','flags storage starts at zero'),
        (0x01EA61FB,'8985d8feffff','ranked prior local at stack-128'),
        (0x01EA622F,'3b85ccfeffff','compare prior rank with command rank'),
        (0x01EA63BC,'898500ffffff','type2 selection stores raw priority'),
        (0x01EA64DC,'898500ffffff','type3 selection stores raw priority'),
        (0x01EA65A9,'898500ffffff','type4 selection stores raw priority'),
        (0x01EA6628,'898500ffffff','type4 retained state stores priority'),
        (0x01EA6673,'898500ffffff','active command stores priority'),
        (0x01EA66BF,'898870010000','native manager priority publication +170'),
        (0x01EB75D6,'8b8070010000','draw priority getter reads +170'),
        (0x01CB3F03,'b8010000008b4d08d3e08b4df82301','flag getter uses one bit'),
        (0x01CFE453,'b8010000008b4d08d3e08b4df80b01','flag setter ORs one bit'),
        (0x01EAFFB9,'8b5128895024','history clear moves ring begin to end'),
        (0x01EB000B,'83c00183f809','history ring insertion reserves nine slots'),
        (0x01EA7C42,'8b003b4508','history membership compares command key'),
    ):exact(*site)
    for site in (
        (0x01EA61A2,0x01C576FC,'construct local flag wrapper'),
        (0x01EA6215,0x01BEEF99,'prepare hook replaces command-priority getter call'),
        (0x01EB7419,0x01C6FF63,'draw hook replaces this priority getter call only'),
        (0x01EA63CA,0x01C68E84,'first type2 selection checks shared history bit'),
        (0x01EA63DF,0x01C0AD0C,'type2 clears history once'),
        (0x01EA640C,0x01BECC2B,'type2 appends command key'),
        (0x01EA6419,0x01BA7711,'type2 sets shared history bit'),
        (0x01EA6448,0x01BA7711,'type2 sets exclusive stop bit'),
        (0x01EA6681,0x01C68E84,'loop reads stop bit'),
        (0x01EA669C,0x01C68E84,'loop finish reads history bit'),
    ):call(*site)
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':len(records),'records':records,
            'gameplay_executed':False,'history_capacity_increased':False}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path);a=p.parse_args()
    print(json.dumps(audit(a.image.read_bytes()),indent=2))
