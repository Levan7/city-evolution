// SPDX-License-Identifier: GPL-2.0-or-later
#include "church.hpp"
#include "lincity/world.hpp"
#include "lincity-ng/Mps.hpp"
#include "util/xmlutil.hpp"
#include <algorithm>
#include <cstdlib>
#include <libxml++/parsers/textreader.h>
#include <libxml/xmlwriter.h>
ChurchGroup churchConstructionGroup;
ChurchGroup::ChurchGroup():ConstructionGroup("Church","Churches",true,GROUP_CHURCH,3,blue(24),0,1200,0,6000,0,8) {
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
 if(active) {
  consumeStuff(STUFF_LABOR,2);consumeStuff(STUFF_GOODS,1);
  world.tech_level=std::min(MAX_TECH_LEVEL,world.tech_level+1);
  ++workingDays;
  if(totalLessons<1000000000)++totalLessons;
 }
 // Month-end bookkeeping must run whether or not the church was open that day.
 if(world.total_time%NUMOF_DAYS_IN_MONTH==NUMOF_DAYS_IN_MONTH-1) {busy=workingDays;workingDays=0;reset_prod_counters();}
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
