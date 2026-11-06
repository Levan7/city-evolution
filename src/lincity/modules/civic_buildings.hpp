// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "residence.hpp"
class CivicBuildingGroup:public ConstructionGroup {
public:CivicBuildingGroup(const std::string&,int group,int size,int cost);Construction* createConstruction(World&)override;
};
extern CivicBuildingGroup policeGroup,glassOfficeGroup,bronzeOfficeGroup,decoOfficeGroup;
extern ResidenceConstructionGroup ivoryTowerGroup,brickTowerGroup,sandTowerGroup;
class CivicBuilding:public Construction {
public: CivicBuilding(World&,ConstructionGroup*);void update()override;void report(Mps&,bool)const override;
};
