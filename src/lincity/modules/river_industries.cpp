// SPDX-License-Identifier: GPL-2.0-or-later
#include "river_industries.hpp"

#include <algorithm>
#include <array>
#include <queue>
#include <sstream>
#include <unordered_map>

#include <libxml++/parsers/textreader.h>
#include <libxml/xmlwriter.h>

#include "city_business.hpp"
#include "lincity/civic_services.hpp"
#include "lincity/modules/residence.hpp"
#include "lincity/world.hpp"
#include "lincity-ng/Mps.hpp"
#include "util/xmlutil.hpp"

namespace {
struct Quantity {
  Commodity commodity = STUFF_COUNT;
  int amount = 0;
};
struct IndustrySpec {
  int group;
  const char* resource;
  const char* title;
  int size, cost, technology, range;
  std::array<Quantity, 5> inputs;
  std::array<Quantity, 2> outputs;
  int interval = 1;
};
constexpr IndustrySpec industries[] = {
  {GROUP_FISHERY, "FishingStation", "Fishing station", 2, 3400, 1, 0,
    {{{STUFF_LABOR, 22}}}, {{{STUFF_FOOD, 144}}}, 4},
  {GROUP_WOOD_FARM, "WoodFarm", "Wood farm", 3, 4800, 1, 0,
    {{{STUFF_LABOR, 18}, {STUFF_WATER, 12}}}, {{{STUFF_TIMBER, 200}}}, 10},
  {GROUP_SAWMILL, "LumberSawmill", "Sawmill", 2, 6000, 3, 0,
    {{{STUFF_LABOR, 12}, {STUFF_TIMBER, 16}}}, {{{STUFF_GOODS, 44}, {STUFF_WASTE, 1}}}},
  {GROUP_SOLAR_FARM, "UtilitySolarFarm", "Solar farm", 5, 76000, 42, 0,
    {{{STUFF_LABOR, 34}}}, {{{STUFF_HIVOLT, 7200}}}},
  {GROUP_GRID_BATTERY, "GridBattery", "Grid battery", 2, 21000, 40, 0,
    {}, {}},
  {GROUP_WASTE_ENERGY, "WasteEnergy", "Waste-to-energy plant", 3, 46000, 33, 0,
    {{{STUFF_LABOR, 28}, {STUFF_WASTE, 90}}}, {{{STUFF_HIVOLT, 1300}}}},
  {GROUP_EV_CHARGERS, "EVCharging", "EV charging station", 1, 7800, 52, 0,
    {{{STUFF_LABOR, 4}, {STUFF_LOVOLT, 90}}}, {}},
  {GROUP_DATA_CENTER, "DataCenter", "Data center", 3, 98000, 64, 0,
    {{{STUFF_LABOR, 38}, {STUFF_WATER, 90}, {STUFF_LOVOLT, 850}, {STUFF_COMPUTERS, 8}}},
    {{{STUFF_WASTE, 3}}}},
  {GROUP_DELIVERY_HUB, "DeliveryHub", "Local delivery hub", 1, 4000, 5, 8,
    {}, {}},
  {GROUP_HOSPITAL, "RegionalHospital", "Regional hospital", 3, 40000, 32, 19,
    {{{STUFF_LABOR, 48}, {STUFF_GOODS, 20}, {STUFF_WATER, 30}, {STUFF_LOVOLT, 110}}},
    {{{STUFF_WASTE, 5}}}},
  {GROUP_UNIVERSITY_CAMPUS, "UniversityCampus", "University campus", 4, 55000, 38, 0,
    {{{STUFF_LABOR, 48}, {STUFF_GOODS, 18}, {STUFF_WATER, 20}, {STUFF_LOVOLT, 100}}},
    {{{STUFF_WASTE, 3}}}},
  {GROUP_HOTEL, "CityHotel", "Hotel", 2, 14000, 15, 0,
    {{{STUFF_LABOR, 18}, {STUFF_FOOD, 14}, {STUFF_WATER, 18}, {STUFF_LOVOLT, 55}}},
    {{{STUFF_WASTE, 3}}}},
  {GROUP_CONVENTION_CENTER, "ConventionCenter", "Convention center", 4, 54000, 30, 0,
    {{{STUFF_LABOR, 32}, {STUFF_GOODS, 14}, {STUFF_WATER, 15}, {STUFF_LOVOLT, 110}}},
    {{{STUFF_WASTE, 3}}}},
  {GROUP_MUSEUM, "CityMuseum", "City museum", 3, 21500, 16, 0,
    {{{STUFF_LABOR, 16}, {STUFF_GOODS, 5}, {STUFF_LOVOLT, 35}}}, {{{STUFF_WASTE, 1}}}},
  {GROUP_VINEYARD, "CityVineyard", "Vineyard", 3, 5600, 2, 0,
    {{{STUFF_LABOR, 20}, {STUFF_WATER, 16}}}, {{{STUFF_GRAPES, 112}}}, 7},
  {GROUP_WINERY, "CityWinery", "Winery", 2, 9500, 5, 0,
    {{{STUFF_LABOR, 15}, {STUFF_GRAPES, 14}, {STUFF_WATER, 6}}},
    {{{STUFF_WINE, 10}, {STUFF_WASTE, 1}}}},
  {GROUP_SPARKLING_CELLAR, "SparklingCellar", "Sparkling wine cellar", 2, 18000, 15, 0,
    {{{STUFF_LABOR, 22}, {STUFF_WINE, 20}, {STUFF_GOODS, 4}, {STUFF_LOVOLT, 22}}},
    {{{STUFF_SPARKLING, 16}, {STUFF_WASTE, 2}}}, 3},
  {GROUP_DISTILLERY, "CityDistillery", "Distillery", 2, 12600, 8, 0,
    {{{STUFF_LABOR, 18}, {STUFF_GRAPES, 12}, {STUFF_WATER, 10}, {STUFF_LOVOLT, 24}}},
    {{{STUFF_SPIRITS, 8}, {STUFF_WASTE, 2}}}},
};

const IndustrySpec& specFor(int group) { return industries[group - GROUP_FISHERY]; }
static_assert(std::size(industries) == NUM_OF_GROUPS - GROUP_FISHERY);

class RiverIndustryGroup : public ConstructionGroup {
public:
  explicit RiverIndustryGroup(const IndustrySpec& spec)
    : ConstructionGroup(spec.title, spec.title, false, spec.group, spec.size, blue(20),
        0, spec.cost / 5, 12, spec.cost, spec.technology * MAX_TECH_LEVEL / 100, spec.range) {
    for(auto item : spec.inputs)
      if(item.amount) commodityRuleCount[item.commodity] = {item.amount * 30, true, false};
    for(auto item : spec.outputs)
      if(item.amount) commodityRuleCount[item.commodity] = {item.amount * 12, false, true};
    if(group == GROUP_GRID_BATTERY)
      commodityRuleCount[STUFF_HIVOLT] = {batteryRate, false, false};
  }
  Construction* createConstruction(World& world) override { return new RiverIndustry(world, this); }
  bool can_build(const World& world, Message::ptr& message) const override {
    if(!ConstructionGroup::can_build(world, message)) return false;
    if(group == GROUP_HOSPITAL && !world.stats.groupCount[GROUP_HEALTH]) {
      message = CityRequirementMessage::create("A regional hospital requires a health centre.");
      return false;
    }
    if(group == GROUP_UNIVERSITY_CAMPUS && !world.stats.groupCount[GROUP_UNIVERSITY]) {
      message = CityRequirementMessage::create("Build a university before opening an advanced campus.");
      return false;
    }
    return true;
  }
  bool can_build_here(const World& world, MapPoint point, Message::ptr& message) const override {
    if(!ConstructionGroup::can_build_here(world, point, message)) return false;
    if(group != GROUP_FISHERY || touchesNavigableWater(world, point, size)) return true;
    message = CityRequirementMessage::create("Place the fishing station beside a navigable river or coast.");
    return false;
  }
};

int nearbyDrivers(const Construction& building) {
  int homes = 0;
  for(int y = building.point.y - 7; y <= building.point.y + 7; ++y) {
    auto* previous = static_cast<const Construction*>(nullptr);
    for(int x = building.point.x - 7; x <= building.point.x + 7; ++x) {
      if(!building.world.map.is_visible({x,y})) continue;
      auto* neighbor = building.world.map({x,y})->construction;
      if(neighbor && neighbor != previous && GROUP_IS_RESIDENCE(neighbor->constructionGroup->group)) {
        homes += static_cast<const Residence*>(neighbor)->local_population;
        previous = neighbor;
      }
    }
  }
  return homes;
}

bool hasStreet(const Construction& building, bool paved = false) {
  return std::any_of(building.neighbors.begin(), building.neighbors.end(), [paved](const auto* neighbor) {
    if(neighbor->isDead()) return false;
    const int group = neighbor->constructionGroup->group;
    return group == GROUP_ROAD || group == GROUP_ROAD_BRIDGE ||
      (!paved && (neighbor->flags & FLAG_IS_TRANSPORT));
  });
}

int dailyOutput(const World& world, const IndustrySpec& spec, Quantity item) {
  if(spec.group == GROUP_SOLAR_FARM) return item.amount * world.evolution.solarPercent() / 100;
  if((spec.group == GROUP_FISHERY || spec.group == GROUP_VINEYARD) && regionalEvent(world.total_time) == 2)
    return item.amount * 3 / 4;
  return item.amount;
}
}

