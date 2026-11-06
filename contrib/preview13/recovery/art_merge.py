import json,shutil,xml.etree.ElementTree as E,hashlib
from pathlib import Path
from PIL import Image
s=Path('preview13-work/CityEvolution-Preview-13-Source');a=Path('preview13-work/art/CityEvolution-Complete-Art-03')
def edit(p,old,new):
 p=s/p;t=p.read_text();assert old in t,(p,old);p.write_text(t.replace(old,new))
b=json.loads((a/'integration/day-resource-bindings.json').read_text());p=s/'data/images/tiles/images.xml';t=p.read_text();root=E.fromstring(t);used=set();counts={}
for g in root.iter('group'):
 key=None;i=0
 for e in g:
  if e.tag=='resourceID':key=e.get('name');i=0
  if e.tag=='image' and key in b:
   assert i<len(b[key]); e.attrib=dict(b[key][i]);used.add(e.get('file'));i+=1;counts[key]=i
for k in counts:assert counts[k]==len(b[k]),k
p.write_text('<?xml version="1.0"?>\n'+E.tostring(root,encoding='unicode'))
for f in used:
 src=a/'assets/data/images/tiles'/f
 if src.exists():shutil.copy2(src,s/'data/images/tiles'/f)
for name in ['transit','help-art01','skyscrapers']:
 shutil.copytree(a/'assets/data/images'/name,s/'data/images'/name,dirs_exist_ok=True)
for p in (a/'assets/data/images/readable').glob('*-building.png'):shutil.copy2(p,s/'data/images/readable'/p.name)
for ent in json.loads((a/'integration/art01/help-image-replacements.json').read_text())['files']:
 p=s/'data'/ent['file']
 if p.exists():
  t=p.read_text()
  for pair in ent['replace']:t=t.replace(pair['old'],pair['new'])
  p.write_text(t)
shutil.copytree(a/'runtime/data/night',s/'data/night',dirs_exist_ok=True)
d=json.loads((s/'data/night/lighting.json').read_text());(s/'data/night/crops').mkdir(exist_ok=True)
for k,l in d['layers'].items():
 im=Image.open(s/'data'/l['atlas']);x,y,w,h=l['rect'];im.crop((x,y,x+w,y+h)).save(s/'data/night/crops'/f'{k}.png')
h=['// SPDX-License-Identifier: GPL-2.0-or-later','#pragma once','#include <string>','#include <vector>','#include <unordered_map>','struct NightCrop { const char* file; int x,y; bool electric,evening; };','inline const std::unordered_map<std::string,std::vector<NightCrop>> nightCrops={']
for k,o in d['objects'].items():
 for idx,f in enumerate(o['frames']):
  if k.startswith('Skyscraper:'):key=k.split(':',1)[1]+':'+str(idx)
  elif k.startswith('Vehicle:'):key='Traffic:'+k.split(':',1)[1]+':'+str(idx)
  elif k.startswith(('Bluecar:','HorseTraffic:')):key=k
  else:key=k+':'+str(idx)
  layers=[]
  for phase in ['late','evening']:
   l=f.get(phase)
   if l:
    x,y=l['offset_from_sprite_top_left'];layers.append('{"night/crops/'+l['layer']+'.png",'+str(x)+','+str(y)+','+str(o['power_policy']=='requires_electricity').lower()+','+str(phase=='evening').lower()+'}')
  if layers:h.append('{'+json.dumps(key)+', {'+','.join(layers)+'}},')
