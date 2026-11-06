from pathlib import Path
from PIL import Image
s=Path('preview13-work/CityEvolution-Preview-13-Source')
def edit(f,a,b):
 p=s/f;t=p.read_text();assert a in t,(f,a);p.write_text(t.replace(a,b))
edit('src/lincity-ng/GameView.hpp','void drawNightLayers(','void drawEmergencyActors(Painter& painter,MapPoint tile);\n  void drawNightLayers(')
edit('src/lincity-ng/GameView.cpp','        // Static details follow their building visibility.','        if(mapOverlay != overlayOnly) drawEmergencyActors(painter,tile);\n        // Static details follow their building visibility.')
p=s/'src/lincity-ng/GameView.cpp';t=p.read_text()
# Fires keep their original emissive brightness at night.
t=t.replace('const int dark = (e.darkness()/25)*25;','const int dark = (graphicsInfo->nightKey.rfind("Fire:",0)==0 || graphicsInfo->nightKey.rfind("FireWasteLand:",0)==0)?0:(e.darkness()/25)*25;')
a='painter.drawStretchTexture(texture,Rect2D(p.x-width/2,p.y-height+4*zoom,p.x+width/2,p.y+4*zoom));'
b='''Rect2D actorRect(p.x-width/2,p.y-height+4*zoom,p.x+width/2,p.y+4*zoom);
      texture->setLighting(255-e.darkness()*110/1000,255-e.darkness()*95/1000,255-e.darkness()*65/1000);
      painter.drawStretchTexture(texture,actorRect);texture->setLighting(255,255,255);
      drawNightLayers(painter,std::string("Traffic:")+kind+direction+":0",actorRect,true,true);'''
assert a in t;t=t.replace(a,b)
a='painter.drawStretchTexture(texture,Rect2D(pos.x-width/2,pos.y-height+4*zoom,pos.x+width/2,pos.y+4*zoom));'
b='''Rect2D actorRect(pos.x-width/2,pos.y-height+4*zoom,pos.x+width/2,pos.y+4*zoom);
    int darkness=w.evolution.darkness();texture->setLighting(255-darkness*110/1000,255-darkness*95/1000,255-darkness*65/1000);painter.drawStretchTexture(texture,actorRect);texture->setLighting(255,255,255);
    drawNightLayers(painter,std::string("Traffic:")+(route.mode==0?(motor?"bus-":"carriage-"):"train-")+directions[dir]+":0",actorRect,true,true);'''
assert a in t;t=t.replace(a,b)
t+='''
void GameView::drawEmergencyActors(Painter& painter,MapPoint tile) {
 auto& w=getWorld();int n=w.map.len();auto point=[n](int id){return MapPoint(id%n,id/n);};
 for(const auto& u:w.civic.units){if(u.path.empty()||u.step>=int(u.path.size()))continue;int id=u.path[u.step];if(point(id)!=tile)continue;int next=u.step+1<int(u.path.size())?u.path[u.step+1]:id;auto a=point(id),b=point(next);int cost=w.civic.terrainCost(w,next,u.home,u.returning?u.home:next);float f=next==id?0:std::clamp(u.credit/float(std::max(1,cost)),0.f,1.f);auto p=getScreenPoint(a)*(1-f)+getScreenPoint(b)*f-Vector2(0,tileHeight*.52f);
  int dx=b.x-a.x,dy=b.y-a.y;const char* direction=dx>0?"SE":dy>0?"SW":dx<0?"NW":"NE";bool motor=TrafficEra::motorVehicle(w.tech_level,MAX_TECH_LEVEL,unsigned(u.home));std::string kind=motor?(u.kind==CivicServices::FIRE_CALL?"firetruck-":u.kind==CivicServices::MEDICAL_CALL?"ambulance-":"policecar-"):u.kind==CivicServices::FIRE_CALL?"pumpwagon-":"carriage-";
  std::string file=std::string("images/")+(!motor&&u.kind!=CivicServices::FIRE_CALL?"transit/":"emergency/")+kind+direction+".png";auto tx=texture_manager->load(file);float width=tx->getWidth()*zoom,height=tx->getHeight()*zoom;int dark=w.evolution.darkness();tx->setLighting(255-dark*110/1000,255-dark*95/1000,255-dark*65/1000);painter.drawStretchTexture(tx,Rect2D(p.x-width/2,p.y-height,p.x+width/2,p.y));tx->setLighting(255,255,255);
  if(motor&&w.evolution.visualTime%800<400){painter.setFillColor(Color(120,195,255));painter.fillRectangle(Rect2D(p.x-2*zoom,p.y-height*.8f,p.x+2*zoom,p.y-height*.8f+2*zoom));}
  if(dark>0){painter.setFillColor(Color(255,215,145));painter.fillRectangle(Rect2D(p.x+width*.25f,p.y-5*zoom,p.x+width*.25f+2*zoom,p.y-3*zoom));}
  if(u.kind==CivicServices::FIRE_CALL&&!u.returning&&next==id){painter.setLineColor(Color(161,211,237));painter.drawLine(p,p+Vector2(12*zoom,-16*zoom));}
 }
 auto* building=w.map(tile)->reportingConstruction;
 if(!building)return;
 int root=building->point.y*n+building->point.x;
 for(auto& c:w.civic.incidents){if(c.target!=root)continue;auto p=getScreenPoint(tile.e(building->constructionGroup->size-1))-Vector2(0,tileHeight);
  if(c.kind==CivicServices::FIRE_CALL&&building->constructionGroup->group!=GROUP_FIRE){auto* rg=ResourceGroup::resMap["Fire"];if(rg&&!rg->graphicsInfoVector.empty())drawTexture(painter,tile.e(building->constructionGroup->size-1),&rg->graphicsInfoVector[(w.evolution.visualTime/100)%rg->graphicsInfoVector.size()]);}
  if(c.kind==CivicServices::ZOMBIE_CALL){painter.setFillColor(Color(133,180,102));for(int i=0;i<std::min(4,c.severity);++i)painter.fillRectangle(Rect2D(p.x+i*4*zoom,p.y-10*zoom,p.x+(i*4+3)*zoom,p.y));}
 }
}
'''
p.write_text(t)
# Four tiny warm window overlays for the new station; no duplicate full night model.
lamp=Image.open(s/'data/night/crops/0a4f984d50a523dc.png').convert('RGBA').resize((4,6));canvas=Image.new('RGBA',(116,23))
for x,y in [(0,4),(17,12),(48,9),(109,0)]:canvas.alpha_composite(lamp,(x,y))
canvas.save(s/'data/night/crops/police-windows.png')
p=s/'src/lincity-ng/NightCrops.hpp';t=p.read_text();pos=t.rfind('};');t=t[:pos]+'{"PoliceStation:0", {{"night/crops/police-windows.png",76,90,false,false}}},\n'+t[pos:];p.write_text(t)