void registerRiverIndustries(bool resources) {
  static std::vector<RiverIndustryGroup> groups = [] {
    std::vector<RiverIndustryGroup> result;
    result.reserve(std::size(industries));
    for(const auto& spec : industries)
      if(spec.group != GROUP_DELIVERY_HUB) result.emplace_back(spec);
    return result;
  }();
  for(auto& group : groups) {
    if(resources) ConstructionGroup::addResourceID(specFor(group.group).resource, &group);
    else ConstructionGroup::addConstructionGroup(&group);
  }
}

std::vector<MapPoint> shorelineWater(const World& world, MapPoint p, int size) {
  std::vector<MapPoint> result;
  for(int i = 0; i < size; ++i) {
    for(MapPoint tile : {p.e(size).s(i), p.w().s(i), p.s(size).e(i), p.n().e(i)}) {
      if(world.map.is_visible(tile) && world.map(tile)->is_river())
        result.push_back(tile);
    }
  }
  return result;
}

bool touchesNavigableWater(const World& world, MapPoint point, int size) {
  return !shorelineWater(world, point, size).empty();
}

std::vector<MapPoint> waterRoute(const World& world, MapPoint start, MapPoint goal, int maxDistance) {
  if(!world.map.is_visible(start) || !world.map(start)->is_river()) return {};
  const bool findFurthest = goal == MapPoint(-1,-1);
  std::unordered_map<MapPoint, MapPoint> previous{{start, start}};
  std::queue<std::pair<MapPoint, int>> pending;
  pending.emplace(start, 0);
  MapPoint end = start;
  while(!pending.empty()) {
    const auto [p, distance] = pending.front();
    pending.pop();
    end = p;
    if(p == goal) break;
    if(distance >= maxDistance) continue;
    for(auto next : {p.n(), p.e(), p.s(), p.w()})
      if(world.map.is_visible(next) && world.map(next)->is_river() &&
         previous.emplace(next, p).second) pending.emplace(next, distance + 1);
  }
  if(!findFurthest && end != goal) return {};
  std::vector<MapPoint> route;
  for(auto tile = end; tile != start; tile = previous.at(tile)) route.push_back(tile);
  route.push_back(start);
  std::reverse(route.begin(), route.end());
  return route;
}

