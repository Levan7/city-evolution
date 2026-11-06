from pathlib import Path
from lxml import etree as E
from PIL import Image, ImageDraw, ImageFont
import ast,copy,ctypes as C,json,math,shutil
import sys
SRC=Path(__file__).resolve().parents[2]
if len(sys.argv)!=2: raise SystemExit('Usage: python3 contrib/revamp/rebuild_preview10.py /absolute/path/to/extracted/CityEvolution-Preview-10')
GAME=Path(sys.argv[1]).resolve()
if not (GAME/'Game/bin/lincity-ng.exe').is_file():raise SystemExit('Pass the extracted portable game folder, containing Play.cmd and Game/')
ROOT=SRC;OLD=SRC;ART=SRC/'art/preview10'
DATA=SRC/'data';TILES=DATA/'images/tiles';READ=DATA/'images/readable'
def write(p,t): p.write_bytes(E.tostring(t,xml_declaration=True,encoding='UTF-8',pretty_print=True))
def el(parent,tag,**attrs): return E.SubElement(parent,tag,{k.replace('_','-'):str(v) for k,v in attrs.items()})
def cell(parent,row,col,**attrs):return el(parent,'cell',row=row,col=col,**attrs)
def para(parent,text,size=16,color='#eef1df',**attrs):
 e=el(parent,'Paragraph',color=color,font_size=size,**attrs);e.text=text;return e
def image(parent,tag,src,w,h):return el(parent,tag,src=src,width=w,height=h,resizable='yes')
def plate(button,prefix,w,h,checked=False):
 for tag,state in [('image','normal'),('image-hover','hover'),('image-clicked','clicked')]+([('image-checked','checked')] if checked else []):
  image(button,tag,prefix+state+'.png',w,h)
def resources(tree):
 out={}
 for group in tree.findall('group'):
  name=None
  for n in group:
   if n.tag=='resourceID':name=n.get('name');out[name]=[]
   elif n.tag=='image' and name:out[name].append(n)
 return out
original=E.parse(str(TILES/'images.xml'));before=resources(original)
saved=json.loads((ART/'before10-frame-layout.json').read_text())
for rid,attrs in saved.items():
 for node,attr in zip(before[rid],attrs):node.attrib.clear();node.attrib.update(attr)
native={rid:[E.Element('image',**a) for a in attrs] for rid,attrs in json.loads((ART/'native-frame-layout.json').read_text()).items()}
BASE=SRC
# SVG rendering uses the same native libraries as the game build tooling.
r=C.CDLL('librsvg-2.so.2');c=C.CDLL('libcairo.so.2');g=C.CDLL('libgobject-2.0.so.0')
class Rect(C.Structure):_fields_=[('x',C.c_double),('y',C.c_double),('width',C.c_double),('height',C.c_double)]
r.rsvg_handle_new_from_data.argtypes=[C.c_char_p,C.c_size_t,C.c_void_p];r.rsvg_handle_new_from_data.restype=C.c_void_p
r.rsvg_handle_render_document.argtypes=[C.c_void_p,C.c_void_p,C.POINTER(Rect),C.c_void_p];r.rsvg_handle_render_document.restype=C.c_int
c.cairo_image_surface_create.argtypes=[C.c_int,C.c_int,C.c_int];c.cairo_image_surface_create.restype=C.c_void_p
c.cairo_create.argtypes=[C.c_void_p];c.cairo_create.restype=C.c_void_p
c.cairo_surface_write_to_png.argtypes=[C.c_void_p,C.c_char_p];c.cairo_destroy.argtypes=[C.c_void_p];c.cairo_surface_destroy.argtypes=[C.c_void_p];g.g_object_unref.argtypes=[C.c_void_p]
def render(svg,path,w,h):
 raw=svg if isinstance(svg,bytes) else svg.encode();handle=r.rsvg_handle_new_from_data(raw,len(raw),None)
 surface=c.cairo_image_surface_create(0,w,h);context=c.cairo_create(surface)
 assert r.rsvg_handle_render_document(handle,context,C.byref(Rect(0,0,w,h)),None)
 assert c.cairo_surface_write_to_png(surface,str(path).encode())==0
 c.cairo_destroy(context);c.cairo_surface_destroy(surface);g.g_object_unref(handle)
