from pathlib import Path
from PIL import Image
import xml.etree.ElementTree as E,hashlib
r=Path('preview14-work');s=r/'CityEvolution-Preview-14-Source';base=r/'base/CityEvolution-Preview-13-Source'
for direction in ['SE','SW','NW','NE']:
 ps=sorted((s/'data/images/transit').glob('woman-'+direction+'-*.png'));assert len(ps)==4,(direction,ps)
 for p in ps:
  im=Image.open(p);assert im.mode=='RGBA' and im.size==(16,24),p;assert im.getbbox(),p
 assert len({hashlib.sha256(p.read_bytes()).hexdigest() for p in ps})>=2,direction
for kind in ['food','goods','ore','coal']:
 areas=[]
 for level in [1,2,3]:
  p=s/f'data/images/activity/{kind}-{level}.png';im=Image.open(p);assert im.mode=='RGBA' and im.size==(50,50),p;areas.append(sum(a>0 for a in im.getchannel('A').tobytes()))
 assert areas[0]<areas[1]<areas[2],(kind,areas)
for p in (base/'data/images/tiles').rglob('*.xml'):
 q=s/p.relative_to(base)
 if q.exists():
  def counts(p):return [(x.tag,x.attrib.get('name'),len(list(x))) for x in E.parse(p).getroot()]
  assert counts(p)==counts(q),'Original sprite/resource frame counts changed: '+str(p)
print('PASS 16 RGBA woman walking frames, distinct poses per direction, 12 stock props, increasing visible inventory and original animation resource frame counts')