RiverIndustry::RiverIndustry(World& world, ConstructionGroup* group) : Construction(world) {
  constructionGroup = group;
  initialize_commodities();
  const auto& spec = specFor(group->group);
  for(auto item : spec.inputs)
    if(item.amount) commodityMaxCons[item.commodity] = item.amount * NUMOF_DAYS_IN_MONTH / spec.interval;
  for(auto item : spec.outputs)
    if(item.amount) commodityMaxProd[item.commodity] = item.amount * NUMOF_DAYS_IN_MONTH / spec.interval;
}

void RiverIndustry::place(MapPoint position) {
  Construction::place(position);
  if(constructionGroup->group != GROUP_FISHERY) return;
  const int size = constructionGroup->size;
  int right = 0, left = 0;
  for(auto water : shorelineWater(world, position, size)) {
    if(water.x >= position.x + size || water.y < position.y) ++right;
    else ++left;
  }
  frameIt->frame = left > right ? 1 : 0;
}

std::string RiverIndustry::status() const {
  if(!lastStatus.empty()) return lastStatus;
  return operating ? "Operating" : "Waiting for first operating day";
}

void RiverIndustry::update() {
  const int group = constructionGroup->group;
  if(world.total_time % NUMOF_DAYS_IN_MONTH == NUMOF_DAYS_IN_MONTH - 1) {
    busy = workingDays;
    workingDays = 0;
    reset_prod_counters();
  }
  if(group == GROUP_GRID_BATTERY) return;
  const auto& spec = specFor(group);
  const auto& rules = constructionGroup->commodityRuleCount;
  const int day = world.total_time;
  auto halt = [&](std::string reason) {
    operating = false;
    lastStatus = std::move(reason);
  };
  if(floodDisrupts(world, point)) return halt("Flooded access");
  if(group == GROUP_FISHERY && !touchesNavigableWater(world, point, spec.size))
    return halt("Needs a navigable shoreline");
  if(!hasStreet(*this, group == GROUP_EV_CHARGERS)) return halt(group == GROUP_EV_CHARGERS ?
    "Connect to a paved road" : "Connect to a road or rail");
  if(group == GROUP_SOLAR_FARM && world.evolution.solarPercent() == 0) return halt("Waiting for sunlight");
  if(group == GROUP_FISHERY && world.map(point)->pollution > 350)
    return halt("Shore too polluted for fishing");
  if((group == GROUP_HOSPITAL || group == GROUP_UNIVERSITY_CAMPUS) && world.total_money < -1999999900)
    return halt("City credit limit reached");
  if(group == GROUP_HOSPITAL && !world.civic.funded(CivicServices::HEALTH, day, point.x + point.y))
    return halt("Needs health funding");
  if(group == GROUP_UNIVERSITY_CAMPUS && !world.civic.funded(CivicServices::EDUCATION, day, point.x + point.y))
    return halt("Needs education funding");
  if(group == GROUP_HOTEL && !world.evolution.visitorDemand) return halt("Waiting for visitors");
  const int drivers = group == GROUP_EV_CHARGERS ? nearbyDrivers(*this) : 0;
  if(group == GROUP_EV_CHARGERS && drivers == 0) return halt("No nearby residents to serve");
  for(auto item : spec.inputs) {
    if(!item.amount) continue;
    int needed = item.amount;
    if(item.commodity == STUFF_WATER && regionalEvent(day) == 1) needed += needed / 2;
    if(commodityCount[item.commodity] < needed) return halt("Needs " + std::string(commodityName(item.commodity)) + " (" +
      std::to_string(commodityCount[item.commodity]) + "/" + std::to_string(needed) + ")");
  }
  for(auto item : spec.outputs) {
    if(item.amount && commodityCount[item.commodity] > rules[item.commodity].maxload - dailyOutput(world, spec, item))
      return halt(std::string(commodityName(item.commodity)) + " has no storage space");
  }
  operating = true;
  lastStatus = {};
  if((day + point.x + point.y) % spec.interval != 0) return;
  for(auto item : spec.inputs) {
    if(!item.amount) continue;
    if(item.commodity == STUFF_WATER && regionalEvent(day) == 1) item.amount += item.amount / 2;
    consumeStuff(item.commodity, item.amount);
  }
  for(auto item : spec.outputs)
    if(item.amount) produceStuff(item.commodity, dailyOutput(world, spec, item));

  ++workingDays;
  if(group == GROUP_DATA_CENTER || group == GROUP_UNIVERSITY_CAMPUS)
    world.tech_level = std::min(MAX_TECH_LEVEL, world.tech_level + (group == GROUP_DATA_CENTER ? 6 : 9));
  if(group == GROUP_WASTE_ENERGY && day % 8 == 0) ++world.map(point)->pollution;
  if(group == GROUP_HOSPITAL) {
    world.expense(world.civic.cost(CivicServices::HEALTH, 34, day, point.x + point.y), world.stats.expenses.health);
    for(int y = point.y - spec.range; y <= point.y + spec.range; ++y)
      for(int x = point.x - spec.range; x <= point.x + spec.range; ++x)
        if(world.map.is_visible({x,y})) world.map({x,y})->flags |= FLAG_HEALTH_COVER_CHECK;
  }
  if(group == GROUP_UNIVERSITY_CAMPUS)
    world.expense(world.civic.cost(CivicServices::EDUCATION, 32, day, point.x + point.y), world.stats.expenses.university);
  if(group == GROUP_HOTEL || group == GROUP_MUSEUM || group == GROUP_CONVENTION_CENTER ||
     group == GROUP_EV_CHARGERS || group == GROUP_DATA_CENTER) {
    const int revenue = group == GROUP_HOTEL ? 26 + 3 * world.evolution.visitorDemand :
      group == GROUP_CONVENTION_CENTER ? 35 + world.evolution.visitorDemand :
      group == GROUP_DATA_CENTER ? 48 : group == GROUP_EV_CHARGERS ? 8 + std::min(32, drivers / 12) : 20;
    world.income(revenue, world.stats.income.service_revenue);
  }
}

