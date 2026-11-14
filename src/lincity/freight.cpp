// SPDX-License-Identifier: GPL-2.0-or-later
#include "freight.hpp"

#include "world.hpp"
#include "transport.hpp"
#include "modules/all_modules.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <queue>
#include <set>

namespace {
constexpr std::array cargo{STUFF_FOOD, STUFF_COAL, STUFF_GOODS, STUFF_ORE, STUFF_STEEL};

bool rail(int group) { return group == GROUP_RAIL || group == GROUP_RAIL_BRIDGE; }

bool available(const World& world, const Construction& building) {
  return !building.isDead() && !(building.flags & FLAG_EVACUATE)
    && !world.civic.burningAt(building.point.y * world.map.len() + building.point.x);
}

template<class Visit>
void perimeter(MapPoint p, int size, Visit visit) {
  for(int i=0; i<size; ++i) {
    visit(p.n().e(i));
    visit(p.w().s(i));
    visit(p.s(size).e(i));
    visit(p.e(size).s(i));
  }
}
}

void distributeWarehouseFreight(World& world) {
  if(!world.stats.groupCount[GROUP_WAREHOUSE] && !world.stats.groupCount[GROUP_LARGE_WAREHOUSE]) return;
  const int side = world.map.len();
  auto index = [side](MapPoint p) { return p.y * side + p.x; };
  auto at = [&world](MapPoint p) -> Construction* {
    return world.map.is_visible(p) ? world.map(p)->reportingConstruction : nullptr;
  };
  // A tile has one daily budget per commodity shared by all warehouse deliveries.
  std::map<int,std::array<int,cargo.size()>> used;
  for(auto* building : world.map.constructions) {
    if(!isWarehouseGroup(building->constructionGroup->group) || !available(world,*building))
      continue;
    auto* warehouse = static_cast<CityBusiness*>(building);
    warehouse->freightSurveyed = false;
    warehouse->freightHasBuyer = false;

    struct Link { Construction* tile; int previous, distance; };
    std::map<int,Link> network;
    std::vector<std::pair<Construction*,int>> endpoints;
    std::set<int> seen;
    std::queue<int> pending;
    const int radius = warehouse->constructionGroup->range;
    const int reach = warehouse->constructionGroup->group == GROUP_LARGE_WAREHOUSE ? 96 : WarehouseFreightRange;
    auto enqueue = [&](MapPoint p, int previous, int distance, Construction* prior) {
      auto* tile = at(p);
      if(!tile || !(tile->flags & FLAG_IS_TRANSPORT) || tile->isDead() ||
         (prior && rail(tile->constructionGroup->group) != rail(prior->constructionGroup->group))) return;
      const int id = index(p);
      if(network.emplace(id,Link{tile,previous,distance}).second) pending.push(id);
    };
    perimeter(warehouse->point,warehouse->constructionGroup->size,
      [&](MapPoint p){ enqueue(p,-1,1,nullptr); });
    if(pending.empty()) continue;
    warehouse->freightSurveyed = true;
    while(!pending.empty()) {
      const int id = pending.front(); pending.pop();
      const auto& link = network.at(id);
      MapPoint p(id%side,id/side);
      for(auto next : {p.n(),p.e(),p.s(),p.w()}) {
        auto* endpoint = at(next);
        if(!endpoint) continue;
        if(endpoint->flags & FLAG_IS_TRANSPORT) {
          enqueue(next,id,link.distance+1,link.tile);
          continue;
        }
        if(endpoint == warehouse || !available(world,*endpoint)) continue;
        if(link.distance > reach && !isWarehouseGroup(endpoint->constructionGroup->group)) continue;
        if(seen.insert(index(endpoint->point)).second) endpoints.emplace_back(endpoint,id);
      }
    }

    // Local cart delivery uses the service square. Longer transfers use road or rail.
    const auto origin = warehouse->point;
    const int size = warehouse->constructionGroup->size;
    for(int y=std::max(1,origin.y-radius); y<std::min(side-1,origin.y+size+radius); ++y)
      for(int x=std::max(1,origin.x-radius); x<std::min(side-1,origin.x+size+radius); ++x) {
        auto* endpoint = at({x,y});
        if(!endpoint || endpoint == warehouse || endpoint->flags & FLAG_IS_TRANSPORT ||
           isWarehouseGroup(endpoint->constructionGroup->group) || !available(world,*endpoint)) continue;
        if(seen.insert(index(endpoint->point)).second) endpoints.emplace_back(endpoint,-1);
      }

    auto transfer = [&](Construction& from, Construction& to, int route, size_t i, int requested) {
      const auto stuff = cargo[i];
      if(!from.localTradeRule(stuff).give || !to.localTradeRule(stuff).take) return;
      int amount = std::min({requested,from.commodityCount[stuff],
        to.localTradeRule(stuff).maxload-to.commodityCount[stuff]});
      if(route < 0) amount = std::min(amount, 40);
      for(int id=route; amount>0 && id>=0; id=network.at(id).previous) {
        const auto& tile = *network.at(id).tile;
        const int capacity = tile.localTradeRule(stuff).maxload / TRANSPORT_RATE;
        amount = std::min(amount,capacity-used[id][i]);
      }
      if(amount<=0) return;
      for(int id=route; id>=0; id=network.at(id).previous) used[id][i] += amount;
      from.commodityCount[stuff] -= amount;
      to.commodityCount[stuff] += amount;
      if(to.constructionGroup->group != GROUP_MARKET && !isWarehouseGroup(to.constructionGroup->group)) {
        if(stuff==STUFF_GOODS) world.taxable.goods += amount;
        if(stuff==STUFF_COAL) world.taxable.coal += amount;
      }
    };

    for(size_t i=0;i<cargo.size();++i) {
      auto stuff = cargo[i];
      for(auto [source,route] : endpoints) {
        const auto& supply = source->localTradeRule(stuff);
        const int threshold = supply.take ? supply.maxload/2 : 0;
        if(source->constructionGroup->group == GROUP_MARKET || isWarehouseGroup(source->constructionGroup->group)) continue;
        transfer(*source,*warehouse,route,i,source->commodityCount[stuff]-threshold);
      }
      for(auto [buyer,route] : endpoints) {
        const int group = buyer->constructionGroup->group;
        if(GROUP_IS_RESIDENCE(group)) continue;
        const auto& demand = buyer->localTradeRule(stuff);
        if(isWarehouseGroup(group)) {
          const int surplus = warehouse->commodityCount[stuff] - warehouse->localTradeRule(stuff).maxload/2;
          transfer(*warehouse,*buyer,route,i,std::min(surplus,demand.maxload/2-buyer->commodityCount[stuff]));
          continue;
        }
        if(demand.give && group!=GROUP_MARKET && group!=GROUP_PORT) continue;
        if(group==GROUP_PORT && !static_cast<Port*>(buyer)->commodityRuleCount[stuff].give) continue;
        if(demand.take) warehouse->freightHasBuyer = true;
        const int target = group==GROUP_MARKET ? demand.maxload/4 : demand.maxload/2;
        transfer(*warehouse,*buyer,route,i,target-buyer->commodityCount[stuff]);
      }
    }
  }
}
