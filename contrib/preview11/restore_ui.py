from pathlib import Path
from lxml import etree as E
from PIL import Image,ImageDraw,ImageFont
import copy,json,shutil
W=Path(__file__).resolve().parent;S=W/'CityEvolution-Preview-11-Source';G=W/'CityEvolution-Preview-11';D=S/'data';A=S/'art/preview11';R=D/'images/readable';T=D/'images/tiles'
def el(p,t,**a):return E.SubElement(p,t,{k.replace('_','-'):str(v) for k,v in a.items()})
def cell(p,r,c,**a):return el(p,'cell',row=r,col=c,**a)
def save(p,t):p.write_bytes(E.tostring(t,encoding='UTF-8',xml_declaration=True,pretty_print=True))
def img(b,t,src,w,h):el(b,t,src=src,width=w,height=h,resizable='yes')
def plate(b,p,w,h):
 for t,st in [('image','normal'),('image-hover','hover'),('image-clicked','clicked')]:img(b,t,p+st+'.png',w,h)
def button(p,name,label,w,h,prefix='images/evolution/wide-',fs=16):
 b=el(p,'Button',name=name,width=w,height=h);plate(b,prefix,w,h);el(b,'text-caption',color='#edf1df',halign='center',font_size=fs).text=label;return b
def clean(im):
 im=im.convert('RGBA');im.putalpha(im.getchannel('A').point(lambda a:a if a>=128 else 0));bb=im.getchannel('A').getbbox();assert bb;return im.crop(bb)
