// SPDX-License-Identifier: GPL-2.0-or-later
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "lincity/freight.hpp"
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/modules/river_industries.hpp"
#include "lincity/road_power.hpp"

namespace {
void check(bool condition, const char* reason) {
  if(!condition) throw std::runtime_error(reason);
}

Construction& build(World& world, int group, MapPoint place) {
  auto* spec = ConstructionGroup::getConstructionGroup(group);
  check(spec != nullptr, "Missing construction registration");
  world.buildConstruction(*spec, place);
  auto* building = world.map(place)->construction;
  check(building && building->constructionGroup->group == group, "Construction wasn't placed");
  return *building;
}

void water(World& world, MapPoint p) {
  world.map(p)->setTerrain(GROUP_WATER);
  world.map(p)->flags |= FLAG_IS_RIVER;
}

void testRiverAndProduction() {
  auto world = new_barren_city(60);
  world->total_money = 2000000;
  world->tech_level = MAX_TECH_LEVEL;
  auto* fishGroup = ConstructionGroup::getConstructionGroup(GROUP_FISHERY);
  check(!fishGroup->can_build_here(*world,{10,10}), "An inland fishing dock was allowed");
  for(int x=3; x<36; ++x) water(*world, {x,12});
  check(waterRoute(*world,{3,12},{20,12},30).size() == 18, "River route did not follow connected water");
  check(waterRoute(*world,{3,12},{20,13},30).empty(), "River route crossed dry land");

  auto& fish = build(*world,GROUP_FISHERY,{10,10});
  auto& road = build(*world,GROUP_ROAD,{9,10});
  for(int x=10; x<24; ++x) build(*world,GROUP_ROAD,{x,9});
  build(*world,GROUP_ROAD,{9,9});
  auto& hub = build(*world,GROUP_DELIVERY_HUB,{13,10});
  auto& market = build(*world,GROUP_MARKET,{16,10});
  fish.commodityCount[STUFF_LABOR] = 100;
  market.commodityCount[STUFF_LABOR] = 100;
  fish.update();
  check(fish.commodityCount[STUFF_FOOD] > 0, "A staffed fishing dock didn't land Food");
  distributeWarehouseFreight(*world);
  check(market.commodityCount[STUFF_FOOD] > 0 || hub.commodityCount[STUFF_FOOD] > 0,
    "Fish did not enter the market delivery network");
  (void)road;

  for(int x=8; x<=32; ++x) build(*world,GROUP_ROAD,{x,22});
  build(*world,GROUP_WAREHOUSE,{17,20});
  auto& wood = build(*world,GROUP_WOOD_FARM,{8,19});
  auto& mill = build(*world,GROUP_SAWMILL,{13,20});
  auto& vineyard = build(*world,GROUP_VINEYARD,{22,19});
  auto& winery = build(*world,GROUP_WINERY,{27,20});
  auto& cellar = build(*world,GROUP_SPARKLING_CELLAR,{30,20});
  for(auto* building : {&wood, &mill, &vineyard, &winery, &cellar})
    for(auto item : {STUFF_LABOR,STUFF_WATER,STUFF_LOVOLT,STUFF_GOODS})
      if(building->constructionGroup->commodityRuleCount[item].take)
        building->commodityCount[item] = 180;
  for(int day=0; day<42; ++day) {
    world->total_time = day;
    distributeWarehouseFreight(*world);
    for(auto* building : {&wood,&mill,&vineyard,&winery,&cellar}) building->update();
  }
  check(wood.commodityProd[STUFF_TIMBER] > 0 && mill.commodityProd[STUFF_GOODS] > 0, "Timber did not reach the sawmill");
  check(vineyard.commodityProd[STUFF_GRAPES] > 0 && winery.commodityProd[STUFF_WINE] > 0,
    "Grapes did not reach the winery");
  check(cellar.commodityProd[STUFF_SPARKLING] > 0, "Wine did not reach the sparkling cellar");
  check(hub.constructionGroup->commodityRuleCount[STUFF_GRAPES].maxload == 0 &&
        hub.constructionGroup->commodityRuleCount[STUFF_GOODS].maxload > 0,
        "The local hub should handle Food and Goods only");
}

void testRegionalEvents() {
  auto world = new_barren_city(50);
  world->total_money = 1000000;
  world->tech_level = MAX_TECH_LEVEL;
  auto& house = static_cast<Residence&>(build(*world,GROUP_RESIDENCE_LL,{10,10}));
  house.local_population = 40;
  const int normalPower = house.powerNeeded();
  int days[4]{};
  for(int day=400; day<50000; ++day)
    if(!days[regionalEvent(day)]) days[regionalEvent(day)] = day;
  check(days[1] && days[2] && days[3], "Some regional events never occur");
  world->total_time = days[1];
  check(house.powerNeeded() > normalPower && gridTradePrice(*world) == 6,
    "Heat didn't increase household demand and peak prices");
  water(*world,{13,10});
  auto& road = build(*world,GROUP_ROAD,{14,10});
  world->total_time = days[2];
  check(floodDisrupts(*world,{14,10}) && !road.localTradeRule(STUFF_GOODS).take,
    "Flooded shoreline road continued forwarding Goods");
  world->total_time = days[3];
  auto& coal = build(*world,GROUP_COAL_POWER,{22,22});
  check(!coal.localTradeRule(STUFF_HIVOLT).give, "The grid outage didn't cut coal power");
  world->evolution.scenarioGoal = 2;
  for(int i=0; i<4; ++i) water(*world,{25,10+i});
  build(*world,GROUP_PORT,{21,10});
  world->taxable.exportsThisYear = 100000;
  updateRegionalEconomy(*world);
  check(world->evolution.achievementFlags & 2, "Export challenge did not unlock");
}

void testStorageAndSave(const std::filesystem::path& path) {
  auto world = new_barren_city(50);
  world->total_money = 1000000;
  world->tech_level = MAX_TECH_LEVEL;
  auto& solar = build(*world,GROUP_SOLAR_FARM,{8,10});
  auto& battery = static_cast<RiverIndustry&>(build(*world,GROUP_GRID_BATTERY,{15,10}));
  for(int x=8; x<23; ++x) build(*world,GROUP_ROAD,{x,9});
  solar.commodityCount[STUFF_HIVOLT] = 3200;

  distributeRoadPower(*world);
  const auto stored = battery.reserve;
  check(stored > 0 && stored <= batteryRate && !battery.operating, "Battery failed to charge from a connected solar farm");
  check(solar.commodityCount[STUFF_HIVOLT] + stored <= 3200, "Battery created energy");
  solar.commodityCount[STUFF_HIVOLT] = 0;
  auto& home = static_cast<Residence&>(build(*world,GROUP_RESIDENCE_LL,{19,10}));
  home.local_population = 20;
  distributeRoadPower(*world);
  check(battery.reserve < stored && home.commodityCount[STUFF_LOVOLT] > 0, "Home did not receive backup power");
  world->bulldozeArea({18,9});
  home.commodityCount[STUFF_LOVOLT] = 0;
  distributeRoadPower(*world);
  check(home.commodityCount[STUFF_LOVOLT] == 0, "Battery supplied a disconnected road");

  world->evolution.scenarioGoal = 1;
  world->evolution.achievementFlags = 1;
  const int reserve = battery.reserve;
  world->save(path);
  auto loaded = World::load(path);
  const auto* restored = dynamic_cast<const RiverIndustry*>(loaded->map({15,10})->construction);
  check(restored && restored->reserve == reserve, "Stored electricity was lost in save");
  check(loaded->evolution.scenarioGoal == 1 && loaded->evolution.achievementFlags == 1, "Challenge state was lost in save");
  std::filesystem::remove(path);
}
}

int main(int argc, char** argv) {
  try {
    initializeModules();
    testRiverAndProduction();
    testRegionalEvents();
    testStorageAndSave(argc > 1 ? argv[1] : "river-industries-check.scn.gz");
    std::cout << "PASS river routes, food and specialty freight, battery dispatch, regional events and save state\n";
    return 0;
  } catch(const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
