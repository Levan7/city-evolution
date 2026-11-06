// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "MapPoint.hpp"
#include <vector>
class World;
bool isStreetTile(const World&,MapPoint);
// Returns adjacent steps excluding start, with no repeated tile. Empty means no useful trip.
std::vector<MapPoint> planStreetTrip(const World&,MapPoint,unsigned seed);
