#!/usr/bin/env python3
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
SHA='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69'; SIZE=60_748_800

class PE:
 def __init__(self,d):
  if len(d)!=SIZE or hashlib.sha256(d).hexdigest()!=SHA: raise ValueError('requires exact January image')
  self.d=d;pe=struct.unpack_from('<I',d,0x3c)[0];opt=pe+24;self.base=struct.unpack_from('<I',d,opt+28)[0]
  n=struct.unpack_from('<H',d,pe+6)[0];os=struct.unpack_from('<H',d,pe+20)[0];self.s=[]
  for i in range(n):
   o=opt+os+i*40;vs,rva,rs,rp=struct.unpack_from('<IIII',d,o+8);self.s.append((self.base+rva,max(vs,rs),rp))
 def read(self,a,n):
  for va,s,r in self.s:
   if va<=a and a+n<=va+s:return self.d[r+a-va:r+a-va+n]
  raise ValueError(hex(a))

def audit(d):
 p=PE(d);rows=[]
 def ex(a,h,l):
  b=bytes.fromhex(h)
  if p.read(a,len(b))!=b:raise ValueError(l+' '+hex(a))
  rows.append({'va':hex(a),'size':len(b),'label':l})
 ex(0x02C3182E,'e8c19f05ff8945e06a008b4de0e8179cf6fe8945d46a008b4de0e8cb86fafe8945c8','pause selection path A reads +1A0 then +1AC')
 ex(0x02C31FCF,'e8209805ff8945ec6a008b4dece87694f6fe8945e06a008b4dece82a7ffafe8945d4','pause selection path B reads +1A0 then +1AC')
 ex(0x01CB3103,'8b45f88b8870090000518b4df8e8e854f9ff','wrapper +1A0 selects mStartPadNo')
 ex(0x01CB3163,'8b450869c0f80200008b4df88b8401a0010000','direct Pad[index]+1A0')
 ex(0x01CB31B3,'8b45f88b8870090000518b4df8e8bb5ef3ff','wrapper +1AC selects mStartPadNo')
 ex(0x01CB3213,'8b450869c0f80200008b4df88b8401ac010000','direct Pad[index]+1AC')
 ex(0x02C31A34,'8b4de0e8e05200ff83c8082345d4','path A +1A0 gates accept/cancel mask')
 ex(0x02C31B6D,'8b45c825100001007451','path A +1AC mask 0x10010')
 ex(0x02C31BC8,'8b45c825400004007445','path A +1AC mask 0x40040')
 ex(0x02C32221,'8b4dece8f34a00ff83c8082345e0','path B +1A0 gates decision mask')
 ex(0x02C32332,'8b4dece8073df7fe2345e0','path B +1A0 secondary decision mask')
 ex(0x02C323CD,'8b45d42550000500743c','path B +1AC mask 0x50050')
 return {
  'status':'PASS_READ_ONLY_EVIDENCE','sha256':SHA,'checks':len(rows),'records':rows,
  'gameplay_executed':False,'game_image_modified':False,
  'finding':'PauseHD has two live selection paths; each reads sPad via mStartPadNo at +1A0/+1AC and consumes those values for decision and selection movement.',
  'policy':'when MenuOwnerRouter surface=Pause and owner=J2, route only these four wrapper callsites to Pad1; preserve stock otherwise'}

if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('original',type=Path);ap.add_argument('--report',type=Path);a=ap.parse_args()
 try:
  r=audit(a.original.read_bytes())
  if a.report:
   if a.report.exists():raise ValueError('report exists')
   a.report.write_text(json.dumps(r,indent=2)+'\n')
  print(json.dumps({k:v for k,v in r.items() if k!='records'}))
 except (OSError,ValueError,struct.error) as e:raise SystemExit('ERROR: '+str(e))
