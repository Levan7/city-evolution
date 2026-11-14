// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "MapPoint.hpp"
#include <set>
#include <vector>
#include <array>
#include <map>
class World;
struct TransitRoute {int mode=0; std::vector<MapPoint> path;};
struct TransitSystem {
 enum Mode {BUS=0, RAIL=1, METRO=2, ELEVATED=3};
 std::set<int> tunnels, viaducts, servedHomes;
 std::vector<TransitRoute> routes;
 std::array<int,4> passengers{}, shoppingTrips{}, running{}, disconnected{}, connected{}, unstaffed{};
 int view=0, assessedMonth=-1, faresMonth=0, upkeepMonth=0;
 static int busReach(int size) { return size >= 2 ? 14 : 8; }
 static int walkingRadius(int mode) { return mode == BUS ? 14 : 10; }
 static int stationMode(int group);
 static std::map<int,int> walkingArea(const World&, MapPoint, int size, int mode);
 int roadTripsAt(MapPoint p, int width) const;
 void deliver(World&);
 bool place(World&,MapPoint,bool elevated);
 bool remove(World&,MapPoint,bool elevated);
 bool covered(MapPoint p,int length) const {return servedHomes.count(p.y*length+p.x);}
 void update(World&,bool account=true,bool initializeSchedule=false);
private:
 struct Trip {
  MapPoint home, destination, originStop, destinationStop;
  int mode, count;
  bool shopping;
  std::vector<MapPoint> path;
 };
 std::vector<Trip> trips;
 std::map<int,int> roadTrips;

};
