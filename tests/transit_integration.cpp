// SPDX-License-Identifier: GPL-2.0-or-later
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/world.hpp"
#include <iostream>
#include <stdexcept>
#include <filesystem>
static void require(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
int main(){try{
 initializeModules();
 for(int mode=0;mode<4;++mode){
  auto w=new_barren_city(50);w->total_money=10000000;w->tech_level=MAX_TECH_LEVEL;
  w->buildConstruction(residenceLLConstructionGroup,MapPoint(4,2));auto* h=static_cast<Residence*>(w->map(MapPoint(4,2))->reportingConstruction);h->local_population=80;
  w->buildConstruction(schoolConstructionGroup,MapPoint(30,2));
  TransitGroup* g=mode==0?&busStopGroup:mode==1?&railStationGroup:mode==2?&metroStationGroup:&elevatedStationGroup;
  w->buildConstruction(*g,MapPoint(5,7));w->buildConstruction(*g,MapPoint(30,7));
  auto* a=static_cast<TransitFacility*>(w->map(MapPoint(5,7))->reportingConstruction);auto* b=static_cast<TransitFacility*>(w->map(MapPoint(30,7))->reportingConstruction);
  for(auto* c:{a,b}){c->commodityCount[STUFF_LABOR]=1000;c->commodityCount[STUFF_LOVOLT]=1000;c->update();require(c->active,"supplied station did not open");c->busy=100;}
  w->transit.update(*w,false);require(w->transit.passengers[mode]==0,"disconnected stations served riders");
  int y=7+g->size;
  for(int x=5;x<=32;++x){MapPoint p(x,y);if(mode==0)w->buildConstruction(roadConstructionGroup,p);else if(mode==1)w->buildConstruction(railConstructionGroup,p);else require(w->transit.place(*w,p,mode==3),"layer track construction failed");}
  w->total_time=100;int money=w->total_money;w->transit.update(*w);
  require(w->transit.passengers[mode]>0,"connected service with homes and jobs has no riders");require(w->transit.covered(h->point,w->map.len()),"home not covered");require(!w->transit.routes.empty(),"no vehicle route traced");
  int expected=w->transit.faresMonth-int(w->transit.tunnels.size()+w->transit.viaducts.size())*2;require(w->total_money-money==expected,"fare / track upkeep accounting mismatch");money=w->total_money;w->transit.update(*w);require(w->total_money==money,"fares collected twice for same month");
  w->bulldozeArea(MapPoint(30,2));w->transit.update(*w,false);require(w->transit.passengers[mode]==0,"roads incorrectly counted as workplaces");w->buildConstruction(schoolConstructionGroup,MapPoint(30,2));w->transit.update(*w,false);
  int full=w->transit.passengers[mode];a->busy=b->busy=50;w->transit.update(*w,false);require(w->transit.passengers[mode]==full/2,"partial service availability not reflected in riders");a->busy=b->busy=100;w->transit.update(*w,false);
  auto file=std::filesystem::path("transit-test-"+std::to_string(mode)+".scn.gz");w->save(file);auto copy=World::load(file);std::filesystem::remove(file);
  require(copy->stats.groupCount[g->group]==2,"stations not saved");require(copy->transit.tunnels==w->transit.tunnels&&copy->transit.viaducts==w->transit.viaducts,"track layer not saved");require(copy->stats.income.transit_fares.acc==w->stats.income.transit_fares.acc,"fare ledger not saved");
  if(mode<2)w->bulldozeArea(MapPoint(18,y));else require(w->transit.remove(*w,MapPoint(18,y),mode==3),"track layer removal failed");
  w->transit.update(*w,false);require(w->transit.passengers[mode]==0&&!w->transit.covered(h->point,w->map.len()),"cut route still serves passengers");
  b->commodityCount[STUFF_LABOR]=0;b->update();require(!b->active,"unstaffed station still open");
  w->bulldozeArea(a->point);w->transit.update(*w,false);require(w->transit.disconnected[mode]==1,"demolished station remains in network");
  std::cout<<"PASS transit mode "<<mode<<": network, ridership, coverage, fares, costs, save/load, route cut, staffing\n";
 }
 {
  auto city=new_barren_city(50);city->total_money=10000000;city->tech_level=MAX_TECH_LEVEL;
  city->buildConstruction(residenceLLConstructionGroup,MapPoint(4,2));static_cast<Residence*>(city->map(MapPoint(4,2))->reportingConstruction)->local_population=80;
  city->buildConstruction(schoolConstructionGroup,MapPoint(40,2));
  for(auto p:{MapPoint(5,7),MapPoint(20,7)})city->buildConstruction(metroStationGroup,p);
  for(auto p:{MapPoint(22,7),MapPoint(40,7)})city->buildConstruction(elevatedStationGroup,p);
  for(auto* c:city->map.constructions)if(auto* f=dynamic_cast<TransitFacility*>(c)){f->commodityCount[STUFF_LABOR]=1000;f->commodityCount[STUFF_LOVOLT]=1000;f->update();f->busy=100;}
  for(int x=5;x<=20;++x)city->transit.place(*city,MapPoint(x,8),false);
  for(int x=22;x<=41;++x)city->transit.place(*city,MapPoint(x,9),true);
  city->transit.update(*city,false);require(city->transit.passengers[2]>0&&city->transit.passengers[3]>0,"metro/elevated interchange did not carry connecting riders");require(city->transit.routes.size()==2,"interchange did not create both vehicle legs");
  city->transit.remove(*city,MapPoint(12,8),false);city->transit.update(*city,false);require(city->transit.passengers[2]==0&&city->transit.passengers[3]==0,"broken interchange route still carries connecting riders");
  std::cout<<"PASS one-transfer underground/elevated interchange and disruption\n";
 }
 {
  auto city = new_barren_city(60);
  city->total_money = 500000;
  city->tech_level = MAX_TECH_LEVEL;
  city->buildConstruction(residenceLLConstructionGroup,{4,2});
  city->buildConstruction(schoolConstructionGroup,{30,2});
  city->buildConstruction(marketConstructionGroup,{34,5});
  city->buildConstruction(busTerminalGroup,{5,7});
  city->buildConstruction(busTerminalGroup,{31,7});
  for(int x=5;x<=32;++x) city->buildConstruction(trackConstructionGroup,{x,9});
  for(auto p:{MapPoint(5,7),MapPoint(31,7)}) {
   auto* stop = static_cast<TransitFacility*>(city->map(p)->construction);
   stop->busy=100;
   stop->active=true;
  }
  auto* home = static_cast<Residence*>(city->map({4,2})->construction);
  auto* work = city->map({30,2})->construction;
  auto* market = city->map({34,5})->construction;
  home->local_population=100;
  home->commodityCount[STUFF_LABOR]=500;
  market->commodityCount[STUFF_LABOR]=500;
  market->commodityCount[STUFF_FOOD]=1000;
  city->transit.update(*city,false,true);
  require(city->transit.roadTripsAt({18,9},city->map.len())>0,"bus route lacks road load");
  for(int day=1;day<=100;++day) {
   city->total_time=day;
   city->transit.deliver(*city);
  }
  require(work->commodityCount[STUFF_LABOR]>0 &&
    home->commodityCount[STUFF_LABOR]+work->commodityCount[STUFF_LABOR]==500,
    "commuting did not transfer actual labor");
  require(home->commodityCount[STUFF_FOOD]>0 &&
    market->commodityCount[STUFF_FOOD]+home->commodityCount[STUFF_FOOD]==1000,
    "shopping did not transfer actual market stock");
  city->bulldozeArea({18,9});
  const int food=home->commodityCount[STUFF_FOOD];
  for(int day=1;day<=100;++day) {
   city->total_time=day;
   city->transit.deliver(*city);
  }
  require(home->commodityCount[STUFF_FOOD]==food,"shopping stock crossed a cut route");
  std::cout<<"PASS direct commuting and shopping transfer conserved stock and respect road breaks\n";
 }
 auto w=new_barren_city(50);w->tech_level=MAX_TECH_LEVEL;w->total_money=1000000;w->buildConstruction(residenceLLConstructionGroup,MapPoint(10,10));auto* home=w->map(MapPoint(10,10))->reportingConstruction;
 require(w->transit.place(*w,MapPoint(11,11),false),"cannot tunnel below existing home");require(w->transit.place(*w,MapPoint(11,11),true),"cannot elevate above existing home");require(w->map(MapPoint(10,10))->reportingConstruction==home,"layer construction removed home");int cash=w->total_money;require(!w->transit.place(*w,MapPoint(11,11),false)&&w->total_money==cash,"duplicate tunnel charged money");
 require(w->transit.remove(*w,MapPoint(11,11),false)&&w->map(MapPoint(10,10))->reportingConstruction==home,"removing tunnel damaged surface home");
 std::cout<<"PASS independent layers preserve buildings and reject duplicates\n";
 }catch(const Message::Exception&e){std::cerr<<e.getMessage()->str()<<'\n';return 1;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
