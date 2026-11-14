// SPDX-License-Identifier: GPL-2.0-or-later
#include "city_business.hpp"
#include <algorithm>
#include "lincity/world.hpp"
#include "lincity-ng/Mps.hpp"
#include "util/xmlutil.hpp"
#include <libxml++/parsers/textreader.h>
#include <libxml/xmlwriter.h>
CityBusinessGroup warehouseConstructionGroup("Freight warehouse", GROUP_WAREHOUSE, 2, 6000, 0);
CityBusinessGroup techCampusConstructionGroup("Technology campus", GROUP_TECH_CAMPUS, 3, 35000, MAX_TECH_LEVEL/4);
CityBusinessGroup cafeConstructionGroup("Neighborhood cafe", GROUP_CAFE, 1, 3000, MAX_TECH_LEVEL/50);
CityBusinessGroup foodHallConstructionGroup("Food hall and bar", GROUP_FOODHALL, 2, 8000, MAX_TECH_LEVEL*15/100);
CityBusinessGroup largeWarehouseConstructionGroup("Freight distribution center", GROUP_LARGE_WAREHOUSE, 3, 22000, MAX_TECH_LEVEL/10);
CityBusinessGroup computerFactoryConstructionGroup("Computer factory", GROUP_COMPUTER_FACTORY, 3, 60000, MAX_TECH_LEVEL/2);
CityBusinessGroup carFactoryConstructionGroup("Car factory", GROUP_CAR_FACTORY, 4, 85000, MAX_TECH_LEVEL*3/5);
CityBusinessGroup nuclearPowerConstructionGroup("Nuclear power station", GROUP_NUCLEAR_POWER, 4, 120000, MAX_TECH_LEVEL*3/4);

namespace {
struct Production {
  int labor, water, power, steel, waste;
  Commodity output;
  int amount;
};

Production productionFor(int group) {
  switch(group) {
  case GROUP_COMPUTER_FACTORY: return {30, 12, 120, 12, 3, STUFF_GOODS, 240};
  case GROUP_CAR_FACTORY: return {45, 24, 180, 45, 6, STUFF_GOODS, 420};
  case GROUP_NUCLEAR_POWER: return {65, 180, 0, 0, 8, STUFF_HIVOLT, 18000};
  default: return {0, 0, 0, 0, 0, STUFF_GOODS, 0};
  }
}
}

