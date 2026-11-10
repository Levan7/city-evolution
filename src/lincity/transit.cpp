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
 int best = 100;
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
 const int length = w.map.len(), limit = walkingRadius(mode);
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
  if(distance >= limit) continue;
  for(auto next : {p.n(),p.e(),p.s(),p.w()}) {
   if(!w.map.is_visible(next)) continue;
   auto* tile = w.map(next);
   auto* building = tile->reportingConstruction;
   const bool transport = building && (building->flags & FLAG_IS_TRANSPORT);
   if(tile->is_water() && !transport) continue;
   if(!distances.emplace(next.y*length+next.x,distance+1).second) continue;
   // A building can be reached from its edge, but pedestrians cannot cross it.
   if(!building || transport) pending.push(next);
  }
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
     from->mode()!=trip.mode || to->mode()!=trip.mode || !destination) continue;
  if(!std::all_of(trip.path.begin(),trip.path.end(),[&](MapPoint p){return hasLink(w,p,trip.mode);})) continue;
  const auto sourceArea = walkingArea(w,from->point,from->constructionGroup->size,trip.mode);
  const auto destinationArea = walkingArea(w,to->point,to->constructionGroup->size,trip.mode);
  if(distanceTo(sourceArea,w.map.len(),*home)>walkingRadius(trip.mode) ||
     distanceTo(destinationArea,w.map.len(),*destination)>walkingRadius(trip.mode)) continue;
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
 struct Hub {int mode,remaining,riders;std::vector<TransitFacility*> stops;std::vector<bool> jobs;TransitRoute route;};
 std::vector<Hub> hubs;
 std::vector<Trip> planned;
 std::vector<TransitFacility*> stations;std::vector<Construction*> homes,jobs;
 std::vector<Market*> markets;
 std::map<TransitFacility*,std::map<int,int>> walking;
 std::set<int> shoppingHomes;
 for(auto* c:w.map.constructions){if(c->isDead())continue;int g=c->constructionGroup->group;
  if(g>=GROUP_BUS_STOP&&g<=GROUP_ELEVATED_STATION){auto* s=static_cast<TransitFacility*>(c);stations.push_back(s);walking.emplace(s,walkingArea(w,s->point,s->constructionGroup->size,s->mode()));s->riders=0;s->requestedTrips=0;s->endpointTrips=0;}
  if(g==GROUP_MARKET) markets.push_back(static_cast<Market*>(c));
  else if(GROUP_IS_RESIDENCE(g))homes.push_back(c);
  else if(stationMode(g)<0 && !w.map(c->point)->is_transport() && g!=GROUP_POWER_LINE && g!=GROUP_WAREHOUSE && c->constructionGroup->commodityRuleCount[STUFF_LABOR].take&&c->constructionGroup->commodityRuleCount[STUFF_LABOR].maxload)jobs.push_back(c);
 }
 auto distance=[](MapPoint a,MapPoint b){return std::abs(a.x-b.x)+std::abs(a.y-b.y);};
 for(int mode:{METRO,ELEVATED,RAIL,BUS}){
  int count=0;for(auto* station:stations)if(station->mode()==mode)++count;
  if(count<2){disconnected[mode]=count;continue;}
  std::vector<unsigned char> links(cells);std::vector<int> comp(cells,-1),parents(cells,-1);
  for(int y=1;y<length-1;++y)for(int x=1;x<length-1;++x){int id=y*length+x;auto* tile=w.map(MapPoint(x,y));int g=tile->reportingConstruction ? tile->reportingConstruction->constructionGroup->group : tile->group;
   links[id]=mode==BUS?(g==GROUP_ROAD||g==GROUP_ROAD_BRIDGE||g==GROUP_TRACK||g==GROUP_TRACK_BRIDGE):mode==RAIL?(g==GROUP_RAIL||g==GROUP_RAIL_BRIDGE):mode==METRO?tunnels.count(id):viaducts.count(id);
  }
  // Multi-tile stations connect to every orthogonally touching track tile.
  std::map<TransitFacility*,std::vector<int>> touches;
  for(auto* s:stations)if(s->mode()==mode){
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
   if(entry.second.empty()){++disconnected[mode];continue;}
   // A station may serve several components, but it never joins unrelated tracks.
   std::set<int> unique;for(int id:entry.second)unique.insert(comp[id]);for(int c:unique)groups[c].push_back(s);
  }
  for(auto& entry:groups){auto& ss=entry.second;if(ss.size()<2){++disconnected[mode];continue;}
   std::sort(ss.begin(),ss.end(),[](auto* a,auto* b){return a->point.y!=b->point.y?a->point.y<b->point.y:a->point.x<b->point.x;});
   connected[mode]+=ss.size();for(auto* stop:ss)if(!stop->active)++unstaffed[mode];
   int radius=walkingRadius(mode);std::vector<bool> work(ss.size(),false);
   std::vector<int> jobWalk(ss.size(),radius+1);
   std::vector<Construction*> workplace(ss.size()), shop(ss.size());
   std::vector<int> shopWalk(ss.size(),radius+1);
   for(size_t i=0;i<ss.size();++i) {
    for(auto* job:jobs) {
     const int walk = distanceTo(walking.at(ss[i]),length,*job);
     const auto& labor = job->constructionGroup->commodityRuleCount[STUFF_LABOR];
     if(walk<=radius && job->commodityCount[STUFF_LABOR]<labor.maxload/2 && walk<jobWalk[i]) {
      jobWalk[i]=walk; workplace[i]=job;
     }
    }
    for(auto* market:markets) {
     const int walk = distanceTo(walking.at(ss[i]),length,*market);
     const bool stocked = (market->commodityRuleCount[STUFF_FOOD].give && market->commodityCount[STUFF_FOOD]>0)
       || (market->commodityRuleCount[STUFF_GOODS].give && market->commodityCount[STUFF_GOODS]>0);
     if(stocked && market->commodityCount[STUFF_LABOR]>=market->labor && walk<shopWalk[i]) {shopWalk[i]=walk;shop[i]=market;}
    }
   }
   std::vector<std::vector<int>> ride(ss.size(),std::vector<int>(ss.size(),cells));
   for(size_t i=0;i<ss.size();++i){std::vector<int> d(cells,-1);std::queue<int> q;for(int v:touches[ss[i]])if(comp[v]==entry.first){d[v]=0;q.push(v);}while(!q.empty()){int v=q.front();q.pop();for(int z:{v-1,v+1,v-length,v+length})if(z>=0&&z<cells&&std::abs(z%length-v%length)+std::abs(z/length-v/length)==1&&links[z]&&d[z]<0){d[z]=d[v]+1;q.push(z);}}for(size_t j=0;j<ss.size();++j)for(int v:touches[ss[j]])if(d[v]>=0)ride[i][j]=std::min(ride[i][j],d[v]);}
   for(size_t i=0;i<ss.size();++i) work[i]=workplace[i]!=nullptr;
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
     const int walk=distanceTo(walking.at(ss[i]),length,*h);
     if(walk>radius||!ss[i]->active) continue;
     for(size_t j=0;j<ss.size();++j) {
      if(j==i||!destinations[j]||!ss[j]->active||ride[i][j]>=cells) continue;
      // Use local access first when the destination is already on the origin's walk network.
      if(distanceTo(walking.at(ss[i]),length,*destinations[j])<=radius) continue;
      const int cost=3*(walk+walks[j])+ride[i][j];
      if(cost<journey){journey=cost;origin=i;destination=j;}
     }
    }
    if(origin<0) continue;
    const int days=std::min(ss[origin]->busy,ss[destination]->busy);
    const int demand=people*(shopping ? 1 : 2)*60/(60+journey);
    const int requested=demand*std::clamp(days,0,100)/100;
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
   hubs.push_back(Hub{mode,capacity-groupRiders,groupRiders,ss,work,TransitRoute{}});auto& hub=hubs.back();
   // One representative vehicle route, traced on the actual connected network.
   int start=-1,end=-1;for(int id:touches[ss.front()])if(comp[id]==entry.first){start=id;break;}for(int id:touches[ss.back()])if(comp[id]==entry.first){end=id;break;}
   if(start>=0&&end>=0){std::fill(parents.begin(),parents.end(),-1);std::queue<int> q;q.push(start);parents[start]=start;
    while(!q.empty()&&parents[end]<0){int id=q.front();q.pop();for(int n:{id-1,id+1,id-length,id+length})if(n>=0&&n<cells&&std::abs(n%length-id%length)+std::abs(n/length-id/length)==1&&links[n]&&parents[n]<0){parents[n]=id;q.push(n);}}
    if(parents[end]>=0){TransitRoute route;route.mode=mode;for(int id=end;;id=parents[id]){route.path.emplace_back(id%length,id/length);if(id==start)break;}std::reverse(route.path.begin(),route.path.end());if(route.path.size()>1)hub.route=std::move(route);}
   }
  }

 }
 // One-transfer journeys: interchange stations within four walking tiles.
 std::vector<std::vector<size_t>> interchange(hubs.size());
 for(size_t i=0;i<hubs.size();++i)for(size_t j=i+1;j<hubs.size();++j)if(hubs[i].mode!=hubs[j].mode){bool adjacent=false;
  for(auto* a:hubs[i].stops)for(auto* b:hubs[j].stops)adjacent|=distance(a->point,b->point)<=4;
  if(adjacent){interchange[i].push_back(j);interchange[j].push_back(i);}
 }
 for(auto* home:homes){int id=home->point.y*length+home->point.x;if(servedHomes.count(id))continue;bool served=false;
  for(size_t i=0;i<hubs.size()&&!served;++i){auto& origin=hubs[i];if(origin.remaining<=0)continue;
   for(size_t a=0;a<origin.stops.size()&&!served;++a){if(!origin.stops[a]->active||distanceTo(walking.at(origin.stops[a]),length,*home)>walkingRadius(origin.mode))continue;
    for(size_t j:interchange[i]){auto& destination=hubs[j];if(destination.remaining<=0)continue;
     for(size_t b=0;b<destination.stops.size();++b)if(destination.jobs[b]&&destination.stops[b]->active){auto* res=dynamic_cast<Residence*>(home);int people=res?res->local_population:0;
      int days=std::min(origin.stops[a]->busy,destination.stops[b]->busy);
      const int requested = std::max(0,people)*2*std::clamp(days,0,100)/100;
      int trips=std::min({origin.remaining,destination.remaining,requested});
      TransitFacility* busStop = origin.mode == BUS ? origin.stops[a]
        : destination.mode == BUS ? destination.stops[b] : nullptr;
      if(busStop) {
       busStop->requestedTrips += requested;
       trips = std::min(trips, busStop->monthlyCapacity() - busStop->endpointTrips);
       busStop->endpointTrips += trips;
      }
      if(trips<=0)continue;
      origin.remaining-=trips;destination.remaining-=trips;origin.riders+=trips;destination.riders+=trips;origin.stops[a]->riders+=trips;destination.stops[b]->riders+=trips;servedHomes.insert(id);served=true;break;
     }
     if(served)break;
    }
   }
  }
 }
 for(auto& hub:hubs)if(hub.riders){passengers[hub.mode]+=hub.riders;running[hub.mode]+=int(hub.stops.size());if(!hub.route.path.empty())routes.push_back(std::move(hub.route));}
 for(int mode=0;mode<4;++mode)faresMonth+=passengers[mode]*(mode==BUS?1:2);
 if(account || initializeSchedule) trips=std::move(planned);
 if(account){assessedMonth=month;w.income(faresMonth,w.stats.income.transit_fares);int trackCost=int(tunnels.size()+viaducts.size())*2;w.expense(trackCost,w.stats.expenses.transport);upkeepMonth=trackCost;}
}
