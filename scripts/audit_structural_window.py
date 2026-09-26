#!/usr/bin/env python3
"""Read-only evidence for January render-worker completion and structural endpoint."""
from __future__ import annotations
import argparse,hashlib,json,os,struct
from pathlib import Path
SHA='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69';SIZE=60748800
class PE:
 def __init__(self,d):
  if len(d)!=SIZE or hashlib.sha256(d).hexdigest()!=SHA:raise ValueError('requires exact January image')
  self.d=d;pe=struct.unpack_from('<I',d,0x3c)[0];n=struct.unpack_from('<H',d,pe+6)[0];opt=pe+24
  self.base=struct.unpack_from('<I',d,opt+28)[0];osz=struct.unpack_from('<H',d,pe+20)[0];self.s=[]
  for i in range(n):
   o=opt+osz+i*40;_,rva,rs,raw=struct.unpack_from('<IIII',d,o+8);self.s.append((self.base+rva,rs,raw))
 def read(self,a,n):
  for va,s,r in self.s:
   if r and va<=a and a+n<=va+s:return self.d[r+a-va:r+a-va+n]
  raise ValueError(hex(a))
def audit(d):
 p=PE(d);rows=[]
 def ex(a,h,l):
  b=bytes.fromhex(h)
  if p.read(a,len(b))!=b:raise ValueError(l)
  rows.append({'va':hex(a),'bytes':h,'label':l})
 for a,h,l in [
  (0x033BFEC3,'8bf48b45fc8b88d400000051ff1564b27d053bf4e8e02d8cfe8bf48b55fc8b82d000000050ff155cb27d053bf4','begin resets completion(+D4) then signals worker(+D0)'),
  (0x033C66CC,'8bf46affe83d358afe8b80d000000050ff1568b27d053bf4e8d3c58bfe85c07402ebdd','render thread waits indefinitely on +D0'),
  (0x033C66EF,'e81e358afe8bf48b88d000000051ff1564b27d05','render thread resets +D0 after wake'),
  (0x033C672D,'e8e0348afe8bf48b80d400000050ff155cb27d05','render thread signals +D4 on one completion path'),
  (0x033C6788,'e885348afe8bf48b88d400000051ff155cb27d05','render thread signals +D4 on normal completion path'),
  (0x033C0052,'8bf46a008b45fc8b88d400000051ff1568b27d05','sRender end polls +D4'),
  (0x033C008C,'8bf46a008b45fc8b88d400000051ff1568b27d05','sRender end final zero-timeout wait on +D4'),
  (0x033C00CD,'e8e2d27efe8b4dfce8787480fe8945d48bf46a006a006a006a008b4dd4518b55d48b028b4844ffd1','device present path occurs after completion wait'),
  (0x01BEBCC7,'e9f4427d01','thunk to sRender::end'),
  (0x02F50E75,'8b45f88b8870a40200e844aec9fe','outer cycle calls sRender::end'),
  (0x02F50F2A,'8b8898a402008b55f88b8298a402008b118bf48bc88b4218ffd03bf4e8711dd3fe5f5e5b81','post-render callbacks finish before endpoint'),
  (0x02F50F4B,'5f5e5b81c4d0000000','pipeline END endpoint after render end and post callbacks')]:
   ex(a,h,l)
 return {'status':'PASS_READ_ONLY_EVIDENCE','sha256':SHA,'checks':len(rows),'records':rows,
  'finding':'Pipeline END is reached after sRender::end has waited for the render-worker completion event and after later outer-cycle callbacks.',
  'cpu_render_worker_drained':True,'gpu_fence_proven':False,'gameplay_executed':False,'game_image_modified':False}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('original',type=Path);ap.add_argument('--report',type=Path);a=ap.parse_args()
 try:
  r=audit(a.original.read_bytes())
  if a.report:
   if os.path.lexists(a.report):raise ValueError('report exists')
   a.report.write_text(json.dumps(r,indent=2)+'\n')
  print(json.dumps({k:v for k,v in r.items() if k!='records'}))
 except (OSError,ValueError,struct.error) as e:raise SystemExit('ERROR: '+str(e))
