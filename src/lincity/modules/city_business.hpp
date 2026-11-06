// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "lincity/lintypes.hpp"
class CityBusinessGroup : public ConstructionGroup {
public:
  CityBusinessGroup(const std::string& name, int group, int size, int cost, int tech);
  Construction* createConstruction(World& world) override;
  bool can_build(const World& world, Message::ptr& message) const override;
};
extern CityBusinessGroup warehouseConstructionGroup, techCampusConstructionGroup,
  cafeConstructionGroup, foodHallConstructionGroup;
class CityBusiness : public Construction {
public:
  CityBusiness(World& world, ConstructionGroup* group);
  void update() override;
  void report(Mps& mps, bool production) const override;
  void save(xmlTextWriterPtr writer) const override;
  bool loadMember(xmlpp::TextReader& reader, unsigned int version) override;
  int workingDays = 0, busy = 0;
  int skillCredit = 0; // accumulates the staffed share of each day; a full day needs 1000
};