# Restore the church and moving horse art previously generated.
church=clean(Image.open(A/'church.png'));church.thumbnail((376,640),Image.Resampling.LANCZOS)
out=Image.new('RGBA',(384,church.height+8));out.alpha_composite(church,((384-church.width)//2,4));out.save(T/'revamp11-church.png')
tree=E.parse(str(T/'images.xml'));root=tree.getroot();grp=el(root,'group');el(grp,'resourceID',name='GeorgianChurch');el(grp,'image',file='revamp11-church.png',x=192,y=out.height-4)
atlas=Image.open(A/'horse-traffic-clean.png');traffic={}
for row,stem,width in [(0,'horse-walk',38),(1,'carriage',64)]:
 for col,di in enumerate(['SE','SW','NW','NE']):
  pic=clean(atlas.crop((round(col*atlas.width/4),round(row*atlas.height/2),round((col+1)*atlas.width/4),round((row+1)*atlas.height/2))))
  h=round(pic.height*(width-4)/pic.width)+4;pic=pic.resize((width-4,h-4),Image.Resampling.LANCZOS);o=Image.new('RGBA',(width,h));o.alpha_composite(pic,(2,2));o.save(D/'images/transit'/f'{stem}-{di}.png');traffic[stem+'-'+di]=o
grp=el(root,'group');el(grp,'resourceID',name='HorseTraffic');dirs=['SW','SE','SE','SE','NE','NW','NW','SW','SW','SE','NE','NW','SW','SE','NE','NW'];mh=max(traffic['carriage-'+d].height for d in ['SE','SW','NW','NE'])
for i,di in enumerate(dirs):
 pic=traffic['carriage-'+di];o=Image.new('RGBA',(64,mh));o.alpha_composite(pic,(0,mh-pic.height));fn=f'revamp11-native-carriage-{i:02}.png';o.save(T/fn);el(grp,'image',file=fn,x=32,y=mh-3)
save(T/'images.xml',tree)
m=json.loads((S/'art/manifest.json').read_text());m['resource_bindings']['GeorgianChurch']='georgian-orthodox-church';m['assets']['georgian-orthodox-church']={'file':'images/tiles/revamp11-church.png','size':list(out.size),'anchor':[192,out.height-4],'footprint':3};(S/'art/manifest.json').write_text(json.dumps(m,indent=2))
# Left construction rail, without a report button; new church in Civic.
bp=E.parse(str(D/'gui/buttonpanel.xml'));bp.getroot().set('height','360')
rail=bp.find('content/TableLayout/cell/TableLayout');rail.set('rows','1')
for n in list(rail):
 if n.get('row')=='2':rail.remove(n)
civic=bp.find("menu[@button-name='BPCivicButton']");el(civic,'tool',name='BPMChurchButton',grpid=62,help='church')
choices=bp.find(".//Panel[@name='BPCivicButtonDrawer']/TableLayout/cell[@row='3']/TableLayout");b=copy.deepcopy(choices.find('cell/CheckButton'));b.set('name','BPMChurchButton');b.find('image-caption').set('src','images/readable/BPMChurchButton-building.png');cell(choices,1,3,halign='center',valign='center').append(b);save(D/'gui/buttonpanel.xml',bp)
# Top right report under Mayor, with explicit building-view status/reset.
app=E.parse(str(D/'gui/app.xml'));layout=app.find('TableLayout');layout.find("rowsize[@row='1']").set('fixed','124');layout.find("colsize[@col='1']").set('fixed','208');top=layout.find("cell[@row='1'][@col='3']")
for n in list(top):top.remove(n)
stack=el(top,'TableLayout',rows=3,cols=1)
for row,h in [(1,44),(2,44),(3,32)]:el(stack,'rowsize',row=row,fixed=h)
for row,name,label,h in [(1,'MayorButton','Mayor',36),(2,'TransportButton','Transport report',36),(3,'ShowBuildingsButton','Building models visible',28)]:button(cell(stack,row,1,halign='right',valign='center'),name,label,200,h,fs=16 if row<3 else 13)
for n in app.xpath('.//rowsize[@fixed="400"]'):n.set('fixed','360')
for n in app.xpath('.//rowsize[@fixed="184"]'):n.set('fixed','156')
save(D/'gui/app.xml',app)
# Existing pause / green play / fast / fastest artwork, one horizontal row.
sp=E.Element('Panel',width='200',height='156');el(sp,'FilledRectangle',color='#102333ee');layout=el(sp,'TableLayout',rows=4,cols=4)
for row,h in [(1,24),(2,40),(3,44),(4,44)]:el(layout,'rowsize',row=row,fixed=h)
el(cell(layout,1,1,colspan=4),'Paragraph',color='#bdd4c8',font_size=14,halign='center').text='GAME SPEED'
for i,(name,tip) in enumerate([('SpeedPauseButton','Pause'),('SpeedNormalButton','Play'),('SpeedFastButton','Fast forward'),('SpeedFastestButton','Fastest forward')],1):
 b=el(cell(layout,2,i,halign='center',valign='center'),'CheckButton',name=name,width=44,height=36)
 for tag,state in [('image','normal'),('image-hover','hover'),('image-checked','checked')]:img(b,tag,'images/evolution/speed-'+state+'.png',44,36)
 img(b,'image-caption','images/evolution/'+name+'.png',44,36);el(b,'tooltip').text=tip
for row,name,label in [(3,'CityAdvisorsButton','City Hall'),(4,'GameMenuButton','Menu / Save city')]:button(cell(layout,row,1,colspan=4,halign='center',valign='center'),name,label,192,34)
save(D/'gui/speedpanel.xml',sp)
# The custom generated city background and navigation in its shaded left area.
menu=E.parse(str(D/'gui/mainmenu.xml'));panel=menu.find('SwitchComponent/Panel');panel.find('Image').set('src','images/evolution/city-menu.png');middle=panel.find('TableLayout/cell/TableLayout');middle.find('colsize').set('col','1');c=middle.find('cell');c.set('col','1');buttons=c.find('TableLayout');c.remove(buttons);p=el(c,'Panel',resizable='yes');el(p,'FilledRectangle',color='#132c2bdf');p.append(buttons)
for n in buttons.findall('.//Paragraph'):
 n.set('color','#f2efdf')
 if n.text:n.text=n.text.replace('Preview 10','Preview 11')
save(D/'gui/mainmenu.xml',menu)
# A report-only transport window; view selection and refresh remain.
tr=E.parse(str(D/'gui/transport.xml'));tr.getroot().set('height','570');tr.getroot().set('width','800');tr.find('title/Paragraph').text='TRANSPORT REPORT';contents=tr.find('contents')
for n in list(contents):contents.remove(n)
table=el(contents,'TableLayout',rows=8,cols=2)
for row,h in [(1,46),(2,34),(3,34),(4,34),(5,34),(6,100),(7,48),(8,48)]:el(table,'rowsize',row=row,fixed=h)
items=[(1,'TransitSummary','Passenger services connect homes to jobs.')]+[(i+2,'TransitMode'+str(i),'Waiting for a connected service.') for i in range(4)]+[(6,'TransitIntro','Build from the categories on the left. Connect at least two supplied stops. Early bus routes use horse carriages. Motor traffic begins at 10% technology and reaches 100% at 30%.')]
for row,name,text in items:el(cell(table,row,1,colspan=2),'Paragraph',name=name,color='#edf0df',font_size=17).text=text
for row,col,span,name,label,w in [(7,1,2,'TransitView','Normal city view',720),(8,1,1,'TransitRefresh','Refresh service report',340),(8,2,1,'TransitClose','Close report',340)]:button(cell(table,row,col,colspan=span,halign='center',valign='center'),name,label,w,36,'images/readable/plate-650-',18)
save(D/'gui/transport.xml',tr)
(D/'help/en/church.xml').write_text('<?xml version="1.0"?><Document style="helpdocument"><Paragraph style="htitle">Georgian Orthodox church</Paragraph><Paragraph style="hp">In Civic. Costs $6,000 and occupies 3 by 3 tiles. Needs 2 labor and 1 goods/day, no electricity. Upkeep $8/day. An operating church provides +1 research/day, nearby housing morale +8 within eight Manhattan-distance tiles and up to +6 city target approval according to housing coverage. Benefits do not stack. Schools and universities are still required for formal skilled workers.</Paragraph></Document>')
for p in [G/'Play.cmd',G/'Diagnose-Startup.cmd']:p.write_text(p.read_text().replace('Preview 10','Preview 11').replace('Preview-10','Preview-11'),newline='\r\n')
print('Restored all requested UI, church and horse changes')
