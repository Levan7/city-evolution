"""Protect audited menus outside the explicit Preview 15 change list."""
from pathlib import Path
import hashlib,json,xml.etree.ElementTree as E
s=Path(__file__).resolve().parents[1]
manifest=json.loads((s/'doc/audit/Preview12-protected-menu-hashes.json').read_text())
authorized = {'data/gui/mps.xml','data/gui/minimap.xml','data/gui/buttonpanel.xml','data/gui/pbar.xml',
 'data/gui/pbar2nd.xml','data/gui/app.xml','data/gui/mainmenu.xml'}
for f,sha in manifest.items():
 if f in authorized: continue
 data=(s/f).read_bytes()
 if f=='data/gui/mainmenu.xml':data=data.replace(b'Preview 14',b'Preview 12')
 assert hashlib.sha256(data).hexdigest()==sha,f+' changed'
expected=json.loads((s/'doc/audit/Preview12-cityhall-pages.json').read_text())
pages=E.parse(s/'data/gui/cityhall.xml').findall('.//SwitchComponent/Panel')
def canonical(p):
 for x in p.iter():
  if x.text is not None: x.text=x.text.strip() or None
  if x.tail is not None: x.tail=x.tail.strip() or None
 return E.tostring(p,encoding='unicode')
assert {p.get('name'):canonical(p) for p in pages if p.get('name')!='MayorPage'}=={n:canonical(E.fromstring(v)) for n,v in expected.items() if n!='MayorPage'},'Audited non-Mayor City Hall page changed' 
cmake=(s/'data/gui/CMakeLists.txt').read_text()
for p in (s/'data/gui').glob('*.xml'):assert p.name in cmake,p.name+' missing build registration'
for p in (s/'data/gui').rglob('*.xml'):
 for panel in E.parse(p).findall('.//Panel'):
  if not (float(panel.get('width','0'))>0 and float(panel.get('height','0'))>0):assert panel.get('resizable')=='yes',(p,panel.attrib)
print('PASS audited menus, menu images, City Hall pages, GUI build registration and panel sizing')