# Export a single shared canvas and lot pivot per animated building.
specs={'pottery':(3,1,2),'blacksmith':(3,1,2),'mill':(3,1,2),'commune':(3,1,4),'ore-mine':(3,1,4),'coal-mine':(1,1,4),'landfill':(3,1,4),'port':(1,1,4),'sports':(3,1,2),'monument':(3,2,2),'rocket':(4,2,4),'park':(1,1,1),'shanty':(1,1,1),'wind-turbine':(3,1,2),'heavy-factory':(3,1,4),'fire-station':(3,1,2),'farm':(3,2,4),'school':(3,1,2),'park-lake':(1,1,1),'factory':(1,1,3),'coal-plant':(1,1,4),'native-car':(4,4,0)}
exports={}
for name,(cols,rows,fp) in specs.items():
 im=Image.open(ART/(name+'.png')).convert('RGBA');cw=math.ceil(im.width/cols);ch=math.ceil(im.height/rows)
 frames=[];bbs=[]
 for y in range(rows):
  for x in range(cols):
   frame=Image.new('RGBA',(cw,ch));part=im.crop((round(x*im.width/cols),round(y*im.height/rows),round((x+1)*im.width/cols),round((y+1)*im.height/rows)))
   frame.paste(part,(0,0));bb=frame.getchannel('A').point(lambda a:255 if a>32 else 0).getbbox()
   assert bb,(name,x,y);frames.append(frame);bbs.append(bb)
 union=(min(b[0] for b in bbs),min(b[1] for b in bbs),max(b[2] for b in bbs),max(b[3] for b in bbs))
 width=128*fp if fp else 48;scale=(width-8)/(union[2]-union[0]);height=math.ceil((union[3]-union[1])*scale)+8
 anchor=[width//2,round(4+(bbs[0][3]-union[1])*scale)]
 output=[]
 for i,frame in enumerate(frames):
  canvas=Image.new('RGBA',(width,height));canvas.paste(frame.crop(union).resize((width-8,height-8),Image.Resampling.LANCZOS),(4,4))
  pivot=anchor
  if name=='native-car':
   old=native['Bluecar'][i];oi=Image.open(BASE/'data/images/tiles'/old.get('file'))
   ax=((256-4*oi.width)/2+4*int(old.get('x')))/256*cw
   ay=((256-4*oi.height)/2+4*int(old.get('y')))/256*ch
   pivot=[round(4+(ax-union[0])*scale),round(4+(ay-union[1])*scale)]
  fn=f'revamp10-{name}-{i:02}.png';canvas.save(TILES/fn)
  output.append({'file':fn,'x':pivot[0],'y':pivot[1]})
 exports[name]={'atlas':name+'.png','grid':[cols,rows],'footprint':fp,'size':[width,height],'anchor':anchor,'frames':output}
(ART/'exports.json').write_text(json.dumps(exports,indent=2))
bindings={}
def bind(rid,name,mode='cycle'):
 if rid not in before:raise KeyError(rid)
 nodes=before[rid];fs=exports[name]['frames'];count=len(fs)
 for i,node in enumerate(nodes):
  if mode=='direct':j=min(i,count-1)
  elif mode=='states':j=min(count-1,int(i*count/len(nodes)))
  elif mode=='finished':j=count-1
  elif mode=='pingpong':
   phase=i*2*(count-1)/max(1,len(nodes)-1);j=round(phase if phase<=count-1 else 2*(count-1)-phase)
  else:j=min(count-1,(i-1)*count//(len(nodes)-1)) if len(nodes)>count and i>0 else i%count
  node.attrib.clear();node.attrib.update({k:str(v) for k,v in fs[j].items()})
 bindings[rid]=name
for rid,name,mode in [('Pottery','pottery','pingpong'),('Blacksmith','blacksmith','pingpong'),('Mill','mill','cycle'),('Commune','commune','cycle'),('Tip','landfill','states'),('Harbor','port','cycle'),('SportsCroud','sports','pingpong'),('Monument','monument','finished'),('MonumentConstruction','monument','direct'),('Rocket','rocket','direct'),('ParklandPlane','park','cycle'),('ParklandLake','park-lake','cycle'),('Shanty','shanty','cycle'),('FireStation','fire-station','pingpong'),('Farm','farm','direct'),('Bluecar','native-car','direct')]:bind(rid,name,mode)
for rid in before:
 for start,name in [('OreMine','ore-mine'),('CoalMine','coal-mine'),('WindMill','wind-turbine'),('IndustryHigh','heavy-factory'),('IndustryLight','factory'),('PowerCoal','coal-plant')]:
  if rid.startswith(start):bind(rid,name,'pingpong' if start=='OreMine' else 'cycle')
# School's animated child overlay is a complete aligned school frame, with a clear second base slot.
bind('School','school','direct')
clear=TILES/'revamp-clear.png';Image.new('RGBA',(1,1)).save(clear)
for n in before['School'][1:]:n.attrib.clear();n.attrib.update({'file':'revamp-clear.png','x':'0','y':'0'})
bind('ChildOnSwing','school','pingpong')
for rid in ['GraySmoke','BlackSmoke']:
 assert len(before[rid])==len(native[rid])
 for n,o in zip(before[rid],native[rid]):n.attrib.clear();n.attrib.update(o.attrib)
write(TILES/'images.xml',original)
(ART/'resource-bindings.json').write_text(json.dumps(bindings,indent=2))
manifest=json.loads((SRC/'art/manifest.json').read_text());manifest['resource_bindings'].update(bindings)
for name,meta in exports.items():
 manifest['assets'][name]={'file':'images/tiles/'+meta['frames'][0]['file'],'size':meta['size'],'anchor':meta['anchor'],'footprint':meta['footprint'],'frames':meta['frames']}
manifest['note']='Preview 10: original native animation slot counts and timings retained; most working buildings cycle three distinct new poses. Native smoke restored (10 gray/11 black frames); native car has 16 views. Terrain, road/rail/powerline, bridges and disasters remain upstream; horses/carts/trucks are decorative.'
(SRC/'art/manifest.json').write_text(json.dumps(manifest,indent=2))
# Main menu: original city scene, preserving authorship, replacing only the upstream logo group.
svg=E.parse(str(DATA/'images/mainmenu/menu_back.svg'))
for n in svg.xpath('//*[@id="LinCity"]'):n.getparent().remove(n)
write(DATA/'images/mainmenu/menu-classic-cityevolution.svg',svg)
render(E.tostring(svg),DATA/'images/mainmenu/menu-classic-cityevolution.png',1536,1152)
menu=E.parse(str(DATA/'gui/mainmenu.xml'));root=menu.getroot()
for name in ['boxtext','dialogbutton']:
 if not root.xpath('./DefineStyle[@name="'+name+'"]'):root.insert(0,E.Element('DefineStyle',name=name,color='#eef1e3',**{'font-size':'18'}))
switch=root.find('SwitchComponent')
for n in list(switch):switch.remove(n)
panel=el(switch,'Panel',name='main-menu',resizable='yes')
el(panel,'Image',src='images/mainmenu/menu-classic-cityevolution.png',resizable='yes')
outer=el(panel,'TableLayout',rows=3,cols=3)
for k in [1,3]:el(outer,'colsize',col=k,fixed=24);el(outer,'rowsize',row=k,relative=1)
el(outer,'rowsize',row=2,fixed=476)
middle=el(cell(outer,2,2),'TableLayout',rows=1,cols=2);el(middle,'colsize',col=2,fixed=360)
buttons=el(cell(middle,1,2),'TableLayout',rows=11,cols=1)
for row,height in [(1,58),(2,32)]+[(i,48) for i in range(3,10)]+[(10,16),(11,34)]:el(buttons,'rowsize',row=row,fixed=height)
para(cell(buttons,1,1),'CITY EVOLUTION',30,'#14352d',font_weight='bold',halign='center')
para(cell(buttons,2,1),'Build a city. Shape its future.',16,'#23473a',halign='center')
for row,(name,label) in enumerate([('ContinueButton','Continue city'),('NewGameButton','New world'),('LoadButton','Load city'),('SaveButton','Save city'),('OptionsButton','Settings'),('CreditsButton','Credits'),('QuitButton','Exit')],3):
 b=el(cell(buttons,row,1,halign='center',valign='center'),'Button',name=name,width=320,height=42);plate(b,'images/evolution/main-',320,42)
 el(b,'text-caption',color='#f2f1e4',halign='center',font_size=19).text=label
para(cell(buttons,11,1),'Preview 10 | A fork of LinCity-NG.',15,'#23473a',halign='center')
for n in ['newgame','loadgame','savegame','credits','options']:el(switch,'Import',src='gui/'+n+'.xml')
write(DATA/'gui/mainmenu.xml',menu)
# Category rail and one mutually exclusive drawer per family; all original tool IDs preserved.
categories=[
('Tools','BPPointerButton','Pointer BullDoze Water Evacuate','pointer'),
('Homes','BPResidenceButton','Residence1 Residence2 Residence3 Residence4 Residence5 Residence6','house'),
('Food','BPFarmButton','Market Farm Waterwell Cafe FoodHall','farm'),
('Services','BPSchoolButton','School University Fire Health Sports','book'),
('Streets','BPTransportButton','Track Street','road'),
('Buses','BPBusButton','BusStop BusTerminal','bus'),
('Rail','BPRailButton','Rail RailStation','rail'),
('Metro','BPMetroButton','MetroTrack MetroStation','metro'),
('Elevated','BPElevatedButton','ElevatedTrack ElevatedStation','elevated'),
('Freight','BPFreightButton','Port Warehouse','warehouse'),
('Power','BPPowerButton','PowerLine CoalPS SolarPS Substation Windmill','bolt'),
('Mining','BPMiningButton','Coal Ore','ore'),
('Industry','BPIndustryButton','Pottery Blacksmith Mill LIndustry HIndustry TechCampus','factory'),
('Land','BPLandButton','Park Tip Recycle Commune','park'),
('Civic','BPCivicButton','Monument Rocket','monument')]
oldbp=E.parse(str(ART/'before10-buttonpanel.xml'));tools={n.get('name'):copy.deepcopy(n) for n in oldbp.findall('menu/tool')}
bp=E.Element('ButtonPanel',name='ButtonPanel',width='432',height='400')
for label,name,keys,glyph in categories:
 names=['BPM'+k+'Button' for k in keys.split()];m=el(bp,'menu',button_name=name,drawer_name=name+'Drawer',default=names[0])
 for tool in names:m.append(copy.deepcopy(tools[tool]))
content=el(bp,'content');layout=el(content,'TableLayout',rows=1,cols=2)
el(layout,'colsize',col=1,fixed=112);el(layout,'colsize',col=2,fixed=320)
rail=el(cell(layout,1,1),'TableLayout',rows=2,cols=1);el(rail,'rowsize',row=1,fixed=360);el(rail,'rowsize',row=2,fixed=40)
grid=el(cell(rail,1,1),'TableLayout',rows=8,cols=2)
for row in range(1,9):el(grid,'rowsize',row=row,fixed=45)
drawers=el(cell(layout,1,2),'SwitchComponent',name='ConstructionDrawers')
font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',11)
tree=ast.parse((SRC/'contrib/revamp/prior_build_icons.py').read_text())
symbols=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(a,ast.Name) and a.id=='S' for a in n.targets))
symbols.update(bus='<rect x="7" y="7" width="34" height="32" rx="6" fill="#7fb6a7"/><path d="M12 14h24v13H12Z"/><circle cx="15" cy="41" r="4"/><circle cx="33" cy="41" r="4"/>',metro='<path d="M5 43V23a19 19 0 0138 0v20M12 43l5-13h14l5 13M16 33h16M18 39h12"/><path d="M15 14v12m0-12 9 9 9-9v12"/>',elevated='<path d="M3 29h42M9 29v16m30-16v16M7 22h34V7H7Z M12 11h9v7h-9m15-7h9v7h-9"/>')
for idx,(label,name,keys,glyph) in enumerate(categories):
 icon=READ/(name+'-glyph.png');render('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48"><g stroke="#efd9b1" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">'+symbols[glyph]+'</g></svg>',icon,30,30)
 caption=Image.new('RGBA',(54,43));caption.alpha_composite(Image.open(icon).convert('RGBA'),(12,0));ImageDraw.Draw(caption).text((27,35),label,font=font,fill='#f2efdf',anchor='mm');caption.save(READ/(name+'-category.png'))
 b=el(cell(grid,idx//2+1,idx%2+1),'CheckButton',name=name,width=54,height=43,lower='true',auto_check='false',auto_uncheck='false')
 plate(b,'images/readable/plate-72-',54,43,True);image(b,'image-caption','images/readable/'+name+'-category.png',54,43);el(b,'tooltip').text=label
 p=el(drawers,'Panel',name=name+'Drawer',resizable='yes');el(p,'FilledRectangle',color='#132c2bf2')
 inner=el(p,'TableLayout',rows=4,cols=3)
 for col in [1,3]:el(inner,'colsize',col=col,fixed=12)
 for row,height in [(1,36),(2,24),(3,240)]:el(inner,'rowsize',row=row,fixed=height)
 para(cell(inner,1,2),label.upper(),18);para(cell(inner,2,2),'Choose a building or route',13,'#c3d0bc')
 choices=el(cell(inner,3,2),'TableLayout',rows=3,cols=3)
 for row in [1,2,3]:el(choices,'rowsize',row=row,fixed=80)
 for i,key in enumerate(keys.split()):
  tool='BPM'+key+'Button';b=el(cell(choices,i//3+1,i%3+1,halign='center',valign='center'),'CheckButton',name=tool,width=88,height=76,auto_uncheck='false')
  plate(b,'images/readable/plate-72-',88,76,True);image(b,'image-caption','images/readable/'+tool+'-building.png',88,76)
report=Image.new('RGBA',(108,34));report.alpha_composite(Image.open(READ/'BPBusButton-glyph.png').convert('RGBA'),(4,2));d=ImageDraw.Draw(report);d.text((72,10),'Transit',font=font,fill='#f2efdf',anchor='mm');d.text((72,23),'report',font=font,fill='#f2efdf',anchor='mm');report.save(READ/'TransitReportButton.png')
b=el(cell(rail,2,1),'Button',name='TransportButton',width=108,height=34);plate(b,'images/evolution/wide-',108,34);image(b,'image-caption','images/readable/TransitReportButton.png',108,34);el(b,'tooltip').text='Passenger transport report'
write(DATA/'gui/buttonpanel.xml',bp)
# Each build choice is derived from the exact in-game resource, rather than a generic category symbol.
thumbs={'Residence1':'ResidentialLowLow','Residence2':'ResidentialMedLow','Residence3':'ResidentialHighLow','Residence4':'ResidentialLow','Residence5':'ResidentialMedHigh','Residence6':'ResidentialHighHigh','Market':'MarketEmpty','Waterwell':'Waterwell','Farm':'Farm','Park':'ParklandPlane','Cafe':'EvolutionCafe','FoodHall':'EvolutionFoodhall','Monument':'Monument','School':'School','University':'University','Fire':'FireStation','Health':'Health','Sports':'SportsCroud','Track':'Track','Street':'Road','Rail':'Rail','Port':'Harbor','Rocket':'Rocket','Warehouse':'EvolutionWarehouse','BusStop':'TransitBusStop','BusTerminal':'TransitBusTerminal','RailStation':'TransitRail','MetroStation':'TransitMetro','ElevatedStation':'TransitElevated','PowerLine':'PowerLine','CoalPS':'PowerCoalEmpty','SolarPS':'PowerSolar','Substation':'Substation','Windmill':'WindMill','Commune':'Commune','Coal':'CoalMine','Ore':'OreMine','Tip':'Tip','Recycle':'Recycle','Pottery':'Pottery','Blacksmith':'Blacksmith','Mill':'Mill','LIndustry':'IndustryLight','HIndustry':'IndustryHigh','TechCampus':'EvolutionTech'}
labels={'Residence1':'Basic low','Residence2':'Basic medium','Residence3':'Basic high','Residence4':'Modern low','Residence5':'Modern medium','Residence6':'Modern high','BullDoze':'Bulldoze','Water':'Water','Evacuate':'Evacuate','Pointer':'Inspect','CoalPS':'Coal plant','SolarPS':'Solar plant','LIndustry':'Light industry','HIndustry':'Heavy industry','Tip':'Landfill','Coal':'Coal mine','Ore':'Ore mine','Windmill':'Wind turbine'}
for _,_,keys,_ in categories:
 for key in keys.split():
  tool='BPM'+key+'Button'
  if key in thumbs:pic=Image.open(TILES/before[thumbs[key]][0].get('file')).convert('RGBA')
  else:pic=Image.open(READ/(tool+'.png')).convert('RGBA').crop((0,0,72,46))
  bb=pic.getchannel('A').point(lambda a:255 if a>32 else 0).getbbox()
  if bb:pic=pic.crop(bb)
  pic.thumbnail((80,50),Image.Resampling.LANCZOS);caption=Image.new('RGBA',(88,76));caption.alpha_composite(pic,((88-pic.width)//2,2+(50-pic.height)//2))
  import re
  label=labels.get(key,re.sub(r'(?<=[a-z])(?=[A-Z])',' ',key));words=label.split();lines=['']
  d=ImageDraw.Draw(caption)
  for word in words:
   attempt=(lines[-1]+' '+word).strip()
   if d.textlength(attempt,font=font)>84 and lines[-1]:lines.append(word)
   else:lines[-1]=attempt
  for i,line in enumerate(lines[:2]):d.text((44,58+12*i),line,font=font,fill='#f2efdf',anchor='mm')
  caption.save(READ/(tool+'-building.png'))
(ART/'thumbnail-bindings.json').write_text(json.dumps(thumbs,indent=2))
# Sidebar reserves real room for category rail, speed controls and map.
app=E.parse(str(DATA/'gui/app.xml'));a=app.getroot();main=a.find('TableLayout')
main.find("colsize[@col='1']").set('fixed','120')
mayor=a.xpath('.//Button[@name="MayorButton"]')[0];top=mayor.getparent().getparent().getparent();nested=mayor.getparent().getparent();mayor.getparent().remove(mayor);top.remove(nested);top.append(mayor)
for n in a.xpath('.//rowsize[@fixed="432"]'):n.set('fixed','400')
for n in a.xpath('.//rowsize[@fixed="200"]'):n.set('fixed','184')
write(DATA/'gui/app.xml',app)
speed=E.Element('Panel',width='112',height='184');el(speed,'FilledRectangle',color='#132c2bf2')
t=el(speed,'TableLayout',rows=5,cols=2)
for row,h in [(1,24),(2,36),(3,36),(4,40),(5,40)]:el(t,'rowsize',row=row,fixed=h)
para(cell(t,1,1,colspan=2),'GAME SPEED',12,halign='center')
for i,(name,tip) in enumerate([('SpeedPauseButton','Pause'),('SpeedNormalButton','Normal speed'),('SpeedFastButton','Fast speed'),('SpeedFastestButton','Fastest speed')]):
 b=el(cell(t,i//2+2,i%2+1,halign='center'),'CheckButton',name=name,width=44,height=34)
 for tag,state in [('image','normal'),('image-hover','hover'),('image-checked','checked')]:image(b,tag,'images/evolution/speed-'+state+'.png',44,34)
 image(b,'image-caption','images/evolution/'+name+'.png',44,34);el(b,'tooltip').text=tip
for row,name,label in [(4,'CityAdvisorsButton','City Hall'),(5,'GameMenuButton','Menu / Save')]:
 b=el(cell(t,row,1,colspan=2,halign='center'),'Button',name=name,width=108,height=32);plate(b,'images/evolution/wide-',108,32);el(b,'text-caption',color='#edf1df',halign='center',font_size=13).text=label
write(DATA/'gui/speedpanel.xml',speed)
# Full Windows distribution; retained Preview 9 executable and dependencies.
PKG=GAME/'Game/share/lincity-ng';shutil.copytree(DATA/'images',PKG/'images',dirs_exist_ok=True)
transform=E.XSLT(E.parse(str(DATA/'gui/identity.xsl')))
for name in ['mainmenu','app','buttonpanel','speedpanel']:
 result=transform(E.parse(str(DATA/'gui'/f'{name}.xml')),NLS=E.XSLT.strparam('disabled'));(PKG/'gui'/f'{name}.xml').write_bytes(bytes(result))
diagnostic=(GAME/'Diagnose-Startup.cmd').read_text().replace('Preview 9','Preview 10').replace('Preview-9','Preview-10')
(GAME/'Diagnose-Startup.cmd').write_text(diagnostic,newline='\r\n')
p=GAME/'Play.cmd';p.write_text(p.read_text().replace('Preview 9','Preview 10').replace('Preview-9','Preview-10'),newline='\r\n')
# Remove replaced old art from the playable package only. Source preserves its inputs.
candidates={n['file'] for rid in set(bindings)|{'GraySmoke','BlackSmoke'} for n in saved[rid]}
candidates|={n.get('file') for rid in set(bindings)|{'GraySmoke','BlackSmoke'} for n in native[rid]}
active={n.get('file') for n in original.findall('.//image')}
textrefs='\n'.join(p.read_text(errors='replace') for p in PKG.rglob('*') if p.is_file() and p.suffix in ['.xml','.xsl','.svg'])
textrefs+='\n'.join(p.read_text(errors='replace') for p in (SRC/'src').rglob('*') if p.suffix in ['.cpp','.hpp'])
removed=[]
for fn in sorted(candidates-active):
 p=PKG/'images/tiles'/fn
 if p.exists() and fn not in textrefs:p.unlink();removed.append(fn)
for p in (PKG/'images/tiles').glob('revamp10-*'):
 if p.name not in active:p.unlink()
for p in GAME.rglob('cmake_install.cmake'):p.unlink()
(ART/'pruned-package-tiles.json').write_text(json.dumps(removed,indent=2))
# Contact sheet of real exported sprites, for review.
gallery=Image.new('RGB',(1100,math.ceil(len(exports)/5)*180),'#d5ddcd');dr=ImageDraw.Draw(gallery)
for i,(name,meta) in enumerate(exports.items()):
 pic=Image.open(TILES/meta['frames'][0]['file']).convert('RGBA');pic.thumbnail((204,145),Image.Resampling.LANCZOS)
 x=i%5*220+(220-pic.width)//2;y=i//5*180;gallery.paste(pic,(x,y),pic);dr.text((i%5*220+10,y+152),name,font=font,fill='#14352d')
gallery.save(ART/'sprite-review.jpg')
summary={'revision':'Preview 10','sprite_sets':len(exports),'categories':len(categories),'tools':len(tools),'building_thumbnails':len(thumbs),'resource_bindings_changed':len(bindings),'legacy_package_tiles_removed':len(removed),'native_executable_rebuilt':False,'windows_interactive_tested':False}
(SRC/'validation').mkdir(exist_ok=True);(SRC/'validation/preview10-summary.json').write_text(json.dumps(summary,indent=2))
print(json.dumps(summary,indent=2))
