from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
from lxml import etree as E
import json,copy
s=Path(__file__).resolve().parents[2];a=s/'art';tiles=s/'data/images/tiles';vehicles=s/'data/images/transit';vehicles.mkdir(exist_ok=True)
sizes={'house':2,'warehouse':2,'tech-campus':3,'cafe':1,'school':2,'clinic':2,'factory':3,'farm':4,'bus-stop':1,'bus-terminal':2,'rail-station':3,'metro-entrance':1,'elevated-station':2,'food-hall':2,'fire-station':2,'university':3,'market':2,'solar-plant':4,'wind-turbine':2,'substation':2,'water-well':2,'coal-plant':4,'heavy-factory':4,'recycling-center':2}
for era in ['basic','modern']:
 for density in ['low','medium','high']:sizes[f'residence-{era}-{density}']=3
widths={'car':34,'car-NE':34,'car-NW':34,'car-SE':34,'truck':48,'horse':24,'horse-cart':52}
for d in ['NE','NW','SE','SW']:widths['bus-'+d]=54;widths['train-'+d]=82
rows={}
for p in sorted((a/'masters').glob('*.png')):
 n=p.stem;im=Image.open(p).convert('RGBA');box=im.getchannel('A').point(lambda x:255 if x>12 else 0).getbbox();assert box,(p,'empty');crop=im.crop(box)
 w=128*sizes[n] if n in sizes else widths[n];h=round(crop.height*(w-8)/crop.width);crop=crop.resize((w-8,h),Image.Resampling.LANCZOS);out=Image.new('RGBA',(w,h+8));out.alpha_composite(crop,(4,4));dest=tiles/('revamp-'+n+'.png') if n in sizes else vehicles/(('car-SW' if n=='car' else n)+'.png');out.save(dest)
 rows[n]={'file':str(dest.relative_to(s/'data')),'size':list(out.size),'anchor':[w//2,out.height-4] if n in sizes else [w//2,out.height-4],'footprint':sizes.get(n)}
# Bind replacement building art while retaining every original frame slot for save/game-code safety.
mapr={'Health':'clinic','School':'school','Farm':'farm','IndustryLight':'factory','IndustryHigh':'heavy-factory','PowerSolar':'solar-plant','WindMill':'wind-turbine','WindMillHTech':'wind-turbine','WindMillHTechRG':'wind-turbine','WindMillHTechG':'wind-turbine','FireStation':'fire-station','University':'university','Waterwell':'water-well','Substation':'substation','SubstationOn':'substation','SubstationOff':'substation','Recycle':'recycling-center','MarketEmpty':'market','MarketLow':'market','MarketMed':'market','MarketFull':'market','PowerCoalEmpty':'coal-plant','PowerCoalLow':'coal-plant','PowerCoalMed':'coal-plant','PowerCoalFull':'coal-plant','EvolutionWarehouse':'warehouse','EvolutionTech':'tech-campus','EvolutionCafe':'cafe','EvolutionFoodhall':'food-hall','ResidentialLowLow':'residence-basic-low','ResidentialMedLow':'residence-basic-medium','ResidentialHighLow':'residence-basic-high','ResidentialLow':'residence-modern-low','ResidentialMedHigh':'residence-modern-medium','ResidentialHighHigh':'residence-modern-high'}
for x in ['L','M','H']:mapr['IndustryLight'+x]='factory';mapr['IndustryHigh'+x]='heavy-factory'
path=tiles/'images.xml';tree=E.parse(str(path));used={}
for group in tree.getroot().findall('group'):
 rid=None
 for node in group:
  if node.tag=='resourceID':rid=node.get('name')
  elif node.tag=='image' and rid in mapr:
   n=mapr[rid];r=rows[n];node.attrib.update({'file':Path(r['file']).name,'x':str(r['anchor'][0]),'y':str(r['anchor'][1])});used[rid]=n
# Old smoke/swing overlays use fixed offsets for old buildings; keep frame counts but suppress misplaced effects.
Image.new('RGBA',(2,2)).save(tiles/'revamp-clear.png')
for group in tree.getroot().findall('group'):
 rid=None
 for node in group:
  if node.tag=='resourceID':rid=node.get('name')
  elif node.tag=='image' and rid in ['GraySmoke','BlackSmoke','ChildOnSwing']:node.attrib.update({'file':'revamp-clear.png','x':'0','y':'0'})
for old in list(tree.getroot()):
 if old.find('resourceID') is not None and old.find('resourceID').get('name','').startswith('Transit'):tree.getroot().remove(old)
for rid,n in [('TransitBusStop','bus-stop'),('TransitBusTerminal','bus-terminal'),('TransitRail','rail-station'),('TransitMetro','metro-entrance'),('TransitElevated','elevated-station')]:
 g=E.SubElement(tree.getroot(),'group');E.SubElement(g,'resourceID',name=rid);r=rows[n];E.SubElement(g,'image',file=Path(r['file']).name,x=str(r['anchor'][0]),y=str(r['anchor'][1]));used[rid]=n
# Track layer construction groups use a tiny neutral marker only for the placement cursor.
for rid in ['TransitTunnel','TransitViaduct']:
 g=E.SubElement(tree.getroot(),'group');E.SubElement(g,'resourceID',name=rid);E.SubElement(g,'image',file='revamp-clear.png',x='0',y='0')
tree.write(str(path),encoding='UTF-8',xml_declaration=True,pretty_print=True)
# Fit a picture and readable caption inside the existing 72x64 toolbar system.
font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',10)
for key,n,label in [('BusStop','bus-stop','Bus stop'),('BusTerminal','bus-terminal','Terminal'),('RailStation','rail-station','Rail station'),('MetroStation','metro-entrance','Metro'),('ElevatedStation','elevated-station','Elevated'),('MetroTrack',None,'Tunnel'),('ElevatedTrack',None,'Viaduct')]:
 icon=Image.new('RGBA',(72,64));d=ImageDraw.Draw(icon)
 if n:
  im=Image.open(s/'data'/rows[n]['file']);im.thumbnail((64,44),Image.Resampling.LANCZOS);icon.alpha_composite(im,((72-im.width)//2,2+(44-im.height)//2))
 else:
  col='#65d1c0' if key=='MetroTrack' else '#d4d0ac'
  for dy in [-4,4]:d.line([(10,33+dy),(60,17+dy)],fill=col,width=3)
  for x in range(16,60,8):
   y=33-(x-10)*16/50;d.line([(x-2,y-5),(x+2,y+5)],fill='#b6c8c0',width=2)
   if key=='ElevatedTrack':d.line([(x,y+5),(x,y+17)],fill='#aab8b2',width=2)
 d.text((36,57),label,font=font,fill='#f1efdf',anchor='mm');icon.save(s/'data/images/readable'/('BPM'+key+'Button.png'))
# Branded native application icon, drawn as a code-native city mark.
im=Image.new('RGBA',(256,256),'#14252d');d=ImageDraw.Draw(im);d.rounded_rectangle((10,10,246,246),radius=36,outline='#6bc6a1',width=8)
for x,y,h in [(45,112,88),(99,55,145),(153,88,112)]:
 d.rectangle((x,y,x+42,200),fill='#6bc6a1')
 for wx in range(x+9,x+37,14):
  for wy in range(y+12,190,20):d.rectangle((wx,wy,wx+5,wy+7),fill='#14252d')
for size,suffix in [(128,''),(256,'_2x')]:im.resize((size,size),Image.Resampling.LANCZOS).save(s/'data'/('io.github.lincity_ng.lincity-ng'+suffix+'.png'))
im.save(s/'data/io.github.lincity_ng.lincity-ng.ico',sizes=[(16,16),(32,32),(48,48),(64,64),(128,128),(256,256)])
(a/'manifest.json').write_text(json.dumps({'projection':'isometric 2:1','note':'Static building sprites; original animation frame slots retained. Ground anchors require Windows visual QA. Transit and cars move along routes; horse, cart and truck are decorative. Legacy terrain and some buildings remain.','resource_bindings':used,'assets':rows},indent=2))
# Review contact sheet from the actual exported engine sprites.
ns=list(sizes);sheet=Image.new('RGB',(1000,((len(ns)+4)//5)*164),'#14252d');d=ImageDraw.Draw(sheet)
for i,n in enumerate(ns):
 im=Image.open(s/'data'/rows[n]['file']);im.thumbnail((182,132),Image.Resampling.LANCZOS);x=(i%5)*200;y=(i//5)*164;sheet.paste(im,(x+(200-im.width)//2,y+(136-im.height)//2),im);d.text((x+100,y+148),n,font=font,fill='#f1efdf',anchor='mm')
sheet.save(a/'model-gallery.jpg',quality=92)
# Scrollable City Hall pages: fixed content height, clipped by a viewport; text cannot spill into lower rows.
p=s/'data/gui/cityhall.xml';t=E.parse(str(p));example=E.parse(str(s/'data/gui/helpwindow.xml')).find('.//ScrollView/scrollbar')
for panel in t.findall('.//SwitchComponent[@name="HallPages"]/Panel'):
 parent=panel.getparent();idx=parent.index(panel);name=panel.get('name');layout=panel.find('TableLayout');total=0
 for rs in list(layout.findall('rowsize')):layout.remove(rs)
 for row in range(1,int(layout.get('rows'))+1):
  cells=layout.findall(f'cell[@row="{row}"]');ps=[c.find('Paragraph') for c in cells if c.find('Paragraph') is not None]
  height=44 if not ps else 64
  if any(q.get('name','').startswith(('Advice','DistDetail','MayorExplain','PlanL')) for q in ps):height=100
  E.SubElement(layout,'rowsize',row=str(row),fixed=str(height));total+=height
 panel.set('name',name+'Contents');panel.set('resizable','no');panel.set('width','748');panel.set('height',str(total))
 scroll=E.Element('ScrollView',name=name);scroll.append(copy.deepcopy(example));contents=E.SubElement(scroll,'contents');parent.remove(panel);contents.append(panel);parent.insert(idx,scroll)
t.write(str(p),encoding='UTF-8',xml_declaration=True,pretty_print=True)
print('Exported',len(rows),'sprites;',len(used),'resource bindings; 7 transport icons and scrollable City Hall pages')
