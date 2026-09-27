#!/usr/bin/env python3
"""Read-only audit for January local menu owner routing; never patches the game."""
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
   o=opt+os+i*40;_,rva,rs,rp=struct.unpack_from('<IIII',d,o+8);self.s.append((self.base+rva,rs,rp))
 def read(self,a,n):
  for va,s,r in self.s:
   if va<=a and a+n<=va+s:return self.d[r+a-va:r+a-va+n]
  raise ValueError(hex(a))
def audit(d):
 p=PE(d);rows=[]
 def ex(a,h,l):
  b=bytes.fromhex(h)
  if p.read(a,len(b))!=b:raise ValueError(l)
  rows.append({'va':hex(a),'size':len(b),'label':l})
 # cEpisodeMainState: state2 -> pause bit8 / submenu bit1, both via mStartPadNo wrapper.
 ex(0x01F3EF35,'83f802757c','cockpit state must be 2 before normal open input')
 ex(0x01F3EF3A,'6a00e8b3c8d4ff8bc8e80fc5c5ff83e008','pause open via sPad wrapper and bit8')
 ex(0x01F3EF72,'6a016a056a00','pause enters cockpit state5')
 ex(0x01F3EF89,'6a00e864c8d4ff8bc8e8c0c4c5ff83e001','submenu open via same sPad wrapper and bit1')
 ex(0x01F3EF9C,'6a016a086a00','submenu enters cockpit state8')
 # Exact stock wrapper and direct per-pad fields.
 ex(0x01CB3103,'8b45f88b887009000051','stock sPad wrapper reads mStartPadNo')
 ex(0x01CB3163,'8b450869c0f80200008b4df88b8401a0010000','direct Pad[index]+1A0 getter')
 ex(0x01CB3003,'8b450869c0f80200008b4df88b840198010000','direct Pad[index]+198 getter')
 # uGUI_SubMenu slot8: two genuinely indexed values.
 ex(0x02B6A484,'e86b1312ff8945e06a008b4de0e88a1c0fff','submenu gets sPad then state198 wrapper')
 ex(0x02B6A499,'6a008b4de0e8b40f03ff8945c8','submenu state1A0 wrapper')
 # The four nearby wrappers read mStartPadNo but their direct implementations ignore index.
 for a,h,l in [
  (0x02B6BDB3,'8b45f88b887009000051','aux wrapper 650 passes mStartPadNo'),
  (0x02B6BE13,'8b45f88b8050060000','aux direct 650 ignores index'),
  (0x02B6BE63,'8b45f88b887009000051','aux wrapper 654 passes mStartPadNo'),
  (0x02B6BEC3,'8b45f88b8054060000','aux direct 654 ignores index'),
  (0x02B50BE3,'8b45f88b887009000051','aux wrapper 658 passes mStartPadNo'),
  (0x02B50C43,'8b45f88b8058060000','aux direct 658 ignores index'),
  (0x02B50C93,'8b45f88b887009000051','aux wrapper 65C passes mStartPadNo'),
  (0x02B50CF3,'8b45f88b805c060000','aux direct 65C ignores index')]: ex(a,h,l)
 # Two hard-coded Self -> own cBioItemPack paths in submenu data rebuild.
 ex(0x02B6DE8F,'8bc8e85ee90bff8945ec','submenu primary Self finder')
 ex(0x02B6DEA4,'8b4dece87be405ff8945e0','submenu primary actor pack')
 ex(0x02B6EFDC,'8bc8e811d80bff8945ec','submenu secondary Self finder')
 ex(0x02B6EFF1,'8b4dece82ed305ff8945e0','submenu secondary actor pack')
 return {'status':'PASS_READ_ONLY_EVIDENCE','sha256':SHA,'checks':len(rows),'records':rows,
  'gameplay_executed':False,'game_image_modified':False,
  'finding':'Episode gameplay opens pause/submenu from mStartPadNo; uGUI_SubMenu indexes two pad bitfields and rebuilds both data paths from Self cBioItemPack.',
  'policy':'route only the audited opener/input/Self callsites; never replace global Self or persistently change mStartPadNo'}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('original',type=Path);ap.add_argument('--report',type=Path);a=ap.parse_args()
 try:
  r=audit(a.original.read_bytes())
  if a.report:
   if a.report.exists():raise ValueError('report exists')
   a.report.write_text(json.dumps(r,indent=2)+'\n')
  print(json.dumps({k:v for k,v in r.items() if k!='records'}))
 except (OSError,ValueError,struct.error) as e:raise SystemExit('ERROR: '+str(e))
