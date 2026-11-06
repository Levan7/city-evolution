from pathlib import Path
from PIL import Image
import hashlib,xml.etree.ElementTree as E
r=Path('preview15-work');s=r/'CityEvolution-Preview-15-Source';b=r/'base/CityEvolution-Preview-14-Source';count=0
for d in ['data/images','data/night']:
 for p in (b/d).rglob('*'):
  if p.is_file():
   q=s/p.relative_to(b);assert q.is_file() and hashlib.sha256(p.read_bytes()).digest()==hashlib.sha256(q.read_bytes()).digest(),q;count+=1
for direction in ['SE','SW','NW','NE']:
 ps=sorted((s/'data/images/transit').glob('woman-'+direction+'-*.png'));assert len(ps)==4
 for p in ps:
  im=Image.open(p);assert im.mode=='RGBA' and im.size==(16,24) and im.getbbox()
 assert len({hashlib.sha256(p.read_bytes()).hexdigest() for p in ps})>=2
for kind in ['food','goods','ore','coal']:
 areas=[]
 for level in [1,2,3]:
  im=Image.open(s/f'data/images/activity/{kind}-{level}.png');assert im.mode=='RGBA' and im.size==(50,50);areas.append(sum(a>0 for a in im.getchannel('A').tobytes()))
 assert areas[0]<areas[1]<areas[2]
print('PASS',count,'existing image/night files preserved byte-for-byte; all woman poses, activity props and animation resources retained')
