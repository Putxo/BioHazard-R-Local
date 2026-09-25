#!/usr/bin/env python3
"""Hash-pinned read-only evidence for scoped P1 mDrawView masking."""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
SHA='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69';SIZE=60748800
class PE:
 def __init__(self,d):
  if len(d)!=SIZE or hashlib.sha256(d).hexdigest()!=SHA:raise ValueError('wrong January image')
  self.d=d;pe=struct.unpack_from('<I',d,0x3c)[0];opt=pe+24
  self.base=struct.unpack_from('<I',d,opt+28)[0];n=struct.unpack_from('<H',d,pe+6)[0];os=struct.unpack_from('<H',d,pe+20)[0]
  self.s=[]
  for i in range(n):
   o=opt+os+i*40;_,rva,rs,rp=struct.unpack_from('<IIII',d,o+8);self.s.append((self.base+rva,rs,rp))
 def read(self,a,n):
  for va,s,r in self.s:
   if va<=a and a+n<=va+s:return self.d[r+a-va:r+a-va+n]
  raise ValueError(hex(a))
 def u32(self,a):return struct.unpack('<I',self.read(a,4))[0]
def audit(d):
 p=PE(d);rows=[]
 def ex(a,h,l):
  b=bytes.fromhex(h)
  if p.read(a,len(b))!=b:raise ValueError(l)
  rows.append({'va':hex(a),'size':len(b),'bytes':h,'label':l})
 for a,h,l in [
  (0x02B48916,'89485c','MainEquipWin stored at cockpit+5C'),
  (0x02B48F53,'898890000000','Reticle stored at cockpit+90'),
  (0x02B681C7,'894840','MapHerb alias at minimap+40'),
  (0x02B681D6,'890c90','MapHerb first slot at minimap+30'),
  (0x02B49DE3,'8b45f88b483c','cockpit pre-loop gateway window'),
  (0x02B68724,'8b45f883c030','minimap pre-loop gateway window'),
  (0x02B49E5C,'8b4df8e846f310ff','cockpit loop exit window'),
  (0x02B687F0,'5f5e5b81c40c010000','minimap common epilogue window'),
  (0x02B49E21,'e8fa8e09ff2345ec7429','cockpit mDrawView filter'),
  (0x02B687BB,'e860a507ff2345e07426','minimap mDrawView filter'),
  (0x0279D1C6,'8b400cc1e81025ff030000','mDrawView getter'),
  (0x01EB6153,'8b450825ff030000c1e010','mDrawView setter mask'),
  (0x01EB6164,'81e2ffff00fc0bd0','mDrawView setter preserves other bits')]:ex(a,h,l)
 for a,v,l in [(0x04DE4C3C,0x01C0C74C,'MainEquip vtable'),(0x04DE52C4,0x01C40C04,'Reticle vtable'),(0x04DE810C,0x01BAB4F6,'MapHerb vtable')]:
  if p.u32(a)!=v:raise ValueError(l)
  rows.append({'va':hex(a),'size':4,'value':hex(v),'label':l})
 return {'status':'PASS_READ_ONLY_EVIDENCE','sha256':SHA,'checks':len(rows),'records':rows,
  'gameplay_executed':False,'game_image_modified':False,
  'finding':'Only the three P1 originals with P2 clones need bit1 suppressed during the stock view1 loops.'}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('original',type=Path);ap.add_argument('--report',type=Path);a=ap.parse_args()
 try:
  r=audit(a.original.read_bytes())
  if a.report:
   if a.report.exists():raise ValueError('report exists')
   a.report.write_text(json.dumps(r,indent=2)+'\n')
  print(json.dumps({k:v for k,v in r.items() if k!='records'}))
 except (OSError,ValueError,struct.error) as e:
  raise SystemExit('ERROR: '+str(e))
