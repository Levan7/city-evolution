// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "lincity/lintypes.hpp"
class TransitGroup : public ConstructionGroup {
public:
 bool layer=false;
 TransitGroup(const char*,int,int,int,int,bool=false);
 Construction* createConstruction(World&) override;
 bool can_build_here(const World&,MapPoint,Message::ptr&) const override;
};
extern TransitGroup busStopGroup,busTerminalGroup,railStationGroup,metroStationGroup,elevatedStationGroup,metroTrackGroup,elevatedTrackGroup;
class TransitFacility : public Construction {
public:
 int workingDays=0,busy=0,riders=0;bool active=false;
 TransitFacility(World&,ConstructionGroup*);
 int mode() const;
 void update() override;
 void report(Mps&,bool) const override;
 void save(xmlTextWriterPtr) const override;
 bool loadMember(xmlpp::TextReader&,unsigned int) override;
};
