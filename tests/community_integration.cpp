// SPDX-License-Identifier: GPL-2.0-or-later
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/traffic_era.hpp"
#include "lincity/world.hpp"
#include "lincity/resources.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
static void require(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main(){try{
 initializeModules();
 int last=0;
 for(int tech=0;tech<=MAX_TECH_LEVEL;tech+=MAX_TECH_LEVEL/1000){
  int share=TrafficEra::motorShare(tech,MAX_TECH_LEVEL),count=0;require(share>=last&&share>=0&&share<=100,"motor share monotonicity");last=share;
  for(unsigned i=0;i<100;i++)count+=TrafficEra::motorVehicle(tech,MAX_TECH_LEVEL,i);
  require(count==share,"motor seed mix");
 }
 require(TrafficEra::motorShare(0,MAX_TECH_LEVEL)==0&&TrafficEra::motorShare(MAX_TECH_LEVEL/10-1,MAX_TECH_LEVEL)==0&&TrafficEra::motorShare(MAX_TECH_LEVEL/10,MAX_TECH_LEVEL)==1&&TrafficEra::motorShare(MAX_TECH_LEVEL*3/10,MAX_TECH_LEVEL)==100,"traffic thresholds");
 std::cout<<"PASS motor traffic thresholds, monotonicity and all seed buckets\n";
 auto w=new_barren_city(50);w->total_money=1000000;w->tech_level=0;
 w->buildConstruction(churchConstructionGroup,MapPoint(20,20));auto* c=dynamic_cast<GeorgianChurch*>(w->map(MapPoint(20,20))->reportingConstruction);
 require(c&&w->total_money==994000,"church registration/cost");
 c->commodityCount[STUFF_LABOR]=20;c->commodityCount[STUFF_GOODS]=10;int cash=w->total_money;c->update();
 require(c->active&&w->tech_level==1&&c->totalLessons==1&&c->workingDays==1,"church education/activity");
 require(w->total_money==cash-8&&w->stats.expenses.church.acc==8&&c->commodityCount[STUFF_LABOR]==18&&c->commodityCount[STUFF_GOODS]==9,"church supplies/upkeep");
 require(churchServes(*w,MapPoint(30,21))&&!churchServes(*w,MapPoint(31,21)),"church coverage radius");
 w->buildConstruction(residenceLLConstructionGroup,MapPoint(24,24));auto* home=static_cast<Residence*>(w->map(MapPoint(24,24))->reportingConstruction);home->local_population=20;w->total_time=100;w->evolution.updateCity(*w);require(w->evolution.spiritualCoverage==100,"spiritual wellbeing");
 for(auto p:{MapPoint(2,2),MapPoint(6,2),MapPoint(10,2)})w->buildConstruction(communeConstructionGroup,p);
 for(auto p:{MapPoint(2,2),MapPoint(6,2),MapPoint(10,2)}){
  auto* frames=w->map(p)->framesptr;require(frames&&!frames->empty(),"commune frame");
  unsigned v=unsigned(p.x*31+p.y*17)%3;require(frames->front().resourceGroup==ResourceGroup::resMap[v==0?"Commune":v==1?"CommuneVariant1":"CommuneVariant2"],"stable commune variant assignment");
 }
 const auto path=std::filesystem::temp_directory_path()/"city-community-check.scn.gz";w->save(path);auto loaded=World::load(path);std::filesystem::remove(path);
 auto* saved=dynamic_cast<GeorgianChurch*>(loaded->map(MapPoint(20,20))->reportingConstruction);
 require(saved&&saved->totalLessons==1&&saved->workingDays==1&&loaded->stats.expenses.church.acc==8,"church save/load");
 saved->commodityCount[STUFF_GOODS]=0;int tech=loaded->tech_level;saved->update();require(!saved->active&&tech==loaded->tech_level&&!churchServes(*loaded,home->point),"closed church benefits");
 loaded->bulldozeArea(MapPoint(20,20));require(loaded->stats.groupCount[GROUP_CHURCH]==0,"church demolition");
  { // Month-end bookkeeping must happen even when the church is shut on the last day of the month.
  auto cw=new_barren_city(40);cw->total_money=1000000;cw->buildConstruction(churchConstructionGroup,MapPoint(10,10));
  auto* ch=dynamic_cast<GeorgianChurch*>(cw->map(MapPoint(10,10))->reportingConstruction);
  for(int d=0;d<NUMOF_DAYS_IN_MONTH;++d){cw->total_time=d;ch->commodityCount[STUFF_LABOR]=d<NUMOF_DAYS_IN_MONTH-1?10:0;ch->commodityCount[STUFF_GOODS]=10;ch->update();}
  require(ch->busy==NUMOF_DAYS_IN_MONTH-1&&ch->workingDays==0&&ch->totalLessons==NUMOF_DAYS_IN_MONTH-1,"church counters must close the month even if it is shut on the last day");
 }
 std::cout<<"PASS church registration, staffing, upkeep, learning, coverage, wellbeing, save/load and demolition\n";
 std::cout<<"PASS stable placement-based commune variants\n";
}catch(const Message::Exception& e){std::cerr<<e.getMessage()->str()<<'\n';return 1;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
