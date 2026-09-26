#!/usr/bin/env python3
"""Read-only proof for the three January HUD allocation thunks."""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
SHA='9124bb92d6c54a047ade47dacc8429221b18f9910b504f0e87718d5151013f69';SIZE=60748800
class PE:
 def __init__(self,d):
  if len(d)!=SIZE or hashlib.sha256(d).hexdigest()!=SHA:raise ValueError('wrong January image')
  self.d=d;pe=struct.unpack_from('<I',d,0x3c)[0];opt=pe+24
  self.base=struct.unpack_from('<I',d,opt+28)[0];n=struct.unpack_from('<H',d,pe+6)[0];os=struct.unpack_from('<H',d,pe+20)[0];self.s=[]
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
 for a,h,l in [
  (0x01C11706,'e9c5e6f200','Reticle allocator thunk'),
  (0x01C15775,'e99655f200','MainEquip allocator thunk'),
  (0x01C7855A,'e9c18fee00','MapHerb allocator thunk'),
  (0x02B3FDEE,'e8164d08ff89853cffffffb970315905e826be08ff8bf4508b450c508b4d08518b953cffffff8b028b8d3cffffff8b501cffd23bf4e8','Reticle native allocation pattern'),
  (0x02B3AD2E,'e854dc0aff89853cffffffb9d4305905e8e60e09ff8bf4508b450c508b4d08518b953cffffff8b028b8d3cffffff8b501cffd23bf4e8','MainEquip native allocation pattern'),
  (0x02B6153E,'e8b46e0cff89853cffffffb98c5d5905e8d6a606ff8bf4508b450c508b4d08518b953cffffff8b028b8d3cffffff8b501cffd23bf4e8','MapHerb native allocation pattern')]:ex(a,h,l)
 return {'status':'PASS_READ_ONLY_EVIDENCE','sha256':SHA,'checks':len(rows),'records':rows,
  'finding':'Each audited class allocator forwards size/alignment/type data to a native allocator virtual and directly returns its allocation result.',
  'game_image_modified':False,'gameplay_executed':False}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('original',type=Path);ap.add_argument('--report',type=Path);a=ap.parse_args()
 try:
  r=audit(a.original.read_bytes())
  if a.report:
   if a.report.exists():raise ValueError('report exists')
   a.report.write_text(json.dumps(r,indent=2)+'\n')
  print(json.dumps({k:v for k,v in r.items() if k!='records'}))
 except (OSError,ValueError,struct.error) as e:raise SystemExit('ERROR: '+str(e))
