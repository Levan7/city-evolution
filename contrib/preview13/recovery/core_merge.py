from pathlib import Path
s=Path('preview13-work/CityEvolution-Preview-13-Source')
def edit(f,old,new):
 p=s/f;t=p.read_text();assert old in t,(f,old);p.write_text(t.replace(old,new))
edit('src/lincity/civic_services.cpp','w.dole','w.money_rates.dole');edit('src/lincity/civic_services.cpp','w.income_tax','w.money_rates.income_tax')
edit('src/lincity/CMakeLists.txt','  city_evolution.cpp','  civic_services.cpp\n  civic_services.hpp\n  city_evolution.cpp')
edit('src/lincity/world.hpp','#include "city_evolution.hpp"','#include "city_evolution.hpp"\n#include "civic_services.hpp"')
edit('src/lincity/world.hpp','  CityEvolution evolution;','  CivicServices civic;\n  CityEvolution evolution;')
edit('src/lincity/groups.hpp','#define NUM_OF_GROUPS    63','#define NUM_OF_GROUPS    70\n#define GROUP_POLICE 63')
edit('src/lincity/groups.hpp','((group == GROUP_RESIDENCE_LL)', '(((group >= 64) && (group <=66)) || (group == GROUP_RESIDENCE_LL)')
edit('src/lincity/simulate.cpp','    evolution.updateCity(*this);','    evolution.updateCity(*this);\n    civic.update(*this);')
edit('src/lincity/simulate.cpp','    cst->trade();','    if(civic.burningAt(cst->point.y*map.len()+cst->point.x) && cst->constructionGroup->group!=GROUP_FIRE) continue;\n    cst->trade();')
edit('src/lincity/lintypes.cpp','Construction::torch() {','Construction::torch() {\n  if(world.civic.beginFire(world,*this)) return;')
edit('src/lincity/lintypes.cpp','if(GROUP_IS_RESIDENCE(constructionGroup->group)&&','if(constructionGroup->group<64&&GROUP_IS_RESIDENCE(constructionGroup->group)&&')
edit('src/lincity/modules/residence.cpp','Residence::torch() {','Residence::torch() {\n  if(world.civic.beginFire(world,*this)) return;')
edit('src/lincity/modules/residence.cpp','    int bad = 35, good = 30;','    int bad = 35, good = 30;\n    good+=world.civic.education/100+std::min(10,world.money_rates.dole/5);\n    bad+=std::max(0,world.money_rates.income_tax-8)+world.civic.illAt(point.y*world.map.len()+point.x)/3;')
edit('src/lincity/modules/residence.cpp','int job_prod = local_population * (working_pop + swing_amt) / 100;','int job_prod = std::max(0,local_population-world.civic.illAt(point.y*world.map.len()+point.x)) * (working_pop + swing_amt) / 100;')
edit('src/lincity/modules/residence.cpp','  else {\n    this->max_population = 50;','  else if(cstgrp->group>=64 && cstgrp->group<=66) this->max_population=600;\n  else {\n    this->max_population = 50;')
edit('src/lincity/modules/residence.hpp','case GROUP_RESIDENCE_HH :','case 64: case 65: case 66: case GROUP_RESIDENCE_HH :')
edit('src/lincity/modules/fire.cpp','    if (world.map(point)->flags & FLAG_FIRE_COVER)\n    {   burning_days += 4;}','    // Suppression is applied by an arrived response crew, not a radius flag.')
edit('src/lincity/modules/fire.cpp','if(world.map(loc)->flags & FLAG_FIRE_COVER)','if(world.civic.fireResponseAt(loc.y*world.map.len()+loc.x))')
edit('src/lincity/xmlloadsave.cpp','static void saveGlobals(xmlTextWriterPtr xmlWriter, const World& world) {','static void saveGlobals(xmlTextWriterPtr xmlWriter, const World& world) {\n  xmlTextWriterWriteElement(xmlWriter,(xmlStr)"civic_state",(xmlStr)world.civic.save().c_str());')
edit('src/lincity/xmlloadsave.cpp','else if(xml_tag == "transit_tunnel" ||','else if(xml_tag == "civic_state") world.civic.pendingLoad=xmlReader.read_string2().value_or("");\n    else if(xml_tag == "transit_tunnel" ||')
edit('src/lincity/xmlloadsave.cpp','if(id>=0&&id<len*len&&world.map.is_visible(MapPoint(id%len,id/len)))','if(id>=0&&id<160000)')
edit('src/lincity/xmlloadsave.cpp','  // Extension fields are optional','  if(!world->civic.pendingLoad.empty()) { auto state=std::move(world->civic.pendingLoad);world->civic.load(state,world->map.len()); }\n  for(auto* cells:{&world->transit.tunnels,&world->transit.viaducts}) for(auto it=cells->begin();it!=cells->end();) { int n=world->map.len(),id=*it;if(id<0||id>=n*n||!world->map.is_visible(MapPoint(id%n,id/n)))it=cells->erase(it);else ++it; }\n  // Extension fields are optional')
# Funding affects work without bypassing bookkeeping on the last day of a month.
for f,service,condition in [('school','EDUCATION','LABOR_MAKE_TECH_SCHOOL'),('university','EDUCATION','UNIVERSITY_LABOR'),('health_centre','HEALTH','HEALTH_CENTRE_LABOR'),('firestation','FIRE','FIRESTATION_LABOR')]:
 edit('src/lincity/modules/'+f+'.cpp','if (commodityCount[STUFF_LABOR] >= '+condition,'if (world.civic.funded(CivicServices::'+service+',world.total_time,point.x+point.y) && commodityCount[STUFF_LABOR] >= '+condition)
# Running cost expressions: keep the exact existing accounting destinations.
edit('src/lincity/modules/school.cpp','world.stats.expenses.school += SCHOOL_RUNNING_COST;','world.stats.expenses.school += world.civic.cost(CivicServices::EDUCATION,SCHOOL_RUNNING_COST);')
edit('src/lincity/modules/university.cpp','world.expense(UNIVERSITY_RUNNING_COST,','world.expense(world.civic.cost(CivicServices::EDUCATION,UNIVERSITY_RUNNING_COST),')
for f,svc,constant,account in [('health_centre','HEALTH','HEALTH','health'),('firestation','FIRE','FIRESTATION','firestation')]:
 p=s/('src/lincity/modules/'+f+'.cpp');t=p.read_text();start=t.index('world.expense('+constant+'_RUNNING_COST');end=t.index('world.stats.expenses.'+account,start);part=t[start:end];part=part.replace('world.expense(','world.expense(world.civic.cost(CivicServices::'+svc+',',1);# final comma before ledger closes scaled expression
 pos=part.rfind(',');part=part[:pos]+'),'+part[pos+1:];t=t[:start]+part+t[end:];p.write_text(t)
