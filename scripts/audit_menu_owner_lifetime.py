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
 ex(0x01C365BF,'e9bc6b0a01','central transition thunk -> 0x02CDD180')
 ex(0x02CDD1C1,'837d08057421837d0806741b837d08077415837d0808740f','engine groups states 5/6/7/8 as paused family')
 ex(0x02CDD415,'8b45f88b4d08894824','central transition commits new state to sIDCockpit+0x24')
 ex(0x02B4FD73,'8b4df88981b80200006a016a066a00e83e3509ff','uGUI_InGameFile stores previous state and enters state6')
 ex(0x02B51AD2,'6a018b45f88b88b8020000516a00e8e01709ff','uGUI_InGameFile restores saved previous state')
 ex(0x01F3F009,'6a016a016a00e8b142caff83c4048bc8e8a2e7d4ff','episode closes state5 through state1')
 ex(0x01F3F055,'6a016a016a00e86542caff83c4048bc8e856e7d4ff','episode closes state8 through state1')
 ex(0x02CDCB8C,'6a006a008b4df8e8279af5fe','sIDCockpit constructor initializes transition state0')
 return {'status':'PASS_READ_ONLY_EVIDENCE','sha256':SHA,'checks':len(rows),'records':rows,
  'gameplay_executed':False,'game_image_modified':False,
  'finding':'0x01C365BF is the central transition thunk; states5-8 are a paused family, state6 saves/restores the previous state, and episode close returns state5/8 to state1.',
  'policy':'observe the central transition: keep owner across nested6/7 and matching5/8; clear otherwise; bind J2 to exact Sub0 pointer+serial'}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('original',type=Path);ap.add_argument('--report',type=Path);a=ap.parse_args()
 try:
  r=audit(a.original.read_bytes())
  if a.report:
   if a.report.exists():raise ValueError('report exists')
   a.report.write_text(json.dumps(r,indent=2)+'\n')
  print(json.dumps({k:v for k,v in r.items() if k!='records'}))
 except (OSError,ValueError,struct.error) as e:raise SystemExit('ERROR: '+str(e))