h.append('};');(s/'src/lincity-ng/NightCrops.hpp').write_text('\n'.join(h)+'\n')
edit(Path('src/lincity/resources.hpp'),'Texture* texture;','std::string nightKey;\n    Texture* texture;')
edit(Path('src/lincity-ng/GameView.cpp'),'graphicsInfo->image = readImage( key );','graphicsInfo->nightKey = resourceGroup->resourceID + ":" + std::to_string(resourceGroup->graphicsInfoVector.size()-1);\n        graphicsInfo->image = readImage( key );')
edit(Path('src/gui/Texture.hpp'),'virtual int getWidth() const = 0;','virtual void setOpacity(unsigned char a) {}\n\n  virtual int getWidth() const = 0;')
edit(Path('src/gui/PainterSDL/TextureSDL.hpp'),'virtual void setScaleMode(ScaleMode mode) override;','void setOpacity(unsigned char a) override { SDL_SetTextureAlphaMod(tx,a); }\n  virtual void setScaleMode(ScaleMode mode) override;')
edit(Path('src/lincity-ng/GameView.hpp'),'void drawStreetActors(', 'void drawNightLayers(Painter& painter, const std::string& key, const Rect2D& rect, bool occupied, bool powered);\n  void drawStreetActors(')
edit(Path('src/lincity-ng/GameView.cpp'),'#include "GameView.hpp"','#include "GameView.hpp"\n#include "NightCrops.hpp"')
p=s/'src/lincity-ng/GameView.cpp';t=p.read_text();t=t.replace('const int strength = powered ? 90 : 135;','const int strength = powered ? 110 : 148;');t=t.replace('graphicsInfo->texture->setLighting(255,255,255);','graphicsInfo->texture->setLighting(255,255,255);\n        bool occupied=true;\n        if(inCity(tile)) { auto* c=getWorld().map(tile)->reportingConstruction; if(c && GROUP_IS_RESIDENCE(c->constructionGroup->group)) occupied=static_cast<Residence*>(c)->local_population>0; }\n        drawNightLayers(painter,graphicsInfo->nightKey,tilerect,occupied,powered);',1)
start=t.index('      float stride=',t.index('void GameView::drawStreetActors'));end=t.index('\n    }\n  }\n}',start)
t=t[:start]+'''      const char* direction=(a.next.x>a.point.x||a.next.y<a.point.y)?"NE":"SW";
      auto texture=texture_manager->load(std::string("images/transit/pedestrian-")+direction+"-0"+std::to_string(int(a.progress*8)%4)+".png");
      texture->setLighting(255-e.darkness()*110/1000,255-e.darkness()*95/1000,255-e.darkness()*65/1000);
      painter.drawStretchTexture(texture,Rect2D(p.x-8*zoom,p.y-21*zoom,p.x+8*zoom,p.y+3*zoom));
      texture->setLighting(255,255,255);'''+t[end:]
t+='''
void GameView::drawNightLayers(Painter& painter,const std::string& key,const Rect2D& rect,bool occupied,bool powered) {
  if(!occupied) return;
  auto it=nightCrops.find(key); if(it==nightCrops.end()) return;
  const auto& e=getWorld().evolution; float amount=e.darkness()/1000.f;
  if(amount<=0) return;
  float late=0;
  if(e.skyPhase==2) late=std::clamp(float(e.skyElapsed)/std::max(1,e.skyDuration)*2.f-.6f,0.f,1.f);
  for(const auto& layer:it->second) {
    if(layer.electric&&!powered)continue;
    float alpha=amount*(layer.evening?1-late:1); if(alpha<=0)continue;
    auto tx=texture_manager->load(layer.file);tx->setOpacity(static_cast<unsigned char>(255*alpha));
    Vector2 p=rect.getPos()+Vector2(layer.x*zoom,layer.y*zoom);
    painter.drawStretchTexture(tx,Rect2D(p.x,p.y,p.x+tx->getWidth()*zoom,p.y+tx->getHeight()*zoom));tx->setOpacity(255);
  }
}
'''
p.write_text(t)
edit(Path('data/CMakeLists.txt'),'  fonts/*','  night/*.png\n  night/*.json\n  fonts/*')
edit(Path('src/lincity/modules/church.cpp'),'"Georgian Orthodox church","Georgian Orthodox churches"','"Church","Churches"')
p=s/'data/help/en/church.xml';t=p.read_text().replace('Georgian Orthodox Christian church','Church').replace('Georgian Orthodox church','Church').replace('Georgian Orthodox','Christian');p.write_text(t)
# Preserve the complete integration metadata for independent auditing.
shutil.copytree(a/'integration',s/'art/preview13/integration',dirs_exist_ok=True)
protected=['data/gui/app.xml','data/gui/buttonpanel.xml','data/gui/speedpanel.xml','data/gui/minimap.xml','data/gui/minimapswitchbuttons.xml','data/gui/cityhall.xml']
manifest={str(p.relative_to(s)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [s/x for x in protected] if p.exists()}
(s/'doc/audit/Preview12-protected-menu-hashes.json').write_text(json.dumps(manifest,indent=2))
print('Rebound',len(counts),'resources,',len(used),'frames; night objects',len(d['objects']))
