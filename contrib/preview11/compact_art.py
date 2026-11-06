from pathlib import Path
from lxml import etree as E
from PIL import Image,ImageDraw,ImageFont
import json,math,copy
W=Path(__file__).resolve().parent;S=W/'CityEvolution-Preview-11-Source';D=S/'data';T=D/'images/tiles';A=S/'art/preview11'
tree=E.parse(str(T/'images.xml'));root=tree.getroot()
def resources():
 d={}
 for g in root.findall('group'):
  key=None
  for n in g:
   if n.tag=='resourceID':key=n.get('name');d[key]=[]
   elif n.tag=='image':d[key].append(n)
 return d
res=resources();manifest=json.loads((S/'art/manifest.json').read_text());native=json.loads((S/'art/preview10/native-frame-layout.json').read_text())
original={k:[dict(n.attrib) for n in v] for k,v in res.items()}
(A/'before-compact-layout.json').write_text(json.dumps(original,indent=2))
def atlas(file,cols,rows):
 im=Image.open(A/file).convert('RGBA');im.putalpha(im.getchannel('A').point(lambda a:a if a>=220 else 0))
 return [im.crop((round(c*im.width/cols),round(r*im.height/rows),round((c+1)*im.width/cols),round((r+1)*im.height/rows))) for r in range(rows) for c in range(cols)]
