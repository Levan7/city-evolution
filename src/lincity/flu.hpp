// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <map>
#include <string>
class World;
struct FluSystem {
 struct Household {std::array<int,2> exposed{};std::array<int,6> infectious{};std::array<int,36> immune{};};
 bool enabled=true;int lastDay=-1,active=0,latent=0,recovered=0,lastNotice=-1200;unsigned rng=4817263;
 std::map<int,Household> households;std::string pendingLoad;
 int infectiousAt(int id) const;
 int seed(const World&,int id,int count);
 int treat(int id,int count);
 void update(World&);void clear();void recount();
 std::string save()const;void load(const std::string&,int side);
 unsigned random(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
};
