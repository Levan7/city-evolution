// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "MapPoint.hpp"
#include <set>
#include <vector>
#include <array>
class World;
struct TransitRoute {int mode=0; std::vector<MapPoint> path;};
struct TransitSystem {
 enum Mode {BUS=0, RAIL=1, METRO=2, ELEVATED=3};
 std::set<int> tunnels, viaducts, servedHomes;
 std::vector<TransitRoute> routes;
 std::array<int,4> passengers{}, running{}, disconnected{};
 int view=0, assessedMonth=-1, faresMonth=0, upkeepMonth=0;
 bool place(World&,MapPoint,bool elevated);
 bool remove(World&,MapPoint,bool elevated);
 bool covered(MapPoint p,int length) const {return servedHomes.count(p.y*length+p.x);}
 void update(World&,bool account=true);
};
