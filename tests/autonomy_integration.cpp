// SPDX-License-Identifier: GPL-2.0-or-later
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/world.hpp"
#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <cstdlib>
static void need(bool x,const char*m){if(!x)throw std::runtime_error(m);}
int main(){try{initializeModules();std::srand(12345);auto w=new_barren_city(80);auto& e=w->evolution;int start=w->total_money;e.setFullAutonomy(*w,true);bool charged=e.mayorSpent>0&&w->total_money<start;
 for(int day=0;day<12000;++day){w->do_time_step();if(day%1200==1199){int pop=0;for(auto*c:w->map.constructions)if(auto*r=dynamic_cast<Residence*>(c))pop+=r->local_population;std::cout<<"day="<<w->total_time<<" pop="<<pop<<" cash="<<w->total_money<<" buildings="<<w->map.constructions.size()<<" tech="<<w->tech_level<<" "<<e.mayorReport<<'\n';}}

 int houseCount=0;for(auto*c:w->map.constructions)houseCount+=GROUP_IS_RESIDENCE(c->constructionGroup->group);need(houseCount>=4,"cannot establish starter housing");need(w->stats.groupCount[GROUP_ORGANIC_FARM]>0&&w->stats.groupCount[GROUP_MARKET]>0&&w->stats.groupCount[GROUP_WATERWELL]>0,"missing essential supply chain");need(charged&&e.mayorProjects>5,"construction was not charged");int pop=0;for(auto*c:w->map.constructions)if(auto*r=dynamic_cast<Residence*>(c))pop+=r->local_population;need(pop>40,"settlement failed to retain residents");need(w->total_money>0,"settlement ran out of cash");
 auto z=new_barren_city(60);for(auto&t:z->map)t.setTerrain(GROUP_TREE);auto& ze=z->evolution;ze.setMayorScopeZone(*z,5,6,30,31);ze.setFullAutonomy(*z,true);for(int month=0;month<12;++month){z->total_time=month*100;ze.runMayor(*z);}int zoneHouses=0;for(auto*c:z->map.constructions)zoneHouses+=GROUP_IS_RESIDENCE(c->constructionGroup->group);need(zoneHouses>=4,"cannot bootstrap wooded zone");for(auto*c:z->map.constructions)need(c->point.x>=5&&c->point.y>=6&&c->point.x+c->constructionGroup->size<=31&&c->point.y+c->constructionGroup->size<=32,"project escaped selected zone");
 if(const char*path=std::getenv("CITY_TEST_SAVE")){if(std::getenv("CITY_TEST_NIGHT"))w->evolution.skyMode=2;w->save(path);}
 auto file=std::filesystem::temp_directory_path()/"autonomy15.scn.gz";z->save(file);auto loaded=World::load(file);std::filesystem::remove(file);need(loaded->evolution.fullAutonomy&&loaded->evolution.mayorScope==2,"full autonomy or area lost on load");
 ze.mayorMode=0;int money=z->total_money,buildings=z->map.constructions.size();ze.runMayor(*z);need(z->total_money==money&&z->map.constructions.size()==buildings,"OFF mayor still spends");
 auto poor=new_barren_city(40);poor->total_money=100;poor->evolution.setFullAutonomy(*poor,true);need(poor->total_money>=0,"planner borrowed unauthorized money");
 std::cout<<"PASS autonomous settlement from empty land, 10-year real simulation, wooded-zone bootstrap, boundary enforcement, paid projects, save/load, OFF and insufficient funding\n";
}catch(const Message::Exception&e){std::cerr<<e.getMessage()->str()<<'\n';return 1;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