def group_model(rid,name,frames,fp,slots=None,cycle=True):
 # Use one union canvas and bottom pivot for a whole animation; avoid frame jitter.
 bbs=[f.getchannel('A').getbbox() for f in frames];assert all(bbs),(rid,bbs)
 u=(min(b[0] for b in bbs),min(b[1] for b in bbs),max(b[2] for b in bbs),max(b[3] for b in bbs))
 w=128*fp;factor=(w-8)/(u[2]-u[0]);h=math.ceil((u[3]-u[1])*factor)+8
 files=[]
 for i,f in enumerate(frames):
  o=Image.new('RGBA',(w,h));o.alpha_composite(f.crop(u).resize((w-8,h-8),Image.Resampling.LANCZOS),(4,4));fn=f'revamp11-compact-{name}-{i}.png';o.save(T/fn);files.append(fn)
 if rid not in res:
  g=E.SubElement(root,'group');E.SubElement(g,'resourceID',name=rid);res[rid]=[E.SubElement(g,'image') for _ in range(slots or 1)]
 nodes=res[rid];anchor=h-4
 for i,n in enumerate(nodes):
  if len(frames)>1 and cycle:
   phase=i*2*(len(frames)-1)/max(1,len(nodes)-1);j=round(phase if phase<=len(frames)-1 else 2*(len(frames)-1)-phase)
  else:j=min(i,len(frames)-1)
  n.attrib.clear();n.attrib.update({'file':files[j],'x':str(w//2),'y':str(anchor)})
 manifest['resource_bindings'][rid]=name
 manifest['assets'][name]={'file':'images/tiles/'+files[0],'size':[w,h],'anchor':[w//2,anchor],'footprint':fp}
forest=atlas('compact-commune.png',3,3)
for row,rid in enumerate(['Commune','CommuneVariant1','CommuneVariant2']):group_model(rid,'forestry-'+str(row),forest[row*3:row*3+3],4,slots=len(native['Commune']))
homes=atlas('compact-housing.png',2,6)
for row,rid in enumerate(['ResidentialLowLow','ResidentialMedLow','ResidentialHighLow','ResidentialLow','ResidentialMedHigh','ResidentialHighHigh']):
 group_model(rid,'housing-compact-'+str(row),[homes[row*2]],2,cycle=False)
 group_model(rid+'Variant1','housing-compact-'+str(row)+'-alt',[homes[row*2+1]],2,slots=len(native[rid]),cycle=False)
shops=atlas('compact-workshops.png',3,4)
for row,rid in enumerate(['Pottery','Blacksmith','Mill','MarketEmpty']):
 group_model(rid,'workshop-compact-'+str(row),shops[row*3:row*3+3],2)
# All market inventory states use the same compact market (native state slot counts unchanged).
for rid in ['MarketLow','MarketMed','MarketFull']:
 group_model(rid,'workshop-compact-market',shops[9:12],2)
farm=atlas('compact-farm.png',3,2);group_model('Farm','farm-fields',farm[:5],4,cycle=False)
# Lower every replacement building to 80% sprite scale, retaining an unscaled native grass lot.
# This is asset preparation: hitboxes, costs, simulation size and road connectivity stay unchanged.
grass=Image.open(T/'green.png').convert('RGBA')
grounds={}
def ground(fp):
 if fp in grounds:return grounds[fp]
 w=128*fp;h=64*fp+8;g=Image.new('RGBA',(w,h))
 for j in range(fp):
  for i in range(fp):
   x=(i-j)*64+w//2-64;y=(i+j)*32+64-70+4
   g.alpha_composite(grass,(x,y))
 grounds[fp]=g;return g
cache={};metrics=[]
for rid,model in manifest['resource_bindings'].items():
 if rid not in res:continue
 fp=manifest['assets'].get(model,{}).get('footprint')
 if not fp:continue
 for n in res[rid]:
  f=n.get('file');im=Image.open(T/f).convert('RGBA')
  if im.size==(1,1):continue
  key=(f,fp,int(n.get('x')),int(n.get('y')))
  if key not in cache:
   old=im.size;scale=.80;ox=int(n.get('x'));oy=int(n.get('y'));nw=max(1,round(im.width*scale));nh=max(1,round(im.height*scale))
   pic=im.resize((nw,nh),Image.Resampling.LANCZOS);g=ground(fp);anchor=max(g.height-4,round(oy*scale))+4
   h=max(anchor+4,anchor-round(oy*scale)+nh)
   out=Image.new('RGBA',(128*fp,h));out.alpha_composite(g,(0,anchor-(g.height-4)))
   out.alpha_composite(pic,(round(64*fp-ox*scale),round(anchor-oy*scale)))
   fn='lower-'+f;out.save(T/fn);cache[key]=(fn,64*fp,anchor,list(out.size))
   metrics.append({'input':f,'output':fn,'original_canvas':list(old),'output_canvas':list(out.size),'sprite_scale':scale,'footprint':fp})
  fn,x,y,size=cache[key];n.attrib.update({'file':fn,'x':str(x),'y':str(y)})
 # Keep manifest's representative image tied to the active exported frame.
 if res[rid]:
  n=res[rid][0];manifest['assets'][model].update(file='images/tiles/'+n.get('file'),size=list(Image.open(T/n.get('file')).size),anchor=[int(n.get('x')),int(n.get('y'))])
# Smoke shares the exact scaling of its parent factory and emitter motion.
for rid in ['GraySmoke','BlackSmoke']:
 for n in res[rid]:
  f=n.get('file');im=Image.open(T/f).convert('RGBA');o=im.resize((max(1,round(im.width*.8)),max(1,round(im.height*.8))),Image.Resampling.LANCZOS);fn='lower-'+f;o.save(T/fn);n.set('file',fn);n.set('x',str(round(int(n.get('x'))*.8)));n.set('y',str(round(int(n.get('y'))*.8)))
(T/'images.xml').write_bytes(E.tostring(tree,encoding='UTF-8',xml_declaration=True,pretty_print=True))
manifest['note']='Preview 11: original-style low-rise forestry, farms and workshops; twelve housing appearances; three location-stable commune variants. Replacement building sprites use 80% scale over original-sized native grass lots. Native animation frame counts retained. Traffic motorises from 10% to 30% tech. Terrain/bridges remain upstream.'
(S/'art/manifest.json').write_text(json.dumps(manifest,indent=2));(A/'scale-metrics.json').write_text(json.dumps(metrics,indent=2))
(A/'compact-resource-layout.json').write_text(json.dumps({k:[dict(n.attrib) for n in v] for k,v in res.items()},indent=2))
# Tool pictures use the current in-game sprite, not the obsolete larger versions.
thumbs=json.loads((S/'art/preview10/thumbnail-bindings.json').read_text());thumbs['Church']='GeorgianChurch'
labels={'Church':'Orthodox church','Residence1':'Basic low','Residence2':'Basic medium','Residence3':'Basic high','Residence4':'Modern low','Residence5':'Modern medium','Residence6':'Modern high','CoalPS':'Coal plant','SolarPS':'Solar plant','LIndustry':'Light industry','HIndustry':'Heavy industry','Tip':'Landfill','Coal':'Coal mine','Ore':'Ore mine','Windmill':'Wind turbine'}
font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',11)
import re
for key,rid in thumbs.items():
 pic=Image.open(T/res[rid][0].get('file')).convert('RGBA');bb=pic.getchannel('A').point(lambda a:255 if a>32 else 0).getbbox()
 if bb:pic=pic.crop(bb)
 pic.thumbnail((80,50),Image.Resampling.LANCZOS);out=Image.new('RGBA',(88,76));out.alpha_composite(pic,((88-pic.width)//2,2+(50-pic.height)//2));dr=ImageDraw.Draw(out)
 words=labels.get(key,re.sub(r'(?<=[a-z])(?=[A-Z])',' ',key)).split();lines=['']
 for word in words:
  text=(lines[-1]+' '+word).strip()
  if dr.textlength(text,font=font)>84 and lines[-1]:lines.append(word)
  else:lines[-1]=text
 for i,line in enumerate(lines[:2]):dr.text((44,58+12*i),line,font=font,fill='#f2efdf',anchor='mm')
 out.save(D/'images/readable'/('BPM'+key+'Button-building.png'))
# Review current representative sprites and variants.
reviewids=['Commune','CommuneVariant1','CommuneVariant2','Farm','Pottery','Blacksmith','Mill','MarketEmpty','ResidentialLowLow','ResidentialLowLowVariant1','ResidentialMedLow','ResidentialHighLow','ResidentialLow','ResidentialMedHigh','ResidentialMedHighVariant1','ResidentialHighHigh','GeorgianChurch']
review=Image.new('RGB',(1200,math.ceil(len(reviewids)/4)*220),'#d2dcca');dr=ImageDraw.Draw(review)
for i,rid in enumerate(reviewids):
 pic=Image.open(T/res[rid][0].get('file')).convert('RGBA');pic.thumbnail((280,185),Image.Resampling.LANCZOS);x=i%4*300+(300-pic.width)//2;y=i//4*220;review.paste(pic,(x,y),pic);dr.text((i%4*300+8,y+194),rid,font=font,fill='#173a31')
review.save(A/'compact-review.jpg')
print('Compact art exported:',len(metrics),'building frames; 3 commune variants, 12 housing appearances')