CityBusinessGroup::CityBusinessGroup(const std::string& n, int g, int s, int c, int t) :
  ConstructionGroup(n, n, !isWarehouseGroup(g), g, s, blue(22), 0, c/5, 20, c, t, isWarehouseGroup(g) ? (s == 3 ? 24 : 14) : 0) {
  auto rule = [this](Commodity c, int capacity, bool take, bool give) {
    commodityRuleCount[c] = CommodityRule{capacity, take, give};
  };
  const auto recipe = productionFor(g);
  if(isWarehouseGroup(g)) {
    for(Commodity c : {STUFF_FOOD, STUFF_COAL, STUFF_GOODS, STUFF_ORE, STUFF_STEEL})
      rule(c, g == GROUP_LARGE_WAREHOUSE ? 80000 : 20000, true, true);
  } else if(recipe.amount) {
    rule(STUFF_LABOR, recipe.labor * 20, true, false);
    rule(STUFF_WATER, recipe.water * 20, true, false);
    if(recipe.power) rule(STUFF_LOVOLT, recipe.power * 20, true, false);
    if(recipe.steel) rule(STUFF_STEEL, recipe.steel * 20, true, false);
    rule(STUFF_WASTE, recipe.waste * 20, false, true);
    rule(recipe.output, recipe.amount * 12, false, true);
  } else {
    rule(STUFF_LABOR, 2000, true, false);
    rule(STUFF_GOODS, g == GROUP_TECH_CAMPUS ? 12000 : 2000, g != GROUP_TECH_CAMPUS, g == GROUP_TECH_CAMPUS);
    rule(STUFF_WATER, 2000, true, false);
    rule(STUFF_LOVOLT, 4000, true, false);
    rule(STUFF_WASTE, 2000, false, true);
    if(g == GROUP_TECH_CAMPUS) rule(STUFF_STEEL, 2000, true, false);
    else rule(STUFF_FOOD, 4000, true, false);
  }
}
Construction* CityBusinessGroup::createConstruction(World& w) { return new CityBusiness(w, this); }
bool CityBusinessGroup::can_build(const World& w, Message::ptr& m) const {
  if(!ConstructionGroup::can_build(w, m)) return false;
  if(group == GROUP_TECH_CAMPUS && !w.stats.groupCount[GROUP_UNIVERSITY]) {
    m = CityRequirementMessage::create("Technology campuses require at least one university. Supply it with labor and goods."); return false;
  }
  if(group == GROUP_CAFE || group == GROUP_FOODHALL) {
    int homes = 0;
    for(int i = 0; i < NUM_OF_GROUPS; ++i) if(GROUP_IS_RESIDENCE(i)) homes += w.stats.groupCount[i];
    int required = group == GROUP_CAFE ? 4 : 8;
    if(homes < required * (w.stats.groupCount[group] + 1)) {
      m = CityRequirementMessage::create("Demand is too low: each cafe needs four housing buildings; each food hall needs eight."); return false;
    }
  }
  return true;
}
CityBusiness::CityBusiness(World& w, ConstructionGroup* g) : Construction(w) {
  constructionGroup = g;
  initialize_commodities();
  const auto recipe = productionFor(g->group);
  if(recipe.amount) {
    commodityMaxCons[STUFF_LABOR] = 100*recipe.labor;
    commodityMaxCons[STUFF_WATER] = 100*recipe.water;
    commodityMaxCons[STUFF_LOVOLT] = 100*recipe.power;
    commodityMaxCons[STUFF_STEEL] = 100*recipe.steel;
    commodityMaxProd[STUFF_WASTE] = 100*recipe.waste;
    commodityMaxProd[recipe.output] = 100*recipe.amount;
  } else if(!isWarehouseGroup(g->group)) {
    const bool tech = g->group == GROUP_TECH_CAMPUS, cafe = g->group == GROUP_CAFE;
    commodityMaxCons[STUFF_LABOR] = 100*(cafe ? 4 : 8);
    commodityMaxCons[STUFF_WATER] = 100*(tech ? 2 : cafe ? 3 : 6);
    commodityMaxCons[STUFF_LOVOLT] = 100*(tech ? 20 : cafe ? 1 : 5);
    commodityMaxProd[STUFF_WASTE] = 100*(tech ? 2 : cafe ? 1 : 3);
    if(tech) { commodityMaxCons[STUFF_STEEL] = 400; commodityMaxProd[STUFF_GOODS] = 6000; }
    else { commodityMaxCons[STUFF_FOOD] = 100*(cafe ? 10 : 20); commodityMaxCons[STUFF_GOODS] = 100*(cafe ? 2 : 8); }
  }
}
void CityBusiness::update() {
  int g = constructionGroup->group;
  bool operated = false;
  const auto recipe = productionFor(g);
  if(isWarehouseGroup(g)) {
    world.expense(g == GROUP_LARGE_WAREHOUSE ? 18 : 6, world.stats.expenses.transport);
    operated = freightSurveyed;
  } else if(recipe.amount) {
    const auto& rules = constructionGroup->commodityRuleCount;
    if(commodityCount[STUFF_LABOR] >= recipe.labor && commodityCount[STUFF_WATER] >= recipe.water
        && commodityCount[STUFF_LOVOLT] >= recipe.power && commodityCount[STUFF_STEEL] >= recipe.steel
        && commodityCount[STUFF_WASTE] <= rules[STUFF_WASTE].maxload-recipe.waste
        && commodityCount[recipe.output] <= rules[recipe.output].maxload-recipe.amount) {
      consumeStuff(STUFF_LABOR, recipe.labor);
      consumeStuff(STUFF_WATER, recipe.water);
      if(recipe.power) consumeStuff(STUFF_LOVOLT, recipe.power);
      if(recipe.steel) consumeStuff(STUFF_STEEL, recipe.steel);
      produceStuff(STUFF_WASTE, recipe.waste);
      produceStuff(recipe.output, recipe.amount);
      operated = true;
    }
  } else {
    const bool tech = g == GROUP_TECH_CAMPUS, cafe = g == GROUP_CAFE;
    const int labor = cafe ? 4 : 8, water = tech ? 2 : cafe ? 3 : 6;
    const int power = tech ? 20 : cafe ? 1 : 5, waste = tech ? 2 : cafe ? 1 : 3;
    const int food = cafe ? 10 : 20, goods = cafe ? 2 : 8;
    bool demand = true;
    // A campus is only as fast as its skilled staff: 45% staffed means it works 45% of days.
    if(tech) {
      skillCredit = std::min(skillCredit + std::min(1000, world.evolution.campusSpeed() * (1000 + world.evolution.districtBonusAt(point.x, point.y, 1)) / 1000), 2000);
      if(skillCredit < 1000) demand = false; else skillCredit -= 1000;
    }
    if(!tech) {
      int homes = 0;
      for(int i=0;i<NUM_OF_GROUPS;++i) if(GROUP_IS_RESIDENCE(i)) homes += world.stats.groupCount[i];
      demand = homes >= (cafe ? 4 : 8) * world.stats.groupCount[g];
    }
    if(demand && commodityCount[STUFF_LABOR] >= labor && commodityCount[STUFF_WATER] >= water
      && commodityCount[STUFF_LOVOLT] >= power && commodityCount[STUFF_WASTE] + waste <= 2000
      && (tech ? (world.stats.groupCount[GROUP_UNIVERSITY] && commodityCount[STUFF_STEEL] >= 4 && commodityCount[STUFF_GOODS] + 60 <= 12000)
        : (commodityCount[STUFF_FOOD] >= food && commodityCount[STUFF_GOODS] >= goods))) {
      consumeStuff(STUFF_LABOR,labor); consumeStuff(STUFF_WATER,water); consumeStuff(STUFF_LOVOLT,power);
      produceStuff(STUFF_WASTE,waste);
      if(tech) { consumeStuff(STUFF_STEEL,4); produceStuff(STUFF_GOODS,60); world.tech_level = std::min(MAX_TECH_LEVEL,world.tech_level+2); }
      else { consumeStuff(STUFF_FOOD,food); consumeStuff(STUFF_GOODS,goods); world.income(cafe ? 12 : 22,world.stats.income.goods_tax); }
      operated = true;
    }
  }
  if(operated) ++workingDays;
  if(world.total_time % NUMOF_DAYS_IN_MONTH == NUMOF_DAYS_IN_MONTH-1) {
    busy = workingDays; workingDays = 0; reset_prod_counters();
  }
}
void CityBusiness::report(Mps& mps, bool production) const {
  mps.add_s(constructionGroup->name);
  mps.add_sfp("Operating days / month",busy);
  if(isWarehouseGroup(constructionGroup->group)) {
    const bool connected = std::any_of(neighbors.begin(), neighbors.end(), [](const auto* neighbor) {
      return !neighbor->isDead() && (neighbor->flags & FLAG_IS_TRANSPORT);
    });
    mps.add_s(connected ? "Freight link connected" : "No freight link");
    mps.add_s(constructionGroup->group == GROUP_LARGE_WAREHOUSE ? "Upkeep: $18/day" : "Upkeep: $6/day");
    const bool hasStock = std::any_of(commodityCount.begin(), commodityCount.end(),
      [](int amount) { return amount > 0; });
    if(connected && freightSurveyed && !freightHasBuyer)
      mps.add_s("No connected buyer");
    else if(connected && !hasStock)
      mps.add_s("Waiting for stock");
  }
  else if(constructionGroup->group == GROUP_TECH_CAMPUS) {
    mps.add_s("Needs engineers and technicians, steel, labor, water, power.");
    mps.add_sfp("Staffed (% of full speed)", world.evolution.campusSpeed()/10);
  }
  else if(productionFor(constructionGroup->group).amount)
    mps.add_s(constructionGroup->group == GROUP_NUCLEAR_POWER ? "Needs workers and water" : "Needs labor, steel, water, power");
  else mps.add_s("Needs homes, food, goods, labor, water, electricity.");
  list_commodities(mps,production);
}
void CityBusiness::save(xmlTextWriterPtr w) const {
  Construction::save(w);
  xmlTextWriterWriteElement(w,(xmlStr)"workingDays",xmlFormat<int>(workingDays));
  xmlTextWriterWriteElement(w,(xmlStr)"busy",xmlFormat<int>(busy));
  xmlTextWriterWriteElement(w,(xmlStr)"skillCredit",xmlFormat<int>(skillCredit));
}
bool CityBusiness::loadMember(xmlpp::TextReader& r,unsigned int v) {
  if(r.get_name()=="workingDays") workingDays=xmlParse<int>(r.read_inner_xml());
  else if(r.get_name()=="busy") busy=xmlParse<int>(r.read_inner_xml());
  else if(r.get_name()=="skillCredit") skillCredit=xmlParse<int>(r.read_inner_xml());
  else return Construction::loadMember(r,v);
  return true;
}
