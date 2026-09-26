#!/usr/bin/env python3
"""Read-only proof of Viewport.mRegion -> cDraw active rect in January build."""
from __future__ import annotations
import argparse,hashlib,json,struct,os
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
  (0x0328A762,'89153c9d7905','sCamera constructor publishes singleton'),
  (0x0328A8FA,'c700a89aec04','sCamera vtable'),
  (0x03700689,'c7001490f704','cDraw vtable'),
  (0x0328AA26,'8b55ec69d2900100008b45fc8d4c1030','normal loop computes Viewport[i]'),
  (0x0328AA8A,'8b45ec508b4d08e82c198ffe','normal loop passes i to cDraw view-index setter'),
  (0x0328CE8E,'8b450825ff0000008b4dfc8b915801000081e200ffffff0bd08b45fc899058010000','cDraw low-byte view index setter'),
  (0x032891FB,'8b4dfce87e079bfe','Viewport render recalculates mRegion'),
  (0x03289203,'8d4de8518b4dfce833d197fe','Viewport render copies region to local'),
  (0x02494B33,'8b45f883c018508b4d08e86b9c6eff','region getter copies Viewport+18'),
  (0x03289469,'8d4de8518d9550ffffff528d45a0508b4b08e85fcc98fe','same region local passed into cDraw setup'),
  (0x0370979E,'8b4b10518b4dfce8264458fe','cDraw setup forwards third arg to active-rect setter'),
  (0x0370844C,'8b4b08518b4dfc81c1bc000000e818e349fe','active rect copied into cDraw+BC'),
  (0x0370845E,'8b5308528b4dfc81c1cc000000e806e349fe','same rect copied into half-resolution cDraw+CC'),
  (0x0328A720,'833d3c9d790500','sCamera singleton checked before publish'),
  (0x0328A943,'c7053c9d790500000000','sCamera destructor clears singleton')]:
   ex(a,h,l)
 return {'status':'PASS_READ_ONLY_EVIDENCE','sha256':SHA,'checks':len(rows),'records':rows,
  'finding':'Normal per-Viewport render recalculates Viewport.mRegion, passes it to cDraw, and installs it at cDraw+0xBC before GUI draw.',
  'gameplay_executed':False,'game_image_modified':False,'manual_half_scale_required':False}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('original',type=Path);ap.add_argument('--report',type=Path);a=ap.parse_args()
 try:
  r=audit(a.original.read_bytes())
  if a.report:
   if os.path.lexists(a.report):raise ValueError('report exists')
   a.report.write_text(json.dumps(r,indent=2)+'\n')
  print(json.dumps({k:v for k,v in r.items() if k!='records'}))
 except (OSError,ValueError,struct.error) as e:raise SystemExit('ERROR: '+str(e))
