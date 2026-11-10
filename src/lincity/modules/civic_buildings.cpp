// SPDX-License-Identifier: GPL-2.0-or-later
#include "civic_buildings.hpp"
#include "lincity/world.hpp"
#include "lincity-ng/Mps.hpp"
#include <algorithm>
CivicBuildingGroup policeGroup("Police station",63,2,8000),glassOfficeGroup("Cyan glass offices",67,3,85000),bronzeOfficeGroup("Charcoal / bronze offices",68,3,85000),decoOfficeGroup("Art Deco offices",69,3,85000);
ResidenceConstructionGroup ivoryTowerGroup("Ivory / blue apartments","Ivory / blue apartments",true,64,3,cyan(24),0,1000,50,60000,MAX_TECH_LEVEL*60/100,0),brickTowerGroup("Red brick apartments","Red brick apartments",true,65,3,cyan(24),0,1000,50,60000,MAX_TECH_LEVEL*60/100,0),sandTowerGroup("Sand / green apartments","Sand / green apartments",true,66,3,cyan(24),0,1000,50,60000,MAX_TECH_LEVEL*60/100,0);
CivicBuildingGroup::CivicBuildingGroup(const std::string& name,int group,int size,int cost):ConstructionGroup(name,name,true,group,size,blue(24),0,1000,50,cost,group==63?0:MAX_TECH_LEVEL*60/100,0){
 commodityRuleCount[STUFF_LABOR]={2000,true,false};commodityRuleCount[STUFF_GOODS]={4000,group==63,group!=63};commodityRuleCount[STUFF_WASTE]={1000,false,true};
 if(group!=63){commodityRuleCount[STUFF_LOVOLT]={2000,true,false};commodityRuleCount[STUFF_WATER]={2000,true,false};}
}
Construction* CivicBuildingGroup::createConstruction(World& w){return new CivicBuilding(w,this);}
CivicBuilding::CivicBuilding(World& w,ConstructionGroup* g):Construction(w){constructionGroup=g;initialize_commodities();}
void CivicBuilding::update(){
 if(constructionGroup->group==63){world.expense(world.civic.cost(CivicServices::POLICE,4, world.total_time, point.x + point.y),world.stats.expenses.police);if(world.civic.funded(CivicServices::POLICE,world.total_time,point.x+point.y)&&commodityCount[STUFF_LABOR]>=4&&commodityCount[STUFF_GOODS]>=1&&commodityCount[STUFF_WASTE]<1000){consumeStuff(STUFF_LABOR,4);consumeStuff(STUFF_GOODS,1);produceStuff(STUFF_WASTE,1);}}
 else if(commodityCount[STUFF_LABOR]>=20&&commodityCount[STUFF_LOVOLT]>=20&&commodityCount[STUFF_WATER]>=4&&commodityCount[STUFF_GOODS]<3950&&commodityCount[STUFF_WASTE]<1000){consumeStuff(STUFF_LABOR,20);consumeStuff(STUFF_LOVOLT,20);consumeStuff(STUFF_WATER,4);produceStuff(STUFF_GOODS,20+20*world.civic.education/1000);produceStuff(STUFF_WASTE,1);}
 if(world.total_time%NUMOF_DAYS_IN_MONTH==NUMOF_DAYS_IN_MONTH-1)reset_prod_counters();
}
void CivicBuilding::report(Mps& m,bool production)const{m.add_s(constructionGroup->name);if(constructionGroup->group==63){m.add_s("4 labor + 1 goods/day; $4/day");int out=0;for(auto& u:world.civic.units)if(u.home==point.y*world.map.len()+point.x)++out;m.add_sfp("Crews responding",out);}else m.add_s("Office services enter the goods network");list_commodities(m,production);}
