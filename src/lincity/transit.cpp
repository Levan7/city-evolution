// SPDX-License-Identifier: GPL-2.0-or-later
#include "transit.hpp"
#include "world.hpp"
#include "transport.hpp"
#include "modules/public_transport.hpp"
#include "modules/all_modules.hpp"
#include <queue>
#include <algorithm>
#include <map>
namespace {
bool hasLink(const World& w, MapPoint p, int mode) {
 if(!w.map.is_visible(p)) return false;
 int group = w.map(p)->getGroup(), id = p.y*w.map.len()+p.x;
 if(mode == TransitSystem::BUS)
  return group == GROUP_ROAD || group == GROUP_TRACK ||
    group == GROUP_ROAD_BRIDGE || group == GROUP_TRACK_BRIDGE;
 if(mode == TransitSystem::RAIL) return group == GROUP_RAIL || group == GROUP_RAIL_BRIDGE;
 return (mode == TransitSystem::METRO ? w.transit.tunnels : w.transit.viaducts).count(id);
}

int distanceTo(const std::map<int,int>& area, int length, const Construction& building) {
 int best = 1000000;
 for(int y=0; y<building.constructionGroup->size; ++y)
  for(int x=0; x<building.constructionGroup->size; ++x) {
   auto it = area.find((building.point.y+y)*length+building.point.x+x);
   if(it != area.end()) best = std::min(best,it->second);
  }
 return best;
}
}

int TransitSystem::stationMode(int group) {
 switch(group) {
 case GROUP_BUS_STOP: case GROUP_BUS_TERMINAL: return BUS;
 case GROUP_RAIL_STATION: return RAIL;
 case GROUP_METRO_STATION: return METRO;
 case GROUP_ELEVATED_STATION: return ELEVATED;
 default: return -1;
 }
}

std::map<int,int> TransitSystem::walkingArea(const World& w, MapPoint point, int size, int mode) {
 const int length = w.map.len(), reach = mode == BUS ? busReach(size) : walkingRadius(mode);
 std::map<int,int> distances;
 std::queue<MapPoint> pending;
 for(int y=0; y<size; ++y) for(int x=0; x<size; ++x) {
  auto p = point.e(x).s(y);
  if(!w.map.is_visible(p)) continue;
  distances.emplace(p.y*length+p.x,0);
  pending.push(p);
 }
 while(!pending.empty()) {
  auto p = pending.front(); pending.pop();
  const int distance = distances.at(p.y*length+p.x);
  for(auto next : {p.n(),p.e(),p.s(),p.w()}) {
   if(!w.map.is_visible(next) || next.x < point.x-reach || next.y < point.y-reach ||
      next.x >= point.x+size+reach || next.y >= point.y+size+reach) continue;
   auto* tile = w.map(next);
   auto* building = tile->reportingConstruction;
   const bool transport = building && (building->flags & FLAG_IS_TRANSPORT);
   if(tile->is_water() && !transport) continue;
   if(!distances.emplace(next.y*length+next.x,distance+1).second) continue;
   // A building can be reached from its edge, but pedestrians cannot cross it.
   if(!building || transport) pending.push(next);
  }
 }
 for(auto& [id, steps] : distances) {
   const int x = id % length, y = id / length;
   steps = std::max({0, point.x-x, x-point.x-size+1, point.y-y, y-point.y-size+1});
  }
 return distances;
}

int TransitSystem::roadTripsAt(MapPoint point, int width) const {
 const auto found = roadTrips.find(point.y*width+point.x);
 return found == roadTrips.end() ? 0 : found->second;
}

