from pathlib import Path
from lxml import etree as E
from PIL import Image
import json
S=Path(__file__).resolve().parents[1];D=S/'data';R={}
for g in E.parse(str(D/'images/tiles/images.xml')).findall('group'):
 key=None
 for n in g:
  if n.tag=='resourceID':key=n.get('name');assert key not in R;R[key]=[]
  elif n.tag=='image':R[key].append(dict(n.attrib))
old=json.loads((S/'art/preview10/native-frame-layout.json').read_text())
for k,v in old.items():assert len(R[k])==len(v),(k,'changed frame count')
for k,frames in R.items():
 for a in frames:assert (D/'images/tiles'/a['file']).exists(),(k,a['file'])
for k in ['Commune','CommuneVariant1','CommuneVariant2']:assert len(R[k])==len(old['Commune'])
for k in ['ResidentialLowLow','ResidentialMedLow','ResidentialHighLow','ResidentialLow','ResidentialMedHigh','ResidentialHighHigh']:
 assert len(R[k+'Variant1'])==len(R[k])
 assert R[k][0]['file']!=R[k+'Variant1'][0]['file']
 assert Image.open(D/'images/tiles'/R[k][0]['file']).size[0]==384
bp=E.parse(str(D/'gui/buttonpanel.xml'));tools={n.get('name'):dict(n.attrib) for n in bp.findall('menu/tool')}
before=E.parse(str(S/'art/preview10/before10-buttonpanel.xml'))
# Preview 15 assigns illustrated per-object Help pages; preserve every
# existing construction identity, group and tool mapping.
for n in before.findall('menu/tool'):
 assert {k:v for k,v in tools[n.get('name')].items() if k!='help'}=={k:v for k,v in n.attrib.items() if k!='help'}

assert len(tools)==59 and len(bp.findall('menu'))==15
assert not bp.xpath('.//Button[@name="TransportButton"]')
app=E.parse(str(D/'gui/app.xml'));assert len(app.xpath('.//Button[@name="TransportButton"]'))==1
assert app.xpath('.//Button[@name="MayorButton"]/../..')[0] is app.xpath('.//Button[@name="TransportButton"]/../..')[0]
assert not app.xpath('.//Button[@name="ShowBuildingsButton"]')
tr=E.parse(str(D/'gui/transport.xml'));assert not tr.xpath('.//Button[starts-with(@name,"Build")]')
sp=E.parse(str(D/'gui/speedpanel.xml'));assert {n.getparent().get('row') for n in sp.findall('.//CheckButton')}=={'2'}
assert E.parse(str(D/'gui/mainmenu.xml')).find('SwitchComponent/Panel/Image').get('src')=='images/evolution/city-menu.png'
tables=0
for name in ['mainmenu','app','buttonpanel','speedpanel','transport','cityhall','publicservices']:
 t=E.parse(str(D/'gui'/f'{name}.xml'))
 for table in t.findall('.//TableLayout'):
  tables+=1;occupied=set();rows=int(table.get('rows'));cols=int(table.get('cols'))
  for n in table.findall('cell'):
   for r in range(int(n.get('row')),int(n.get('row'))+int(n.get('rowspan','1'))):
    for c in range(int(n.get('col')),int(n.get('col'))+int(n.get('colspan','1'))):
     assert 1<=r<=rows and 1<=c<=cols and (r,c) not in occupied,(name,r,c)
     occupied.add((r,c))
 for n in t.iter():
  if not isinstance(n.tag,str):continue
  for a in ['src','background']:
   p=n.get(a)
   if p and p.startswith(('images/','gui/','fonts/')):assert (D/p).exists(),(name,p)
metrics=json.loads((S/'art/preview11/scale-metrics.json').read_text());assert all(x['sprite_scale']==.8 for x in metrics)
result={'result':'PASS','tools':59,'previous_tools_preserved':51,'categories':15,'tables_checked':tables,'commune_variants':3,'housing_appearances':12,'replacement_sprite_scale':.8,'building_frames_rescaled':len(metrics),'native_frame_counts_preserved':True,'transport_report_build_controls':0,'windows_graphical_play_tested':False}
print(json.dumps(result,indent=2))
