// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
class World;
class Construction;
inline constexpr int MaxSubstationReach = 14;
int substationReach(const Construction&);
void distributeRoadPower(World&);
