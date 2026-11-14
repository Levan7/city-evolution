// SPDX-License-Identifier: GPL-2.0-or-later
#include "road_power.hpp"

#include "modules/all_modules.hpp"
#include "world.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <queue>

namespace {
constexpr int unreachable = std::numeric_limits<int>::max();

template<class Visit>
void footprint(const Construction& building, Visit visit) {
  const auto p = building.point;
  const int size = building.constructionGroup->size;
  for(int k=0; k<size; ++k) {
    visit(p.e(k).n());
    visit(p.e(k).s(size));
    visit(p.s(k).w());
    visit(p.s(k).e(size));
  }
  for(int y=0; y<size; ++y)
    for(int x=0; x<size; ++x) visit(p.e(x).s(y));
}

int conductor(const World& world, MapPoint point) {
  if(!world.map.is_visible(point)) return 0;
  const int group = world.map(point)->getTransportGroup();
  if(group == GROUP_POWER_LINE) return 1;
  if(group == GROUP_RAIL) return 2;
  const int id = point.y * world.map.len() + point.x;
  if(world.transit.tunnels.count(id) || world.transit.viaducts.count(id)) return 2;
  if(group == GROUP_ROAD) return 3;
  return group == GROUP_TRACK ? 5 : 0;
}

void deliver(Construction& source, Commodity from, Construction& target, Commodity to,
             int loss, int need) {
  // One unit of high voltage converts to two units of low voltage.
  const int ratio = from == STUFF_HIVOLT && to == STUFF_LOVOLT ? 2 : 1;
  const int stock = source.commodityCount[from];
  const int amount = std::min(need, int(stock * 100LL * ratio / (100 + loss)));
  if(amount <= 0) return;
  const int sent = int((amount * (100LL + loss) + 100*ratio-1) / (100*ratio));
  source.consumeStuff(from, sent);
  target.commodityCount[to] += amount;
}

bool takes(const Construction& building, Commodity voltage) {
  const auto& rule = building.constructionGroup->commodityRuleCount[voltage];
  return rule.take && !rule.give && rule.maxload > 0;
}

int demand(const Construction& building, Commodity voltage) {
  if(!takes(building, voltage)) return 0;
  if(voltage == STUFF_LOVOLT)
    if(const auto* home = dynamic_cast<const Residence*>(&building))
      return home->local_population > 0 ? POWER_RES_OVERHEAD +
        POWER_USE_PER_PERSON * home->local_population - home->commodityCount[voltage] : 0;
  const auto& rule = building.constructionGroup->commodityRuleCount[voltage];
  const int daily = building.commodityMaxCons[voltage] / NUMOF_DAYS_IN_MONTH;
  return std::min(rule.maxload, std::max(20, daily * 2)) - building.commodityCount[voltage];
}
}

int substationReach(const Construction& station) {
  const int supply = station.commodityCount[STUFF_HIVOLT] * 2 + station.commodityCount[STUFF_LOVOLT];
  return std::clamp((supply + 299) / 300, 0, 14);
}

void distributeRoadPower(World& world) {
  std::vector<Construction*> sources, consumers, substations;
  for(auto* building : world.map.constructions) {
    if(building->isDead() || building->flags & FLAG_EVACUATE) continue;
    const int group = building->constructionGroup->group;
    if(group == GROUP_SUBSTATION) substations.push_back(building);
    else if(takes(*building,STUFF_LOVOLT) || takes(*building,STUFF_HIVOLT))
      consumers.push_back(building);
    if(group != GROUP_SUBSTATION && group != GROUP_POWER_LINE && !(building->flags & FLAG_IS_TRANSPORT)) {
      const auto& rules = building->constructionGroup->commodityRuleCount;
      if((rules[STUFF_HIVOLT].give && building->commodityCount[STUFF_HIVOLT] > 0) ||
         (rules[STUFF_LOVOLT].give && building->commodityCount[STUFF_LOVOLT] > 0))
        sources.push_back(building);
    }
  }
  if(!consumers.empty())
    std::rotate(consumers.begin(),consumers.begin()+world.total_time%consumers.size(),consumers.end());

  const int side = world.map.len();
  auto index = [side](MapPoint p) { return p.y*side+p.x; };
  std::vector<int> weights(side*side,0);
  std::map<Construction*,int> stationInput;
  if(!sources.empty() || !substations.empty())
    for(int y=1; y<side-1; ++y)
      for(int x=1; x<side-1; ++x) weights[y*side+x] = conductor(world,{x,y});

  auto distribute = [&](Construction& source, Commodity voltage, bool local) {
    if(source.commodityCount[voltage] <= 0) return;
    using Node = std::pair<int,int>;
    std::priority_queue<Node,std::vector<Node>,std::greater<Node>> pending;
    std::vector<int> distance(side*side,unreachable);
    footprint(source,[&](MapPoint point) {
      if(!world.map.is_visible(point)) return;
      const int id = index(point);
      if(weights[id] && distance[id] == unreachable) {
        distance[id] = 0;
        pending.emplace(0,id);
      }
    });
    while(!pending.empty()) {
      const auto [cost,id] = pending.top();
      pending.pop();
      if(cost != distance[id]) continue;
      const MapPoint p(id%side,id/side);
      for(const auto next : {p.n(),p.e(),p.s(),p.w()}) {
        const int ni = index(next);
        if(weights[ni] && cost+weights[ni] < distance[ni]) {
          distance[ni] = cost+weights[ni];
          pending.emplace(distance[ni],ni);
        }
      }
    }
    auto lossTo = [&](const Construction& target) {
      int loss = unreachable;
      footprint(target,[&](MapPoint point) {
        if(world.map.is_visible(point)) loss = std::min(loss,distance[index(point)]);
      });
      return loss;
    };
    if(voltage == STUFF_HIVOLT) {
      // Transformer input is limited by route quality even after long accumulation.
      for(auto* station : substations) {
        const int loss = lossTo(*station);
        if(loss == unreachable) continue;
        const int quota = int(1800LL*100/(100+loss));
        const int cap = std::min(4500-stationInput[station],
          station->constructionGroup->commodityRuleCount[voltage].maxload-station->commodityCount[voltage]);
        const int before = station->commodityCount[voltage];
        deliver(source,voltage,*station,voltage,loss,std::min(quota,cap));
        stationInput[station] += station->commodityCount[voltage]-before;
      }
    }
    const int reach = local ? substationReach(source) : 0;
    for(auto* target : consumers) {
      if(source.commodityCount[voltage] <= 0) break;
      int loss = lossTo(*target);
      if(reach) {
        const auto p = target->point;
        const int size = target->constructionGroup->size;
        const int dx = std::max({0, source.point.x-p.x-size+1,p.x-source.point.x});
        const int dy = std::max({0, source.point.y-p.y-size+1,p.y-source.point.y});
        if(std::max(dx,dy) <= reach) loss = std::min(loss,2*(dx+dy));
      }
      if(loss == unreachable) continue;
      const auto output = voltage == STUFF_HIVOLT && takes(*target,STUFF_HIVOLT) ? STUFF_HIVOLT : STUFF_LOVOLT;
      const int need = voltage == STUFF_LOVOLT && takes(*target,STUFF_HIVOLT) &&
        !takes(*target,STUFF_LOVOLT) ? 0 : demand(*target,output);
      deliver(source,voltage,*target,output,loss,need);
    }
  };
  for(auto* source : sources) {
    distribute(*source,STUFF_HIVOLT,false);
    distribute(*source,STUFF_LOVOLT,false);
  }
  for(auto* station : substations) distribute(*station,STUFF_LOVOLT,true);
}