void RiverIndustry::report(Mps& mps, bool production) const {
  mps.add_s(constructionGroup->name);
  if(constructionGroup->group == GROUP_GRID_BATTERY) {
    mps.add_sd("Stored high voltage", reserve);
    mps.add_sd("Storage capacity", batteryCapacity);
    mps.add_sd("Daily transfer limit", batteryRate);
    mps.add_s("Charges after city demand");
    mps.add_s("Discharges into local shortages");
    return;
  }
  mps.add_s(status());
  mps.add_sd("Batches last month", busy);
  list_commodities(mps, production);
}

void RiverIndustry::save(xmlTextWriterPtr writer) const {
  Construction::save(writer);
  xmlTextWriterWriteElement(writer, (xmlStr)"batches", xmlFormat<int>(workingDays));
  xmlTextWriterWriteElement(writer, (xmlStr)"previous_batches", xmlFormat<int>(busy));
  xmlTextWriterWriteElement(writer, (xmlStr)"stored_energy", xmlFormat<int>(reserve));
}

bool RiverIndustry::loadMember(xmlpp::TextReader& reader, unsigned int version) {
  const auto name = reader.get_name();
  if(name == "batches") workingDays = std::clamp(xmlParse<int>(reader.read_inner_xml()), 0, 100);
  else if(name == "previous_batches") busy = std::clamp(xmlParse<int>(reader.read_inner_xml()), 0, 100);
  else if(name == "stored_energy") reserve = std::clamp(xmlParse<int>(reader.read_inner_xml()), 0, batteryCapacity);
  else return Construction::loadMember(reader, version);
  return true;
}