void TransitSystem::deliver(World& w) {
 auto at = [&w](MapPoint p) -> Construction* {
  if(!w.map.is_visible(p)) return nullptr;
  auto* building = w.map(p)->construction;
  return building && !building->isDead() && !(building->flags & FLAG_EVACUATE) &&
    !w.civic.burningAt(p.y*w.map.len()+p.x) ? building : nullptr;
 };
 if(w.total_time <= 0 || trips.empty()) return;
 const int day = (w.total_time-1) % NUMOF_DAYS_IN_MONTH;
 for(const auto& trip : trips) {
  const int quota = trip.count*(day+1)/NUMOF_DAYS_IN_MONTH - trip.count*day/NUMOF_DAYS_IN_MONTH;
  if(quota <= 0 || trip.path.empty()) continue;
  auto* home = dynamic_cast<Residence*>(at(trip.home));
  auto* from = dynamic_cast<TransitFacility*>(at(trip.originStop));
  auto* to = dynamic_cast<TransitFacility*>(at(trip.destinationStop));
  auto* destination = at(trip.destination);
  if(!home || home->local_population<=0 || !from || !to || !from->active || !to->active ||
     !destination) continue;
  auto pathOpen = [&](const std::vector<MapPoint>& path,int mode) {
   return std::all_of(path.begin(),path.end(),[&](MapPoint p){return hasLink(w,p,mode);});
  };
  if(!pathOpen(trip.path,trip.mode)) continue;
  if(trip.transferMode>=0) {
   auto* exit=dynamic_cast<TransitFacility*>(at(trip.interchangeFrom));
   auto* entry=dynamic_cast<TransitFacility*>(at(trip.interchangeTo));
   if(!exit || !entry || !exit->active || !entry->active ||
      !pathOpen(trip.transferPath,trip.transferMode)) continue;
  }
   auto touchesCatchment = [&](const Construction& stop,const Construction& building,int mode) {
   const int size = stop.constructionGroup->size;
   const int reach = mode == BUS ? busReach(size) : walkingRadius(mode);
   const int other = building.constructionGroup->size;
   return building.point.x < stop.point.x+size+reach && building.point.x+other > stop.point.x-reach &&
          building.point.y < stop.point.y+size+reach && building.point.y+other > stop.point.y-reach;
  };
  if(!touchesCatchment(*from,*home,trip.mode) ||
     !touchesCatchment(*to,*destination,trip.transferMode<0 ? trip.mode : trip.transferMode)) continue;
  if(trip.shopping) {
   auto* market = dynamic_cast<Market*>(destination);
   if(!market || market->commodityCount[STUFF_LABOR]<market->labor) continue;
   for(auto stuff : {STUFF_FOOD, STUFF_GOODS}) {
    if(!market->commodityRuleCount[stuff].give) continue;
    const int desired = stuff==STUFF_FOOD ? home->local_population*2 : home->local_population;
    const int target = std::min(desired,home->constructionGroup->commodityRuleCount[stuff].maxload);
    const int amount = std::max(0,std::min({quota*(stuff==STUFF_FOOD ? 12 : 2),
      target-home->commodityCount[stuff],market->commodityCount[stuff]}));
    market->commodityCount[stuff] -= amount;
    home->commodityCount[stuff] += amount;
    if(stuff==STUFF_GOODS) w.taxable.goods += amount;
   }
  } else {
   const auto& labor = destination->constructionGroup->commodityRuleCount[STUFF_LABOR];
   if(!labor.take) continue;
   const int amount = std::max(0,std::min({quota*8,home->commodityCount[STUFF_LABOR],
     labor.maxload/2-destination->commodityCount[STUFF_LABOR]}));
   home->commodityCount[STUFF_LABOR] -= amount;
   destination->commodityCount[STUFF_LABOR] += amount;
   if(destination->constructionGroup->group != GROUP_MARKET) w.taxable.labor += amount;
  }
 }
}
bool TransitSystem::place(World& w,MapPoint p,bool elevated){
 auto& group=elevated?elevatedTrackGroup:metroTrackGroup;Message::ptr msg;
 if(!group.can_build(w,msg)||!group.can_build_here(w,p,msg))return false;
 auto& set=elevated?viaducts:tunnels;w.expense(group.getCosts(w),w.stats.expenses.construction,!group.no_credit);set.insert(p.y*w.map.len()+p.x);update(w,false);return true;
}
bool TransitSystem::remove(World& w,MapPoint p,bool elevated){auto& set=elevated?viaducts:tunnels;if(!set.erase(p.y*w.map.len()+p.x))return false;w.expense(100,w.stats.expenses.construction);update(w,false);return true;}
void TransitSystem::update(World& w,bool account,bool initializeSchedule){
 const int month=w.total_time/NUMOF_DAYS_IN_MONTH;if(account&&assessedMonth==month)return;
 int length=w.map.len(),cells=length*length;routes.clear();roadTrips.clear();servedHomes.clear();passengers.fill(0);shoppingTrips.fill(0);running.fill(0);disconnected.fill(0);connected.fill(0);unstaffed.fill(0);faresMonth=0;
 struct Hub {int mode,remaining,riders;std::vector<TransitFacility*> stops;std::vector<Construction*> jobs;TransitRoute route;std::map<std::pair<int,int>,std::vector<MapPoint>> transferPaths;};
 std::vector<Hub> hubs;
 std::vector<Trip> planned;
 std::vector<TransitFacility*> stations;std::vector<Construction*> homes,jobs;
 std::vector<Market*> markets;
 std::array<std::map<TransitFacility*,std::map<int,int>>,4> walking;
 std::set<int> shoppingHomes;
 for(auto* c:w.map.constructions){if(c->isDead())continue;int g=c->constructionGroup->group;
  if(g>=GROUP_BUS_STOP&&g<=GROUP_ELEVATED_STATION){auto* s=static_cast<TransitFacility*>(c);stations.push_back(s);s->riders=0;s->requestedTrips=0;s->endpointTrips=0;}
  if(g==GROUP_MARKET) markets.push_back(static_cast<Market*>(c));
  else if(GROUP_IS_RESIDENCE(g))homes.push_back(c);
  else if(stationMode(g)<0 && !w.map(c->point)->is_transport() && g!=GROUP_POWER_LINE && !isWarehouseGroup(g) && c->constructionGroup->commodityRuleCount[STUFF_LABOR].take&&c->constructionGroup->commodityRuleCount[STUFF_LABOR].maxload)jobs.push_back(c);
 }
 auto distance=[](MapPoint a,MapPoint b){return std::abs(a.x-b.x)+std::abs(a.y-b.y);};
 for(int mode:{METRO,ELEVATED,RAIL,BUS}){
  auto supports = [mode](const TransitFacility* station) {
   return station->mode()==mode || (mode!=BUS && station->mode()!=BUS);
  };
  int count=0;for(auto* station:stations)if(supports(station))++count;
  if(count<2){disconnected[mode]=count;continue;}
  std::vector<unsigned char> links(cells);std::vector<int> comp(cells,-1);
  for(int y=1;y<length-1;++y)for(int x=1;x<length-1;++x){int id=y*length+x;auto* tile=w.map(MapPoint(x,y));int g=tile->reportingConstruction ? tile->reportingConstruction->constructionGroup->group : tile->group;
   links[id]=mode==BUS?(g==GROUP_ROAD||g==GROUP_ROAD_BRIDGE||g==GROUP_TRACK||g==GROUP_TRACK_BRIDGE):mode==RAIL?(g==GROUP_RAIL||g==GROUP_RAIL_BRIDGE):mode==METRO?tunnels.count(id):viaducts.count(id);
  }
  // Multi-tile stations connect to every orthogonally touching track tile.
  std::map<TransitFacility*,std::vector<int>> touches;
  for(auto* s:stations)if(supports(s)){
   auto& ts=touches[s];int sz=s->constructionGroup->size;
   for(int j=-1;j<=sz;++j)for(int i=-1;i<=sz;++i){if(i!=-1&&i!=sz&&j!=-1&&j!=sz)continue;if((i==-1||i==sz)&&(j==-1||j==sz))continue;
    MapPoint p=s->point.e(i).s(j);if(w.map.is_visible(p)&&links[p.y*length+p.x])ts.push_back(p.y*length+p.x);
   }
   // Underground/elevated rails can run directly under the entrance footprint.
   if(mode>=METRO)for(int j=0;j<sz;++j)for(int i=0;i<sz;++i){int id=(s->point.y+j)*length+s->point.x+i;if(links[id])ts.push_back(id);}
  }
  int component=0;
  for(int start=0;start<cells;++start)if(links[start]&&comp[start]<0){std::queue<int> q;q.push(start);comp[start]=component;
   while(!q.empty()){int id=q.front();q.pop();int x=id%length,y=id/length;for(int n:{id-1,id+1,id-length,id+length})if(n>=0&&n<cells&&std::abs(n%length-x)+std::abs(n/length-y)==1&&links[n]&&comp[n]<0){comp[n]=component;q.push(n);}}
   ++component;
  }
  std::map<int,std::vector<TransitFacility*>> groups;
  for(auto& entry:touches){auto* s=entry.first;
   if(entry.second.empty()){if(s->mode()==mode) ++disconnected[mode];continue;}
   walking[mode].emplace(s,walkingArea(w,s->point,s->constructionGroup->size,mode));
   // A station may serve several components, but it never joins unrelated tracks.
   std::set<int> unique;for(int id:entry.second)unique.insert(comp[id]);for(int c:unique)groups[c].push_back(s);
  }
  for(auto& entry:groups){auto& ss=entry.second;if(ss.size()<2){if(ss.front()->mode()==mode) ++disconnected[mode];continue;}
   std::sort(ss.begin(),ss.end(),[](auto* a,auto* b){return a->point.y!=b->point.y?a->point.y<b->point.y:a->point.x<b->point.x;});
   connected[mode]+=ss.size();for(auto* stop:ss)if(!stop->active)++unstaffed[mode];
   int radius=walkingRadius(mode);
   std::vector<int> jobWalk(ss.size(),radius+1);
   std::vector<Construction*> workplace(ss.size()), shop(ss.size());
   std::vector<int> shopWalk(ss.size(),radius+1);
   for(size_t i=0;i<ss.size();++i) {
    for(auto* job:jobs) {
     const int walk = distanceTo(walking[mode].at(ss[i]),length,*job);
     const auto& labor = job->constructionGroup->commodityRuleCount[STUFF_LABOR];
     if(walk<=radius && job->commodityCount[STUFF_LABOR]<labor.maxload/2 && walk<jobWalk[i]) {
      jobWalk[i]=walk; workplace[i]=job;
     }
    }
    for(auto* market:markets) {
     const int walk = distanceTo(walking[mode].at(ss[i]),length,*market);
     const bool stocked = (market->commodityRuleCount[STUFF_FOOD].give && market->commodityCount[STUFF_FOOD]>0)
       || (market->commodityRuleCount[STUFF_GOODS].give && market->commodityCount[STUFF_GOODS]>0);
     if(stocked && market->commodityCount[STUFF_LABOR]>=market->labor && walk<shopWalk[i]) {shopWalk[i]=walk;shop[i]=market;}
    }
   }
   std::vector<std::vector<int>> ride(ss.size(),std::vector<int>(ss.size(),cells));
   for(size_t i=0;i<ss.size();++i){std::vector<int> d(cells,-1);std::queue<int> q;for(int v:touches[ss[i]])if(comp[v]==entry.first){d[v]=0;q.push(v);}while(!q.empty()){int v=q.front();q.pop();for(int z:{v-1,v+1,v-length,v+length})if(z>=0&&z<cells&&std::abs(z%length-v%length)+std::abs(z/length-v/length)==1&&links[z]&&d[z]<0){d[z]=d[v]+1;q.push(z);}}for(size_t j=0;j<ss.size();++j)for(int v:touches[ss[j]])if(d[v]>=0)ride[i][j]=std::min(ride[i][j],d[v]);}
   int groupRiders=0,capacity=0;for(auto* stop:ss) capacity+=stop->monthlyCapacity();
   auto pathBetween = [&](int origin, int destination) {
    std::vector<int> previous(cells,-1);
    std::queue<int> pending;
    for(int tile:touches[ss[origin]]) if(comp[tile]==entry.first) {
     previous[tile]=tile;pending.push(tile);
    }
    std::set<int> ends;
    for(int tile:touches[ss[destination]]) if(comp[tile]==entry.first) ends.insert(tile);
    std::vector<MapPoint> path;
    while(!pending.empty()) {
     int tile=pending.front();pending.pop();
     if(ends.count(tile)) {
      for(;;tile=previous[tile]) {
       path.emplace_back(tile%length,tile/length);
       if(previous[tile]==tile) break;
      }
      std::reverse(path.begin(),path.end());
      break;
     }
     for(int next:{tile-1,tile+1,tile-length,tile+length})
      if(next>=0 && next<cells && distance({tile%length,tile/length},{next%length,next/length})==1 &&
         comp[next]==entry.first && previous[next]<0) {
       previous[next]=tile;pending.push(next);
      }
    }
    return path;
   };
   std::map<std::pair<int,int>,std::vector<MapPoint>> paths;
   for(bool shopping:{false,true}) for(auto* h:homes) {
    const int id=h->point.y*length+h->point.x;
    if(shopping ? shoppingHomes.count(id) : servedHomes.count(id)) continue;
    auto* residence=static_cast<Residence*>(h);
    const int people=std::max(0,residence->local_population);
    if(!people || (shopping && h->commodityCount[STUFF_FOOD]>=people*2 && h->commodityCount[STUFF_GOODS]>=people)) continue;
    const auto& destinations = shopping ? shop : workplace;
    const auto& walks = shopping ? shopWalk : jobWalk;
    int origin=-1,destination=-1,journey=2*cells;
    for(size_t i=0;i<ss.size();++i) {
     const int walk=distanceTo(walking[mode].at(ss[i]),length,*h);
     if(walk>radius||!ss[i]->active) continue;
     for(size_t j=0;j<ss.size();++j) {
      if(j==i||!destinations[j]||!ss[j]->active||ride[i][j]>=cells) continue;
      // Use local access first when the destination is already on the origin's walk network.
      if(distanceTo(walking[mode].at(ss[i]),length,*destinations[j]) <= (mode == BUS ? radius : 12)) continue;
      const int cost=3*(walk+walks[j])+ride[i][j];
      if(cost<journey){journey=cost;origin=i;destination=j;}
     }
    }
    if(origin<0) continue;
    const int days=std::min(ss[origin]->busy,ss[destination]->busy);
    const int demand=people*(shopping ? 1 : 2)*60/(60+journey);
    const int requested = demand * std::clamp(days,0,100) /
      (100 + 20 * std::max(0, fares[mode] - (mode == BUS ? 1 : 2)));
    int riders=std::min(capacity-groupRiders,requested);
    for(auto* stop:{ss[origin],ss[destination]}) {
     if(mode==BUS) stop->requestedTrips+=requested;
     riders=std::min(riders,stop->monthlyCapacity()-stop->endpointTrips);
    }
    if(riders<=0) continue;
    auto [it,inserted]=paths.try_emplace({origin,destination});
    if(inserted) it->second=pathBetween(origin,destination);
    const auto& path=it->second;
    if(path.empty()) continue;
    if(mode==BUS) for(auto point:path) {
     const int id=point.y*length+point.x;
     const auto* street=static_cast<const Transport*>(w.map(point)->reportingConstruction);
     const int group=street->constructionGroup->group;
     const int seats=(group==GROUP_TRACK || group==GROUP_TRACK_BRIDGE) ? 2000 : 6000;
     const int peak=std::max(street->trafficCount[STUFF_GOODS],street->trafficCount[STUFF_LABOR]);
     const int busy=std::clamp(peak*TRANSPORT_RATE,0,TRANSPORT_QUANTA*3/4);
     riders=std::min(riders,std::max(0,seats*(TRANSPORT_QUANTA-busy)/TRANSPORT_QUANTA-roadTrips[id]));
    }
    if(riders<=0) continue;
    ss[origin]->endpointTrips+=riders;
    ss[destination]->endpointTrips+=riders;
    (shopping ? shoppingHomes : servedHomes).insert(id);
    if(shopping) shoppingTrips[mode]+=riders;
    groupRiders+=riders;
    ss[origin]->riders+=riders;
    planned.push_back({h->point,destinations[destination]->point,ss[origin]->point,
      ss[destination]->point,mode,riders,shopping,path});
    if(mode==BUS) for(auto point:path) roadTrips[point.y*length+point.x]+=riders;
   }
   hubs.push_back(Hub{mode,capacity-groupRiders,groupRiders,ss,workplace,TransitRoute{},{}});auto& hub=hubs.back();
   // Keep the actual line segment for transfers and the longest segment for animation.
   for(size_t i=0;i<ss.size();++i) for(size_t j=i+1;j<ss.size();++j) {
    if(!ss[i]->active || !ss[j]->active) continue;
    if(ss.size()>2 && mode==BUS && ride[i][j]>=cells) continue;
    auto path=pathBetween(i,j);
    if(path.size()>hub.route.path.size()) hub.route={mode,path};
    hub.transferPaths.emplace(std::make_pair(i,j),std::move(path));
   }
  }

 }
 // Interchange passengers ride actual track segments on both sides of the transfer.
 auto leg = [](const Hub& hub,int from,int to) {
  auto it=hub.transferPaths.find({std::min(from,to),std::max(from,to)});
  return it==hub.transferPaths.end() ? std::vector<MapPoint>{} : it->second;
 };
 for(auto* home:homes) {
  const int id=home->point.y*length+home->point.x;
  if(servedHomes.count(id)) continue;
  const int people=static_cast<Residence*>(home)->local_population;
  if(people<=0) continue;
  auto transfer = [&]() {
   for(auto& origin:hubs) for(auto& destination:hubs) {
    if(origin.mode==destination.mode || origin.remaining<=0 || destination.remaining<=0) continue;
    for(size_t a=0;a<origin.stops.size();++a) {
     auto* start=origin.stops[a];
     if(!start->active || distanceTo(walking[origin.mode].at(start),length,*home)>walkingRadius(origin.mode)) continue;
     for(size_t x=0;x<origin.stops.size();++x) {
      auto first=leg(origin,a,x);
      if(first.empty()) continue;
      auto* exit=origin.stops[x];
      for(size_t y=0;y<destination.stops.size();++y) {
       auto* entry=destination.stops[y];
       if(distanceTo(walking[origin.mode].at(exit),length,*entry)>4) continue;
       for(size_t b=0;b<destination.stops.size();++b) {
        auto* end=destination.stops[b];
        auto* job=destination.jobs[b];
        if(!job || distanceTo(walking[origin.mode].at(start),length,*job)<=12) continue;
        auto second=leg(destination,y,b);
        if(second.empty()) continue;
        const int days=std::min({start->busy,exit->busy,entry->busy,end->busy});
        const int base=(origin.mode==BUS?1:2)+(destination.mode==BUS?1:2);
        const int fare=fares[origin.mode]+fares[destination.mode];
        const int count=people*2*days/(100+20*std::max(0,fare-base));
        int carried=std::min({origin.remaining,destination.remaining,count});
        const std::set<TransitFacility*> stops{start,exit,entry,end};
        for(auto* stop:stops)
         carried=std::min(carried,stop->monthlyCapacity()-stop->endpointTrips);
        if(carried<=0) continue;
        for(auto* stop:stops) stop->endpointTrips+=carried;
        for(const auto& segment : {std::make_pair(origin.mode,&first),std::make_pair(destination.mode,&second)})
         if(segment.first==BUS) for(const auto point:*segment.second) roadTrips[point.y*length+point.x]+=carried;
        origin.remaining-=carried;
        destination.remaining-=carried;
        origin.riders+=carried;
        destination.riders+=carried;
        start->riders+=carried;
        end->riders+=carried;
        servedHomes.insert(id);
        planned.push_back({home->point,job->point,start->point,end->point,
          origin.mode,carried,false,std::move(first),destination.mode,std::move(second),
          exit->point,entry->point});
        return;
       }
      }
     }
    }
   }
  };
  transfer();
 }
 for(auto& hub:hubs)if(hub.riders){passengers[hub.mode]+=hub.riders;running[hub.mode]+=int(hub.stops.size());}
 for(auto& hub:hubs) if(!hub.route.path.empty() &&
   std::count_if(hub.stops.begin(),hub.stops.end(),[](const auto* stop){return stop->active;}) >= 2)
   routes.push_back(std::move(hub.route));
 for(int mode=0;mode<4;++mode)faresMonth+=passengers[mode]*fares[mode];
 if(account || initializeSchedule) trips=std::move(planned);
 if(account){assessedMonth=month;w.income(faresMonth,w.stats.income.transit_fares);int trackCost=int(tunnels.size()+viaducts.size())*2;w.expense(trackCost,w.stats.expenses.transport);upkeepMonth=trackCost;}
}
