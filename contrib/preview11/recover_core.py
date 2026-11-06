from pathlib import Path
S=Path(__file__).resolve().parent/'CityEvolution-Preview-11-Source'
def put(n,s):p=S/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(s)
def change(n,a,b,count=1):
 p=S/n;s=p.read_text();assert a in s,(n,a[:80]);p.write_text(s.replace(a,b,count))
put('src/lincity/traffic_era.hpp',r'''// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <cstdint>
namespace TrafficEra {
inline int motorShare(int tech,int maximum) {
 if(maximum<=0 || tech*100LL<maximum*10)return 0;
 return std::clamp(1+int((tech*100LL-maximum*10)*99/(maximum*20LL)),1,100);
}
inline bool motorVehicle(int tech,int maximum,std::uint32_t seed){return int(seed%100)<motorShare(tech,maximum);}
}
''')
put('src/lincity/modules/church.hpp',r'''// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "lincity/lintypes.hpp"
class ChurchGroup:public ConstructionGroup {
public:ChurchGroup();Construction* createConstruction(World&) override;
};
extern ChurchGroup churchConstructionGroup;
class GeorgianChurch:public Construction {
public:
 static constexpr int Radius=8,Morale=8,DailyCost=8;
 int workingDays=0,busy=0,totalLessons=0;bool active=false;
 explicit GeorgianChurch(World&);
 void update() override;
 void report(Mps&,bool) const override;
 void save(xmlTextWriterPtr) const override;
 bool loadMember(xmlpp::TextReader&,unsigned int) override;
};
bool churchServes(const World&,MapPoint);
''')
put('src/lincity/modules/church.cpp',r'''// SPDX-License-Identifier: GPL-2.0-or-later
#include "church.hpp"
#include "lincity/world.hpp"
#include "lincity-ng/Mps.hpp"
#include "util/xmlutil.hpp"
#include <algorithm>
#include <cstdlib>
#include <libxml++/parsers/textreader.h>
#include <libxml/xmlwriter.h>
ChurchGroup churchConstructionGroup;
ChurchGroup::ChurchGroup():ConstructionGroup("Georgian Orthodox church","Georgian Orthodox churches",true,GROUP_CHURCH,3,blue(24),0,1200,0,6000,0,8) {
 commodityRuleCount[STUFF_LABOR]={200,true,false};commodityRuleCount[STUFF_GOODS]={100,true,false};
}
Construction* ChurchGroup::createConstruction(World& w){return new GeorgianChurch(w);}
GeorgianChurch::GeorgianChurch(World& w):Construction(w) {
 constructionGroup=&churchConstructionGroup;initialize_commodities();
 commodityMaxCons[STUFF_LABOR]=200;commodityMaxCons[STUFF_GOODS]=100;
}
void GeorgianChurch::update() {
 world.expense(DailyCost,world.stats.expenses.church);
 active=commodityCount[STUFF_LABOR]>=2&&commodityCount[STUFF_GOODS]>=1;
 if(active){consumeStuff(STUFF_LABOR,2);consumeStuff(STUFF_GOODS,1);world.tech_level=std::min(MAX_TECH_LEVEL,world.tech_level+1);++workingDays;if(totalLessons<1000000000)++totalLessons;}
 if(world.total_time%NUMOF_DAYS_IN_MONTH==NUMOF_DAYS_IN_MONTH-1){busy=workingDays;workingDays=0;reset_prod_counters();}
}
bool churchServes(const World& w,MapPoint p) {
 for(auto* c:w.map.constructions) {
  if(c->constructionGroup->group!=GROUP_CHURCH||!static_cast<const GeorgianChurch*>(c)->active)continue;
  int x=std::clamp(p.x,c->point.x,c->point.x+2),y=std::clamp(p.y,c->point.y,c->point.y+2);
  if(std::abs(p.x-x)+std::abs(p.y-y)<=8)return true;
 }return false;
}
void GeorgianChurch::report(Mps& m,bool production)const {
 m.add_s(constructionGroup->name);m.add_s(active?"Community service open":"Needs 2 labor + 1 goods/day");
 m.add_sfp("Operating days / month",busy);m.add_s("Spiritual wellbeing: radius 8");
 m.add_s("Housing morale +8; no stacking");m.add_s("Community lessons: +1 tech/day");
 m.add_s("$8/day; schools still needed");list_commodities(m,production);
}
void GeorgianChurch::save(xmlTextWriterPtr w)const {
 Construction::save(w);
 xmlTextWriterWriteElement(w,(xmlStr)"church_working_days",xmlFormat<int>(workingDays));
 xmlTextWriterWriteElement(w,(xmlStr)"church_busy",xmlFormat<int>(busy));
 xmlTextWriterWriteElement(w,(xmlStr)"church_lessons",xmlFormat<int>(totalLessons));
}
bool GeorgianChurch::loadMember(xmlpp::TextReader& r,unsigned int v) {
 auto n=r.get_name();
 if(n=="church_working_days")workingDays=std::clamp(xmlParse<int>(r.read_inner_xml()),0,NUMOF_DAYS_IN_MONTH);
 else if(n=="church_busy")busy=std::clamp(xmlParse<int>(r.read_inner_xml()),0,NUMOF_DAYS_IN_MONTH);
 else if(n=="church_lessons")totalLessons=std::clamp(xmlParse<int>(r.read_inner_xml()),0,1000000000);
 else return Construction::loadMember(r,v);return true;
}
''')
change('src/lincity/groups.hpp','#define NUM_OF_GROUPS    62','#define NUM_OF_GROUPS    63\n#define GROUP_CHURCH 62')
change('src/lincity/modules/all_modules.hpp','#include "city_business.hpp"','#include "city_business.hpp"\n#include "church.hpp"')
change('src/lincity/modules/all_modules.cpp','    ConstructionGroup::clearResourcepMap();','    ConstructionGroup::addConstructionGroup(&churchConstructionGroup);\n    ConstructionGroup::clearResourcepMap();\n    ConstructionGroup::addResourceID("GeorgianChurch",&churchConstructionGroup);\n    new ResourceGroup("HorseTraffic"); ResourceGroup::resMap["HorseTraffic"]->is_vehicle=true;\n    new ResourceGroup("CommuneVariant1"); new ResourceGroup("CommuneVariant2");')
change('src/lincity/modules/CMakeLists.txt','  public_transport.cpp','  church.cpp\n  church.hpp\n  public_transport.cpp')
change('src/lincity/CMakeLists.txt','  transit.hpp','  transit.hpp\n  traffic_era.hpp')
change('src/lincity/stats.hpp','    Stat<int> school;','    Stat<int> school;\n    Stat<int> church;')
change('src/lincity/stats.cpp','  expenses.school.finalize();','  expenses.school.finalize();\n  expenses.church.finalize();')
change('src/lincity/xmlloadsave.cpp','      xmlTextWriterWriteElement(xmlWriter, (xmlStr)"school_ly",','      xmlTextWriterWriteElement(xmlWriter, (xmlStr)"church_ly", f(world.stats.expenses.church.stat));\n      xmlTextWriterWriteElement(xmlWriter, (xmlStr)"church_ytd", f(world.stats.expenses.church.acc));\n      xmlTextWriterWriteElement(xmlWriter, (xmlStr)"school_ly",')
change('src/lincity/xmlloadsave.cpp','            else if(xml_tag == "school_ly")','            else if(xml_tag == "church_ly") world.stats.expenses.church.stat = p<int>(xmlReader.read_inner_xml());\n            else if(xml_tag == "church_ytd") world.stats.expenses.church.acc = p<int>(xmlReader.read_inner_xml());\n            else if(xml_tag == "school_ly")')
change('src/lincity-ng/Mps.cpp','    + stats.expenses.school','    + stats.expenses.school\n    + stats.expenses.church')
change('src/lincity-ng/Mps.cpp','  add_ss(N_("Schools") , num_to_ansi(stats.expenses.school));','  add_ss(N_("Schools / Church"), num_to_ansi(stats.expenses.school+stats.expenses.church));')
change('src/lincity/modules/residence.cpp','#include "residence.hpp"','#include "residence.hpp"\n#include "church.hpp"')
change('src/lincity/modules/residence.cpp','    int bad = 35, good = 30;','    int bad = 35, good = 30;\n    if(churchServes(world,point)) good += GeorgianChurch::Morale;')
change('src/lincity/city_evolution.hpp','  int residents = 0,','  int churchesWorking=0,spiritualCoverage=0;\n  int residents = 0,')
change('src/lincity/city_evolution.cpp','#include "city_evolution.hpp"','#include "city_evolution.hpp"\n#include "modules/church.hpp"')
change('src/lincity/city_evolution.cpp','  int fed = 0, employed = 0, pollution = 0;','  int fed = 0, employed = 0, pollution = 0, spiritualHomes=0;\n  churchesWorking=0;')
change('src/lincity/city_evolution.cpp','      ++homes;','      ++homes;\n      if(churchServes(world,c->point))++spiritualHomes;')
change('src/lincity/city_evolution.cpp','    if(g >= GROUP_WAREHOUSE','    if(g==GROUP_CHURCH&&static_cast<GeorgianChurch*>(c)->active)++churchesWorking;\n    if(g >= GROUP_WAREHOUSE')
change('src/lincity/city_evolution.cpp','  if(world.total_money < 0) target -= 10;','  spiritualCoverage=homes?spiritualHomes*100/homes:0;\n  target+=spiritualCoverage*6/100;\n  if(world.total_money < 0) target -= 10;')
change('src/lincity/Vehicles.cpp','#include "world.hpp"','#include "traffic_era.hpp"\n#include "world.hpp"')
change('src/lincity/Vehicles.cpp','    frameIt->resourceGroup =  ResourceGroup::resMap["Bluecar"];','    frameIt->resourceGroup=ResourceGroup::resMap[TrafficEra::motorVehicle(world.tech_level,MAX_TECH_LEVEL,unsigned(point.x*131+point.y*17))?"Bluecar":"HorseTraffic"];')
change('src/lincity/Vehicles.cpp','    speed0 = BLUE_CAR_SPEED;','    speed0=frameIt->resourceGroup==ResourceGroup::resMap["HorseTraffic"]?BLUE_CAR_SPEED*2:BLUE_CAR_SPEED;')
change('src/lincity-ng/GameView.hpp','    void connectButtons();','    void connectButtons();\n    void updateBuildingViewIndicator();')
change('src/lincity-ng/GameView.cpp','#include "GameView.hpp"','#include "GameView.hpp"\n#include "lincity/traffic_era.hpp"')
change('src/lincity-ng/GameView.cpp','this->game = game;','    this->game = game;\n    hideHigh=false; mapOverlay=overlayNone; updateBuildingViewIndicator(); setMapDirty();')
change('src/lincity-ng/GameView.cpp','    Button* button = getButton( *root, "hideHighBuildings" );','    auto* show=getButton(*root,"ShowBuildingsButton");\n    show->clicked.connect(std::bind(&GameView::buttonClicked,this,_1));\n    updateBuildingViewIndicator();\n    Button* button = getButton( *root, "hideHighBuildings" );')
change('src/lincity-ng/GameView.cpp','void GameView::buttonClicked( Button* button ){','void GameView::updateBuildingViewIndicator(){Component* root=this;while(root->getParent())root=root->getParent();if(auto* b=dynamic_cast<Button*>(root->findComponent("ShowBuildingsButton")))b->setCaptionText(hideHigh?"Models hidden - click to show":"Building models visible");}\nvoid GameView::buttonClicked( Button* button ){')
change('src/lincity-ng/GameView.cpp','    if( name == "hideHighBuildings" ){','    if(name=="ShowBuildingsButton"){hideHigh=false;mapOverlay=overlayNone;updateBuildingViewIndicator();setDirty();setMapDirty();return;}\n    if( name == "hideHighBuildings" ){')
change('src/lincity-ng/GameView.cpp','        hideHigh = !hideHigh;\n        setDirty();','        hideHigh = !hideHigh;\n        updateBuildingViewIndicator();\n        setDirty();')
change('src/lincity-ng/GameView.cpp','      hideHigh = !hideHigh;\n      setDirty();','      hideHigh = !hideHigh;\n      updateBuildingViewIndicator();\n      setDirty();')
change('src/lincity-ng/GameView.cpp','          const char* name=cstgrp->group==GROUP_ORGANIC_FARM?"horse-cart":cstgrp->group==GROUP_WAREHOUSE?"truck":"horse";','          bool motor=TrafficEra::motorVehicle(getWorld().tech_level,MAX_TECH_LEVEL,unsigned(upperLeft.x*131+upperLeft.y*17));\n          const char* name=cstgrp->group==GROUP_ORGANIC_FARM?"horse-cart":cstgrp->group==GROUP_WAREHOUSE?(motor?"truck":"horse-cart"):"horse";')
change('src/lincity-ng/GameView.cpp',"// Static working-city details belong to their building's depth-sorted draw pass.\n        if(mapOverlay != overlayOnly &&","// Static details follow their building visibility.\n        if(!hideHigh && mapOverlay != overlayOnly &&")
change('src/lincity-ng/GameView.cpp','      auto texture=texture_manager->load(std::string("images/transit/car-")+direction+".png");','      bool motor=TrafficEra::motorVehicle(getWorld().tech_level,MAX_TECH_LEVEL,a.appearance);\n      const char* kind=motor?"car-":(a.appearance%2?"carriage-":"horse-walk-");\n      auto texture=texture_manager->load(std::string("images/transit/")+kind+direction+".png");')
change('src/lincity-ng/GameView.cpp','    a.progress += delta / (a.car ? 1900.f : 5200.f);','    bool motor=TrafficEra::motorVehicle(world.tech_level,MAX_TECH_LEVEL,a.appearance);\n    a.progress += delta/(a.car?(motor?1900.f:3600.f):5200.f);')
change('src/lincity-ng/GameView.cpp','    auto texture=texture_manager->load(std::string("images/transit/")+(route.mode==0?"bus-":"train-")+directions[dir]+".png");','    bool motor=TrafficEra::motorVehicle(w.tech_level,MAX_TECH_LEVEL,unsigned(ri*37));\n    auto texture=texture_manager->load(std::string("images/transit/")+(route.mode==0?(motor?"bus-":"carriage-"):"train-")+directions[dir]+".png");')
p=S/'src/lincity-ng/Game.cpp';s=p.read_text();start=s.index('  const std::pair<const char*,ConstructionGroup*> tools[]={',s.index('void Game::showTransport()'));end=s.index('  getButton(*w,"TransitView")',start);s=s[:start]+s[end:];s=s.replace('  userOperation = op;','  userOperation = op;\n  if(op.action==UserOperation::ACTION_BUILD&&op.constructionGroup){int g=op.constructionGroup->group;if(g==GROUP_METRO_TRACK)world->transit.view=2;else if(g==GROUP_ELEVATED_TRACK)world->transit.view=3;else world->transit.view=0;getGameView().setMapDirty();}',1);s=s.replace('getParagraph(*w,"AtmosphereSummary")->setText("Automatic skies. Solar panels now produce " + std::to_string(e.solarPercent()) + "% of full power.");','getParagraph(*w,"AtmosphereSummary")->setText("Spiritual wellbeing: "+std::to_string(e.spiritualCoverage)+"% homes | "+std::to_string(e.churchesWorking)+" churches open. Solar: "+std::to_string(e.solarPercent())+"%.");');p.write_text(s)
p=S/'src/lincity-ng/ButtonPanel.cpp';s=p.read_text().replace('  button->enable(tool->button->isEnabled());','  button->enable(true);').replace('      tool->button->setTooltip(os.str());\n    }\n  }\n}','      tool->button->setTooltip(os.str());\n    }\n  }\n  for(auto& m:menus)m.first->enable(true);\n}');p.write_text(s)
change('src/lincity/modules/city_business.cpp','mps.add_s("Buffer: connect road and rail at edges. $6/day.");','{mps.add_s("Road / rail commodity buffer");mps.add_s("$6/day; connect at the edges");}')
# Stable placement-derived variants retain the exact original simulation frame contract.
change('src/lincity/lintypes.cpp','    ResourceGroup *resGroup = ResourceGroup::resMap[constructionGroup->resourceID];','    ResourceGroup *resGroup = ResourceGroup::resMap[constructionGroup->resourceID];\n    if(constructionGroup->group==GROUP_COMMUNE){unsigned v=unsigned(point.x*31+point.y*17)%3;if(v)resGroup=ResourceGroup::resMap[v==1?"CommuneVariant1":"CommuneVariant2"];}')
for n in ['src/lincity-ng/main.cpp','src/lincity-ng/MainMenu.cpp']:
 p=S/n;p.write_text(p.read_text().replace('Preview 9','Preview 11').replace('Preview-9','Preview-11'))
print('Restored church, traffic, rendering, reports, category and variance code')
