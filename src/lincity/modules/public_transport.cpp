// SPDX-License-Identifier: GPL-2.0-or-later
#include "public_transport.hpp"
#include "lincity/world.hpp"
#include "lincity-ng/Mps.hpp"
#include "util/xmlutil.hpp"
#include <libxml++/parsers/textreader.h>
#include <libxml/xmlwriter.h>
TransitGroup busStopGroup("Bus stop",GROUP_BUS_STOP,1,800,0),busTerminalGroup("Bus terminal",GROUP_BUS_TERMINAL,2,7500,0),railStationGroup("Passenger railway station",GROUP_RAIL_STATION,3,15000,MAX_TECH_LEVEL/20),metroStationGroup("Underground metro station",GROUP_METRO_STATION,1,18000,MAX_TECH_LEVEL/10),elevatedStationGroup("Elevated metro station",GROUP_ELEVATED_STATION,2,22000,MAX_TECH_LEVEL/10),metroTrackGroup("Underground metro track",GROUP_METRO_TRACK,1,1000,MAX_TECH_LEVEL/10,true),elevatedTrackGroup("Elevated metro track",GROUP_ELEVATED_TRACK,1,1800,MAX_TECH_LEVEL/10,true);
TransitGroup::TransitGroup(const char* n,int g,int sz,int c,int tech,bool l):ConstructionGroup(n,n,true,g,sz,blue(22),0,c/5,0,c,tech,0),layer(l) {
 if(!l){commodityRuleCount[STUFF_LABOR]={2000,true,false};commodityRuleCount[STUFF_LOVOLT]={4000,true,false};}
}
Construction* TransitGroup::createConstruction(World& w){return new TransitFacility(w,this);}
bool TransitGroup::can_build_here(const World& w,MapPoint p,Message::ptr& msg)const{
 if(!layer)return ConstructionGroup::can_build_here(w,p,msg);
 if(!w.map.is_visible(p)){msg=OutsideMapMessage::create(p);return false;}
 const auto& cells=group==GROUP_ELEVATED_TRACK?w.transit.viaducts:w.transit.tunnels;
 if(cells.count(p.y*w.map.len()+p.x)){msg=CityRequirementMessage::create("Track already exists on this layer.");return false;}
 return true;
}
TransitFacility::TransitFacility(World& w,ConstructionGroup* g):Construction(w){constructionGroup=g;initialize_commodities();}
int TransitFacility::mode()const{return constructionGroup->group==GROUP_RAIL_STATION?1:constructionGroup->group==GROUP_METRO_STATION?2:constructionGroup->group==GROUP_ELEVATED_STATION?3:0;}
void TransitFacility::update(){
 int m=mode(),staff=m==0?2:8,power=m==0?0:15;
 active=commodityCount[STUFF_LABOR]>=staff&&commodityCount[STUFF_LOVOLT]>=power;
 if(active){consumeStuff(STUFF_LABOR,staff);if(power)consumeStuff(STUFF_LOVOLT,power);++workingDays;}
 int cost=constructionGroup->group==GROUP_BUS_STOP?4:constructionGroup->group==GROUP_BUS_TERMINAL?18:m==1?35:m==2?40:45;
 world.expense(cost,world.stats.expenses.transport);world.transit.upkeepMonth+=cost;
 if(world.total_time%NUMOF_DAYS_IN_MONTH==NUMOF_DAYS_IN_MONTH-1){busy=workingDays;workingDays=0;reset_prod_counters();}
}
void TransitFacility::report(Mps& m,bool production)const{
 m.add_s(constructionGroup->name);m.add_s(active?"Running":"Needs staff / power");
 m.add_sfp("Days active",busy);m.add_sfp("Journeys / month",riders);
 m.add_s("Needs 2 connected stops.");
 m.add_s("Top-right: Transport");list_commodities(m,production);
}
void TransitFacility::save(xmlTextWriterPtr w)const{Construction::save(w);xmlTextWriterWriteElement(w,(xmlStr)"transit_days",xmlFormat<int>(workingDays));xmlTextWriterWriteElement(w,(xmlStr)"transit_busy",xmlFormat<int>(busy));}
bool TransitFacility::loadMember(xmlpp::TextReader& r,unsigned int v){if(r.get_name()=="transit_days")workingDays=std::clamp(xmlParse<int>(r.read_inner_xml()),0,100);else if(r.get_name()=="transit_busy")busy=std::clamp(xmlParse<int>(r.read_inner_xml()),0,100);else return Construction::loadMember(r,v);return true;}
