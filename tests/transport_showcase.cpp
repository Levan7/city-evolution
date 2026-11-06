// SPDX-License-Identifier: GPL-2.0-or-later
// Generate a stocked transport sandbox; not a self-sustaining example city.
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/world.hpp"
#include <filesystem>
#include <iostream>
int main(int argc,char**argv){try{
 initializeModules();
 if(argc>2){for(auto& e:std::filesystem::directory_iterator(argv[2]))if(e.path().extension()==".gz" && e.path().stem().extension()==".scn"){
  auto w=World::load(e.path());auto n=w->map.constructions.size();w->save(argv[1]);auto copy=World::load(argv[1]);if(copy->map.constructions.size()!=n)return 2;for(int i=0;i<10;++i)copy->do_time_step();std::cout<<"PASS legacy load/save/simulate "<<e.path().filename()<<'\n';
 }return 0;}
 auto w=new_barren_city(100);w->total_money=100000000;w->tech_level=MAX_TECH_LEVEL;w->given_scene="City Evolution transport sandbox";w->evolution.cityName="Green Junction";
 auto road=[&](MapPoint p){if(!w->map(p)->reportingConstruction)w->buildConstruction(roadConstructionGroup,p);};
 TransitGroup* groups[]={&busTerminalGroup,&railStationGroup,&metroStationGroup,&elevatedStationGroup};
 for(int mode=0;mode<4;++mode){int y=12+mode*20;
  w->buildConstruction(residenceLLConstructionGroup,MapPoint(6,y-5));auto* h=static_cast<Residence*>(w->map(MapPoint(6,y-5))->reportingConstruction);h->local_population=100;
  w->buildConstruction(schoolConstructionGroup,MapPoint(40,y-5));w->buildConstruction(marketConstructionGroup,MapPoint(12,y-5));
  auto* g=groups[mode];w->buildConstruction(*g,MapPoint(7,y));w->buildConstruction(*g,MapPoint(40,y));
  for(int x=6;x<=43;++x)road(MapPoint(x,y-2));
  for(int yy=y-4;yy<=y+2;++yy){road(MapPoint(6,yy));road(MapPoint(39,yy));}
  int line=y+g->size;
  for(int x=7;x<=43;++x){MapPoint p(x,line);if(mode==0)road(p);else if(mode==1){if(!w->map(p)->reportingConstruction)w->buildConstruction(railConstructionGroup,p);}else w->transit.place(*w,p,mode==3);}
 }
 w->buildConstruction(universityConstructionGroup,MapPoint(64,20));
 for(int i=0;i<4;++i){w->buildConstruction(residenceLLConstructionGroup,MapPoint(80,7+10*i));static_cast<Residence*>(w->map(MapPoint(80,7+10*i))->reportingConstruction)->local_population=80;}
 // Existing new businesses and a wider set of replacement models, separated from the lines.
 struct Model{ConstructionGroup* g;MapPoint p;};
 Model models[]={{&warehouseConstructionGroup,{53,12}},{&techCampusConstructionGroup,{58,12}},{&cafeConstructionGroup,{64,12}},{&foodHallConstructionGroup,{68,12}},{&healthCentreConstructionGroup,{53,20}},{&fireStationConstructionGroup,{58,20}},{&organic_farmConstructionGroup,{53,29}},{&solarPowerConstructionGroup,{62,29}},{&substationConstructionGroup,{68,29}},{&recycleConstructionGroup,{74,29}},{&industryLightConstructionGroup,{53,40}},{&industryHeavyConstructionGroup,{60,40}}};
 for(auto& m:models)w->buildConstruction(*m.g,m.p);
 for(int x=50;x<=78;++x){road(MapPoint(x,17));road(MapPoint(x,25));road(MapPoint(x,35));road(MapPoint(x,46));}
 for(int y=12;y<=46;++y)road(MapPoint(50,y));
 for(auto* c:w->map.constructions){c->bootstrap_commodities(100);if(auto* f=dynamic_cast<TransitFacility*>(c)){f->update();f->busy=100;}}
 w->total_money=5000000;w->total_time=100;w->transit.update(*w,false);for(int mode=0;mode<4;++mode)if(w->transit.passengers[mode]<=0){std::cerr<<"Empty demo line "<<mode<<'\n';return 3;}
 w->map.recentPoint=MapPoint(28,28);w->evolution.headline("Stocked transport sandbox. Open Transport at the top right. Supplies are temporary; this is not a balanced city.");w->save(argv[1]);auto copy=World::load(argv[1]);std::cout<<"PASS stocked transport sandbox save/load: "<<copy->map.constructions.size()<<" constructions\n";
 }catch(const Message::Exception&e){std::cerr<<e.getMessage()->str()<<'\n';return 1;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
