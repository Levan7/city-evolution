// SPDX-License-Identifier: GPL-2.0-or-later
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/road_power.hpp"
#include "lincity/world.hpp"
#include <iostream>
#include <stdexcept>
static void need(bool b,const char*m){if(!b)throw std::runtime_error(m);}
static int electricity(int end,bool cut){auto w=new_barren_city(140);w->total_money=10000000;w->tech_level=MAX_TECH_LEVEL;w->buildConstruction(windmillConstructionGroup,{3,10});w->buildConstruction(residenceLLConstructionGroup,{end,10});for(int x=3;x<end+3;++x)w->buildConstruction(trackConstructionGroup,{x,9});auto* source=w->map({3,10})->reportingConstruction;auto* h=static_cast<Residence*>(w->map({end,10})->reportingConstruction);h->local_population=100;source->commodityCount[STUFF_LOVOLT]=100;if(cut)w->bulldozeArea({8,9});distributeRoadPower(*w);int got=h->commodityCount[STUFF_LOVOLT];need(source->commodityCount[STUFF_LOVOLT]>=0&&got+source->commodityCount[STUFF_LOVOLT]<=100,"power created or negative source stock");return got;}
static int passengers(int end,bool river){auto w=new_barren_city(140);w->total_money=10000000;w->tech_level=MAX_TECH_LEVEL;w->buildConstruction(residenceLLConstructionGroup,{3,2});static_cast<Residence*>(w->map({3,2})->reportingConstruction)->local_population=80;w->buildConstruction(schoolConstructionGroup,{end,2});w->buildConstruction(busStopGroup,{5,7});w->buildConstruction(busStopGroup,{end,7});for(int x=5;x<=end;++x)w->buildConstruction(trackConstructionGroup,{x,8});for(auto p:{MapPoint(5,7),MapPoint(end,7)}){auto*f=static_cast<TransitFacility*>(w->map(p)->reportingConstruction);f->active=true;f->busy=100;}if(river)for(int x=1;x<139;++x)w->map({x,5})->setTerrain(GROUP_WATER);w->transit.update(*w,false);need(w->transit.connected[0]==2,"distance removed physical route connection");return w->transit.passengers[0];}
static void negativeTreasury() {
  auto w = new_barren_city(50);
  w->total_money = -15000000;
  w->tech_level = 0;
  int expected = w->total_money;
  for(auto entry : {std::pair<ConstructionGroup*,MapPoint>{&warehouseConstructionGroup,{4,4}},
                    {&busStopGroup,{10,10}}, {&busTerminalGroup,{14,10}},
                    {&trackConstructionGroup,{10,11}}}) {
    expected -= entry.first->getCosts(*w);
    w->buildConstruction(*entry.first,entry.second);
    need(w->total_money == expected,"construction did not charge the negative treasury");
  }
  Message::ptr message;
  need(!metroTrackGroup.can_build(*w,message),"negative-balance policy bypassed technology");
  w->tech_level = MAX_TECH_LEVEL;
  expected -= metroTrackGroup.getCosts(*w);
  need(w->transit.place(*w,{10,11},false) && w->total_money == expected,
    "underground track could not charge a negative balance");
  need(!churchConstructionGroup.can_build(*w,message),"non-infrastructure cash check was removed");
}

static void congestedBusStops() {
  auto w = new_barren_city(60);
  w->total_money = 500000;
  w->tech_level = MAX_TECH_LEVEL;
  w->buildConstruction(residenceLLConstructionGroup,{5,4});
  auto* home = static_cast<Residence*>(w->map({5,4})->reportingConstruction);
  home->local_population = 250;
  w->buildConstruction(schoolConstructionGroup,{16,4});
  w->buildConstruction(busStopGroup,{5,7});
  w->buildConstruction(busTerminalGroup,{16,6});
  for(int x=5;x<=17;++x) w->buildConstruction(trackConstructionGroup,{x,8});
  auto* stop = static_cast<TransitFacility*>(w->map({5,7})->reportingConstruction);
  auto* terminal = static_cast<TransitFacility*>(w->map({16,6})->reportingConstruction);
  stop->active = terminal->active = true;
  stop->busy = terminal->busy = 100;
  w->transit.update(*w,false);
  need(stop->overloaded() && !terminal->overloaded(),"small-stop crowding not detected");
  need(stop->endpointTrips == 200 && terminal->endpointTrips == 200
    && w->transit.passengers[0] == 200,"route ignored the bottleneck at a small stop");
}

static void marketFreight() {
  auto w = new_barren_city(60);
  w->total_money = 500000;
  w->tech_level = 0;
  w->buildConstruction(warehouseConstructionGroup,{5,5});
  w->buildConstruction(marketConstructionGroup,{9,5});
  w->buildConstruction(residenceLLConstructionGroup,{13,5});
  auto* warehouse = w->map({5,5})->reportingConstruction;
  auto* market = w->map({9,5})->reportingConstruction;
  auto* home = w->map({13,5})->reportingConstruction;
  warehouse->commodityCount[STUFF_FOOD] = 10000;
  market->commodityCount[STUFF_LABOR] = 500;
  market->update();
  market->update();
  need(warehouse->commodityCount[STUFF_FOOD] < 10000
    && market->commodityCount[STUFF_FOOD] > 0 && home->commodityCount[STUFF_FOOD] > 0,
    "warehouse stock did not flow through a nearby market to housing");
  need(warehouse->commodityCount[STUFF_FOOD] + market->commodityCount[STUFF_FOOD]
    + home->commodityCount[STUFF_FOOD] == 10000,"market trade did not conserve stock");
}

int main(){try{initializeModules();negativeTreasury();congestedBusStops();marketFreight();int shortTrip=passengers(20,false),longTrip=passengers(100,false);need(shortTrip>longTrip&&longTrip>0,"long trips have no efficiency penalty");need(passengers(20,true)==0,"pedestrians walk through an uncrossed river");int near=electricity(13,false),far=electricity(113,false);need(near>far&&far>0,"distance losses missing");need(electricity(13,true)==0,"power crossed cut road");
 auto w=new_barren_city(60);w->total_money=10000000;w->tech_level=MAX_TECH_LEVEL;w->buildConstruction(busStopGroup,{10,9});w->buildConstruction(busStopGroup,{35,11});for(int x=10;x<=35;++x)w->buildConstruction(trackConstructionGroup,{x,10});w->transit.update(*w,false);need(w->transit.connected[0]==2&&w->transit.disconnected[0]==0&&w->transit.unstaffed[0]==2,"track link, opposite sides or staffing reported disconnected");need(w->transit.passengers[0]==0,"unstaffed route carries passengers");w->bulldozeArea({22,10});w->transit.update(*w,false);need(w->transit.connected[0]==0&&w->transit.disconnected[0]==2,"route remains connected across break");
 for(int value:{5,15,25}){w->civic.budget[0]=value;int days=0;for(int day=0;day<100;++day)days+=w->civic.funded(0,day,9);need(days==value,"granular funding not respected");}
 std::cout<<"PASS negative treasury placement, stop capacity, warehouse-to-market-to-home freight, opposite-side dirt-track stops, staffing vs connectivity, broken streets, distance power losses, energy conservation and 5/15/25 percent funding\n";
}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
