from pathlib import Path
import xml.etree.ElementTree as E,hashlib,re
s=Path(__file__).resolve().parents[1];data=s/'data'
def named(r,n):return [x for x in r.iter() if x.get('name')==n]
app=E.parse(data/'gui/app.xml').getroot();assert not named(app,'ShowBuildingsButton');assert len(named(app,'MayorButton'))==len(named(app,'TransportButton'))==1
hud=E.parse(data/'gui/minimap.xml').getroot();assert hud.get('width')=='264';names=['ZoomOutButton','ZoomInButton','HelpButton','showTerrainHeight','hideHighBuildings','StatButton','mapOverlay','ScrollPageUp','ScrollPageDown'];assert all(len(named(hud,n))==1 for n in names)
nav=next(x for x in hud.iter('TableLayout') if x.get('cols')=='9');assert sum(int(x.get('fixed')) for x in nav.findall('colsize'))==264
for f in ['pbar.xml','pbar2nd.xml']:
 r=E.parse(data/'gui'/f).getroot();assert not any(x.get('background')=='images/gui/pbar_bg.png' for x in r.iter());assert any(x.get('color')=='#14252dff' for x in r.iter('FilledRectangle'))
cat=E.parse(data/'gui/buttonpanel.xml').getroot();tools=list(cat.iter('tool'));assert len(tools)==64;assert len({x.get('name') for x in tools})==64
for g in range(63,75):assert len([x for x in tools if x.get('grpid')==str(g)])==1
for tool in tools:assert len(named(cat,tool.get('name')))==2,(tool.attrib,'missing/multiple construction button')
for tool in tools:
 if not tool.get('grpid'):continue
 p=data/'help/en'/(tool.get('help')+'.xml');assert p.exists(),p
 r=E.parse(p).getroot();assert any(x.tag.lower() in ['image','img'] for x in r.iter()),('missing Help model',tool.attrib)
 for image in r.iter('img'):assert (data/image.get('src')).is_file(),image.attrib
church=named(cat,'BPMChurchButton')[-1];assert any((x.text or '').strip()=='Church' for x in church.iter('Paragraph'));assert 'Orthodox' not in E.tostring(church,encoding='unicode')
services=E.parse(data/'gui/publicservices.xml').getroot();assert len(named(services,'FullAutonomy'))==1;assert not named(services,'ZombieToggle');assert not named(services,'BuildPolice');assert not named(services,'DevelopmentPage')
city=E.parse(data/'gui/cityhall.xml').getroot();assert len(named(city,'MayorFullAutonomy'))==1
credits=(data/'gui/creditslist.xml').read_text();assert '>City Evolution by Levan</Paragraph>' in credits and 'studio76@mail.com' in credits
for p in (data/'gui').glob('*.xml'):assert re.search(r'<\?xml-stylesheet',p.read_text()),p
print('PASS compact HUD/navigation, dark statistics, 64 tools, twelve added catalogue buildings, every construction Help illustration, Church text, autonomy controls, hidden event and owner credits')
