from pathlib import Path
from PIL import Image
import shutil,json
s=Path('preview14-work/CityEvolution-Preview-14-Source');(s/'art/preview14').mkdir(parents=True,exist_ok=True)
p=Path('generated_images/exec-99b17c2f-b4a3-4b1c-aaa7-454fff81d769.png');im=Image.open(p);shutil.copy2(p,s/'art/preview14/woman-walk-atlas.png')
for row,direction in enumerate(['SE','SW','NW','NE']):
 for col in range(4):
  cell=im.crop((round(col*im.width/4),round(row*im.height/4),round((col+1)*im.width/4),round((row+1)*im.height/4)));cell=cell.crop(cell.getbbox());height=18;width=max(1,round(cell.width*height/cell.height));cell=cell.resize((width,height),Image.Resampling.LANCZOS);canvas=Image.new('RGBA',(16,24));canvas.alpha_composite(cell,((16-width)//2,21-height));canvas.save(s/'data/images/transit'/f'woman-{direction}-0{col}.png')
p=Path('generated_images/exec-cdd225e2-1489-44d3-865f-9ef99bc104b7.png');im=Image.open(p);shutil.copy2(p,s/'art/preview14/inventory-atlas.png');(s/'data/images/activity').mkdir(exist_ok=True)
# One constant scale per column: stock increases by quantity, rather than scaling each pile to the same size.
for col,kind in enumerate(['food','goods','ore','coal']):
 cells=[]
 for row in range(3):
  c=im.crop((round(col*im.width/4),round(row*im.height/3),round((col+1)*im.width/4),round((row+1)*im.height/3)));cells.append(c.crop(c.getbbox()))
 factor=46/max(c.width for c in cells)
 for row,cell in enumerate(cells):
  cell=cell.resize((max(1,round(cell.width*factor)),max(1,round(cell.height*factor))),Image.Resampling.LANCZOS);canvas=Image.new('RGBA',(50,50));canvas.alpha_composite(cell,((50-cell.width)//2,47-cell.height));canvas.save(s/'data/images/activity'/f'{kind}-{row+1}.png')
p=s/'src/lincity-ng/GameView.cpp';t=p.read_text().replace('#include "NightCrops.hpp"','#include "NightCrops.hpp"\n#include "lincity/building_visual_state.hpp"\n#include "lincity/modules/health_centre.hpp"\n#include "lincity/modules/firestation.hpp"');a='''      const char* direction=(a.next.x>a.point.x||a.next.y<a.point.y)?"NE":"SW";
      auto texture=texture_manager->load(std::string("images/transit/pedestrian-")+direction+"-0"+std::to_string(int(a.progress*8)%4)+".png");''';b='''      const bool woman=(a.appearance%2)==0;
      const int dx=a.next.x-a.point.x,dy=a.next.y-a.point.y;
      const char* direction=woman?(dx>0?"SE":dy>0?"SW":dx<0?"NW":"NE"):(dx>0||dy<0?"NE":"SW");
      auto texture=texture_manager->load(std::string("images/transit/")+(woman?"woman-":"pedestrian-")+direction+"-0"+std::to_string(int(a.progress*8)%4)+".png");''';assert a in t;t=t.replace(a,b)
a='        // Static details follow their building visibility.';b='''        if(!hideHigh && mapOverlay!=overlayOnly) {
          auto* parent=getWorld().map(upperLeft)->reportingConstruction;
          if(parent){auto stock=buildingStockVisual(*parent);if(stock.kind>=0&&stock.level>0){const char* kinds[]={"food","goods","ore","coal"};auto tx=texture_manager->load(std::string("images/activity/")+kinds[stock.kind]+"-"+std::to_string(stock.level)+".png");auto p=getScreenPoint(lowerRightTile)-Vector2(tileWidth*.60f,tileHeight*.48f);int dark=getWorld().evolution.darkness();tx->setLighting(255-dark*148/1000,255-dark*136/1000,255-dark*113/1000);painter.drawStretchTexture(tx,Rect2D(p.x-25*zoom,p.y-47*zoom,p.x+25*zoom,p.y+3*zoom));tx->setLighting(255,255,255);}}
        }
        // Static details follow their building visibility.''';assert a in t;t=t.replace(a,b)
a='if(c && GROUP_IS_RESIDENCE(c->constructionGroup->group)) occupied=static_cast<Residence*>(c)->local_population>0;';b='''if(c){int group=c->constructionGroup->group;if(GROUP_IS_RESIDENCE(group))occupied=static_cast<Residence*>(c)->local_population>0;else if(group==GROUP_HEALTH)occupied=static_cast<HealthCentre*>(c)->active;else if(group==GROUP_FIRESTATION)occupied=static_cast<FireStation*>(c)->active;else if(group==GROUP_POLICE)occupied=getWorld().civic.stationReady(*c,CivicServices::POLICE);}''';assert a in t;t=t.replace(a,b);p.write_text(t)
(s/'art/preview14/asset-notes.json').write_text(json.dumps({'woman':'Generated 4 directions × 4 walking phases, exported to 16×24 RGBA with 18px visible figure; mixed with existing men by stable actor appearance seed.','inventory':'Generated food/goods/ore/coal sprites at low/medium/full stock, one constant export scale per material, 50×50 canvas; selected from actual building inventory or native market ratio.','existing_states':'Original module state transitions and animation slot counts remain unchanged. Existing farm, workshop, mine and windmill variations remain installed. New stock overlays distinguish matching states without replacing whole buildings.'},indent=2))
