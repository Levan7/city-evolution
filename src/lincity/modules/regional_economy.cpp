// SPDX-License-Identifier: GPL-2.0-or-later
#include "river_industries.hpp"

#include <algorithm>
#include <cstdint>

#include "city_business.hpp"
#include "lincity/world.hpp"

int regionalEvent(int day) {
  const int month = day / NUMOF_DAYS_IN_MONTH;
  const int within = day % NUMOF_DAYS_IN_MONTH;
  if(month < 4 || within < 32 || within >= 50) return 0;
  const uint32_t roll = (uint32_t(month) * 0x9e3779b9u) ^ (uint32_t(month / 9) * 0x85ebca6bu);
  const unsigned code = (roll >> 24) % 19;
  return code < 3 ? int(code) + 1 : 0;
}

int gridTradePrice(const World& world) {
  const int event = regionalEvent(world.total_time);
  if(event == 1 || event == 3) return 6;
  return world.evolution.solarPercent() >= 70 ? 1 : 2;
}

bool floodDisrupts(const World& world, MapPoint p) {
  if(regionalEvent(world.total_time) != 2 || !world.map.is_visible(p) ||
     world.map(p)->is_water()) return false;
  for(MapPoint near : {p.n(), p.e(), p.s(), p.w()})
    if(world.map.is_visible(near) && world.map(near)->is_river() &&
       world.map(p)->ground.altitude <= world.map(near)->ground.altitude + 3) return true;
  return false;
}

void updateRegionalEconomy(World& world) {
  auto& state = world.evolution;
  const int day = world.total_time;
  const int event = regionalEvent(day);
  if(event && regionalEvent(day - 1) != event) {
    static constexpr const char* messages[] = {
      "", "Heat wave. Buildings need more water and the grid is under load.",
      "River flooding. Low-lying waterfront roads and businesses may be interrupted.",
      "Grid outage. Fuel-powered plants have restricted output. Storage can cover shortages."
    };
    state.headline(messages[event]);
  }
  int airports = 0, attractions = 0, hotels = 0;
  long long gridClean = 0, gridDirty = 0;
  for(const auto* building : world.map.constructions) {
    if(building->isDead() || (building->flags & FLAG_EVACUATE)) continue;
    const int type = building->constructionGroup->group;
    if(type == GROUP_AIRPORT || type == GROUP_RACING_CIRCUIT || type == GROUP_AMUSEMENT_PARK) {
      if(static_cast<const CityBusiness*>(building)->operating)
        type == GROUP_AIRPORT ? ++airports : ++attractions;
    } else if(type == GROUP_FOOTBALL) {
      if(building->commodityProd[STUFF_LABOR] < 0) ++attractions;
    } else if(type == GROUP_MUSEUM || type == GROUP_CONVENTION_CENTER || type == GROUP_HOTEL) {
      if(static_cast<const RiverIndustry*>(building)->operating)
        type == GROUP_HOTEL ? ++hotels : ++attractions;
    }
    const int output = std::max({0, building->commodityProdPrev[STUFF_HIVOLT], building->commodityProd[STUFF_HIVOLT]});
    switch(type) {
      case GROUP_COAL_POWER: case GROUP_WASTE_ENERGY: gridDirty += output; break;
      case GROUP_SOLAR_POWER: case GROUP_SOLAR_FARM: case GROUP_WIND_POWER:
      case GROUP_NUCLEAR_POWER: gridClean += output; break;
    }
  }
  state.tourismProviders = attractions;
  state.visitorDemand = attractions ? std::min(40, attractions * 2 + airports * 5) : 0;
  auto earn = [&](int bit, const char* title) {
    if(state.achievementFlags & bit) return;
    state.achievementFlags |= bit;
    state.headline(title);
  };
  if(hotels > 0 && airports > 0 && attractions >= 2) earn(4, "Achievement unlocked: Tourist destination");
  if(state.scenarioGoal == 1 && day - state.scenarioStart <= 40 * NUMOF_DAYS_IN_YEAR &&
     state.homes >= 12 && state.poweredHomes == state.homes && gridClean > 0 && gridDirty == 0)
    earn(1, "Goal reached: Clean grid with 12 powered neighborhoods");
  if(state.scenarioGoal == 2 && world.taxable.exportsThisYear >= 100000 && world.stats.groupCount[GROUP_PORT])
    earn(2, "Goal reached: Export port economy");
}

std::string regionalGoals(const World& world) {
  const auto& state = world.evolution;
  std::string progress;
  if(state.scenarioGoal == 1) {
    progress = state.achievementFlags & 1 ? "Clean grid challenge complete." :
      "Clean grid: power all homes (at least 12) with clean generation by year 40. " +
      std::to_string(state.poweredHomes) + "/" + std::to_string(state.homes) + " powered.";
    if(!(state.achievementFlags & 1) && world.total_time - state.scenarioStart > 40 * NUMOF_DAYS_IN_YEAR)
      progress += " Deadline passed.";
  } else if(state.scenarioGoal == 2) {
    progress = state.achievementFlags & 2 ? "Export port challenge complete." :
      "Export port: ship $100,000 in a single year. $" + std::to_string(world.taxable.exportsThisYear) + " so far.";
  } else {
    progress = "Sandbox city. New World offers Clean grid and Export port challenges.";
  }
  if(state.achievementFlags & 4) progress += " Tourist destination earned.";
  return progress;
}
