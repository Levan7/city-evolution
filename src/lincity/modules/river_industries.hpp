// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "lincity/lintypes.hpp"

void registerRiverIndustries(bool resources);
bool touchesNavigableWater(const World& world, MapPoint origin, int size);
std::vector<MapPoint> shorelineWater(const World& world, MapPoint origin, int size);
std::vector<MapPoint> waterRoute(const World& world, MapPoint start, MapPoint goal, int maxDistance);
void updateRegionalEconomy(World& world);
bool floodDisrupts(const World& world, MapPoint point);
int regionalEvent(int day);
int gridTradePrice(const World& world);
std::string regionalGoals(const World& world);

class RiverIndustry : public Construction {
public:
  RiverIndustry(World& world, ConstructionGroup* group);
  void place(MapPoint point) override;
  void update() override;
  void report(Mps& mps, bool production) const override;
  void save(xmlTextWriterPtr writer) const override;
  bool loadMember(xmlpp::TextReader& reader, unsigned int version) override;
  std::string status() const;
  int workingDays = 0, busy = 0, reserve = 0;
  bool operating = false;
private:
  std::string lastStatus;
};

inline constexpr int batteryCapacity = 36000;
inline constexpr int batteryRate = 2400;
