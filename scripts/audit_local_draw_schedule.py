#!/usr/bin/env python3
"""Read-only January sUnit immediate/queued drawing evidence. Never executes PE."""
import argparse
import json
import struct
from pathlib import Path
from audit_hud_lifetime import Image

def audit(data):
    im=Image(data);records=[]
    def exact(va,h,label):
        b=bytes.fromhex(h)
        if im.read(va,len(b))!=b:raise ValueError(label)
        records.append({'va':hex(va),'label':label})
    def call(va,target,label):exact(va,(b'\xe8'+struct.pack('<i',target-va-5)).hex(),label)
    for site in (
        (0x03268560,'558bec83ec60','sUnit draw frame layout'),
        (0x0326861A,'8b4d08','render context is EBP+8'),
        (0x03268622,'ba010000008bc8d3e28955d4','context index selects view mask'),
        (0x03268633,'0fb6c085c0750a','stock AL selector takes immediate path'),
        (0x0326863A,'837ddc010f854c010000','one worker also takes immediate path'),
        (0x03268644,'6a016a006a008b4d0851','immediate setup uses context, null tasks, zero count, true'),
        (0x03268735,'8bf48b4d08518b55cc8b028b4dcc8b503cffd2','immediate alternate draw slot15'),
        (0x03268751,'8bf48b4508508b4dcc8b118b4dcc8b422cffd0','immediate normal draw slot11'),
        (0x0326877B,'6a006a008b550852','immediate cleanup uses context, null tasks, zero count'),
        (0x0326878B,'e92d020000','immediate joins shared cleanup after queued path'),
        (0x03268790,'8b45dc508b4dfc81c1400d000051','queued path configures per-worker task storage'),
        (0x0326895C,'6809e0c701','queued normal draw callback'),
        (0x0326D100,'8b01ff602c','queued callback tail-dispatches slot11'),
        (0x032689BD,'c745a800000000','shared cleanup entry'),
        (0x03268A51,'c20400','draw function consumes context argument'),
        (0x0326CCD0,'558beca09c727a055dc3','stock selector reads global byte only'),
    ):exact(*site)
    for site in (
        (0x0326862E,0x01BCD42A,'only replaced selector call'),
        (0x0326864E,0x01BA2E78,'native immediate setup'),
        (0x03268783,0x01C6F35B,'native immediate cleanup'),
        (0x032687B9,0x01BA2E78,'native queued setup'),
        (0x0326898A,0x01C25C5B,'native queued wait'),
        (0x032689A0,0x01C6F35B,'native queued cleanup'),
    ):call(*site)
    return {'status':'PASS_STATIC_EVIDENCE_ONLY','checks':len(records),'records':records,
            'gameplay_executed':False,'global_selector_modified':False,'performance_measured':False}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path);a=p.parse_args()
    print(json.dumps(audit(a.image.read_bytes()),indent=2))
