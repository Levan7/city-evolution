// SPDX-License-Identifier: GPL-2.0-or-later
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/street_routes.hpp"
#include "lincity/world.hpp"
#include <set>
#include <iostream>
#include <stdexcept>
static void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
static void valid(const World&w,MapPoint start,const std::vector<MapPoint>&r){std::set<std::pair<int,int>>seen;seen.insert({start.x,start.y});for(auto p:r){need(isStreetTile(w,p),"route leaves street");need(std::abs(p.x-start.x)+std::abs(p.y-start.y)==1,"route jumps");need(seen.insert({p.x,p.y}).second,"route circles a previously visited tile");start=p;}}
int main(){try{initializeModules();auto w=new_barren_city(50);for(int x=4;x<40;++x)w->buildConstruction(trackConstructionGroup,{x,15});for(int y=10;y<=20;++y)for(int x=15;x<=25;++x)if(y==10||y==20||x==15||x==25)if(!w->map({x,y})->reportingConstruction)w->buildConstruction(trackConstructionGroup,{x,y});
 std::set<std::pair<int,int>>goals;for(unsigned seed=0;seed<64;++seed){auto r=planStreetTrip(*w,{20,15},seed);need(r.size()>=6,"trips remain trapped around the junction");valid(*w,{20,15},r);goals.insert({r.back().x,r.back().y});}need(goals.size()>4,"actors all select the same destination");
 auto old=planStreetTrip(*w,{5,15},9);need(!old.empty(),"no connected route");w->bulldozeArea({30,15});for(unsigned seed=0;seed<8;++seed){auto r=planStreetTrip(*w,{5,15},seed);valid(*w,{5,15},r);for(auto p:r)need(p.x<30,"route crosses demolished road");}
 auto isolated=new_barren_city(40);isolated->buildConstruction(trackConstructionGroup,{5,5});isolated->buildConstruction(trackConstructionGroup,{6,5});need(planStreetTrip(*isolated,{5,5},1).empty(),"two-tile fragments create perpetual back-and-forth traffic");need(planStreetTrip(*isolated,{0,0},1).empty(),"edge tile used as road");
 std::cout<<"PASS destination trips, junction escape, loop-free paths, diverse destinations, demolished roads, isolated fragments and bounds\n";
}catch(const Message::Exception&e){std::cerr<<e.getMessage()->str();return 1;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
