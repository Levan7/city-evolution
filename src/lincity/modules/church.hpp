// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "lincity/lintypes.hpp"
class ChurchGroup:public ConstructionGroup {
public:ChurchGroup();Construction* createConstruction(World&) override;
};
extern ChurchGroup churchConstructionGroup;
class GeorgianChurch:public Construction {
public:
 static constexpr int Radius=8,Morale=8,DailyCost=8;
 int workingDays=0,busy=0,totalLessons=0;bool active=false;
 explicit GeorgianChurch(World&);
 void update() override;
 void report(Mps&,bool) const override;
 void save(xmlTextWriterPtr) const override;
 bool loadMember(xmlpp::TextReader&,unsigned int) override;
};
bool churchServes(const World&,MapPoint);
