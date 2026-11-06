from pathlib import Path
import re
s=Path('preview13-work/CityEvolution-Preview-13-Source')
def edit(f,a,b):
 p=s/f;t=p.read_text();assert a in t,(f,a);p.write_text(t.replace(a,b))
edit('src/lincity/city_evolution.cpp','#include "city_evolution.hpp"','#include "city_evolution.hpp"\n#include "modules/civic_buildings.hpp"\n#include "modules/organic_farm.hpp"\n#include "modules/market.hpp"\n#include "modules/light_industry.hpp"')
edit('src/lincity/city_evolution.cpp','int housing=0,schools=0,wells=0,parks=0,health=0,fire=0;','int housing=0,schools=0,wells=0,parks=0,health=0,fire=0,pop=0,capacity=0,farms=0,markets=0,factories=0,police=0,food=0,goods=0;')
edit('src/lincity/city_evolution.cpp','if(GROUP_IS_RESIDENCE(g)) { ++housing; anchor=p; }','if(GROUP_IS_RESIDENCE(g)) { ++housing; anchor=p;auto* r=static_cast<Residence*>(c);pop+=r->local_population;capacity+=r->max_population;food+=c->commodityCount[STUFF_FOOD];goods+=c->commodityCount[STUFF_GOODS]; }\n    farms+=g==GROUP_ORGANIC_FARM;markets+=g==GROUP_MARKET;factories+=g==GROUP_INDUSTRY_L;police+=g==GROUP_POLICE;')
edit('src/lincity/city_evolution.cpp','else if(housing>=24 && mayorServices && !fire)', 'else if(housing>=8 && mayorServices && !police && w.civic.crime>30) {group=&policeGroup;reason="police response capacity";}\n  else if(housing>=24 && mayorServices && !fire)')
a='  if(!group) {mayorReport=planWait.empty()'
b='''  else if(pop>=100 && housing>=4 && mayorServices && !markets) {group=&marketConstructionGroup;reason="food and goods distribution";}
  else if(housing>=4 && mayorServices && farms<std::max(1,pop/500) && food<pop*2) {group=&organic_farmConstructionGroup;reason="food supply";}
  else if(markets && farms && mayorServices && factories<std::max(1,pop/1000) && goods<pop/2 && engCoverage>=500) {group=&industryLightConstructionGroup;reason="jobs and goods supply";}
  else if(capacity>0 && pop*100/capacity>=85 && w.people_pool>100 && mayorServices) {group=&residenceLLConstructionGroup;reason="housing shortage";}
  if(!group) {mayorReport=planWait.empty()'''
edit('src/lincity/city_evolution.cpp',a,b)
# Rescue pauses a building, but population remains present in city statistics.
edit('src/lincity/simulate.cpp','if(civic.burningAt(cst->point.y*map.len()+cst->point.x) && cst->constructionGroup->group!=GROUP_FIRE) continue;','if(civic.burningAt(cst->point.y*map.len()+cst->point.x) && cst->constructionGroup->group!=GROUP_FIRE) { if(GROUP_IS_RESIDENCE(cst->constructionGroup->group)){auto* r=static_cast<Residence*>(cst);stats.population.population_m+=r->local_population;stats.population.housed_m+=r->local_population;stats.population.housing_m+=r->max_population;} continue; }')
edit('src/lincity/simulate.cpp','#include "world.hpp"','#include "world.hpp"\n#include "modules/residence.hpp"')
edit('src/lincity-ng/Mps.cpp','    + stats.expenses.firestation','    + stats.expenses.police\n    + stats.expenses.firestation')
edit('src/lincity-ng/Mps.cpp','add_ss(N_("Fire Stn"), num_to_ansi(stats.expenses.firestation));','add_ss(N_("Fire / Police"), num_to_ansi(stats.expenses.firestation+stats.expenses.police));')
# Smoke remains animated, with emitters aligned to new chimney pixels.
for f,coords in [('coal_power',[(-85,-325),(-51,-339),(-20,-351)]),('light_industry',[(-79,-248),(-51,-260)])]:
 p=s/('src/lincity/modules/'+f+'.cpp');t=p.read_text()
 for i,(x,y) in enumerate(coords):
  for axis,val in [('x',x),('y',y)]:t=re.sub(r'(frits\['+str(i)+r'\]->move_'+axis+r' = )-?\d+;',r'\g<1>'+str(val)+';',t)
 if f=='coal_power':
  # The other five legacy emitters share one of three real chimney tips.
  for i in range(3,8):
   for axis,val in [('x',coords[i%3][0]),('y',coords[i%3][1])]:t=re.sub(r'(frits\['+str(i)+r'\]->move_'+axis+r' = )-?\d+;',r'\g<1>'+str(val)+';',t)
 p.write_text(t)
# A single notice for the rare event; normal dispatch needs no modal dialogs.
edit('src/lincity-ng/Game.cpp','            while(Message::ptr message = world->popMessage())','            if(world->civic.zombieNoticePending && !blockingDialogIsOpen) { world->civic.zombieNoticePending=false;DialogBuilder().titleText("Pollution outbreak").messageAddText("Your city was so polluted that zombies appeared. S.T.A.R.S. report for duty! Police crews are responding; reduce pollution to prevent another outbreak.").buttonSet(DialogBuilder::ButtonSet::OK).build(); }\n            while(Message::ptr message = world->popMessage())')
