// SPDX-License-Identifier: GPL-2.0-or-later
#include "transit.hpp"
#include "world.hpp"
#include "modules/public_transport.hpp"
#include "modules/all_modules.hpp"
#include <queue>
#include <algorithm>
#include <map>
namespace {
int walkDistance(const World& w,const Construction& a,const Construction& b,int limit){
 int n=w.map.len();std::map<int,int>d;std::queue<MapPoint> q;
 for(int y=0;y<a.constructionGroup->size;++y)for(int x=0;x<a.constructionGroup->size;++x){auto p=a.point.e(x).s(y);d[p.y*n+p.x]=0;q.push(p);}
 while(!q.empty()){auto p=q.front();q.pop();int dist=d[p.y*n+p.x];if(p.x>=b.point.x&&p.y>=b.point.y&&p.x<b.point.x+b.constructionGroup->size&&p.y<b.point.y+b.constructionGroup->size)return dist;if(dist>=limit)continue;
 for(auto z:{p.n(),p.e(),p.s(),p.w()}){if(!w.map.is_visible(z)||d.count(z.y*n+z.x))continue;auto* tile=w.map(z);auto*c=tile->reportingConstruction;if(c&&c!=&a&&c!=&b&&!(c->flags&FLAG_IS_TRANSPORT))continue;if(tile->is_water()&&(!c||!(c->flags&FLAG_IS_TRANSPORT)))continue;d[z.y*n+z.x]=dist+1;q.push(z);}}
 return limit+1;
}
}
bool TransitSystem::place(World& w,MapPoint p,bool elevated){
 auto& group=elevated?elevatedTrackGroup:metroTrackGroup;Message::ptr msg;
 if(!group.can_build(w,msg)||!group.can_build_here(w,p,msg))return false;
 auto& set=elevated?viaducts:tunnels;w.expense(group.getCosts(w),w.stats.expenses.construction,!group.no_credit);set.insert(p.y*w.map.len()+p.x);update(w,false);return true;
}
bool TransitSystem::remove(World& w,MapPoint p,bool elevated){auto& set=elevated?viaducts:tunnels;if(!set.erase(p.y*w.map.len()+p.x))return false;w.expense(100,w.stats.expenses.construction);update(w,false);return true;}
void TransitSystem::update(World& w,bool account){
 const int month=w.total_time/NUMOF_DAYS_IN_MONTH;if(account&&assessedMonth==month)return;
 int length=w.map.len(),cells=length*length;routes.clear();servedHomes.clear();passengers.fill(0);running.fill(0);disconnected.fill(0);connected.fill(0);unstaffed.fill(0);faresMonth=0;
 struct Hub {int mode,remaining,riders;std::vector<TransitFacility*> stops;std::vector<bool> jobs;TransitRoute route;};
 std::vector<Hub> hubs;
 std::vector<TransitFacility*> stations;std::vector<Construction*> homes,jobs;
 for(auto* c:w.map.constructions){if(c->isDead())continue;int g=c->constructionGroup->group;
  if(g>=GROUP_BUS_STOP&&g<=GROUP_ELEVATED_STATION){auto* s=static_cast<TransitFacility*>(c);stations.push_back(s);s->riders=0;s->requestedTrips=0;s->endpointTrips=0;}
  else if(GROUP_IS_RESIDENCE(g))homes.push_back(c);
  else if(!w.map(c->point)->is_transport() && g!=GROUP_POWER_LINE && g!=GROUP_WAREHOUSE && c->constructionGroup->commodityRuleCount[STUFF_LABOR].take&&c->constructionGroup->commodityRuleCount[STUFF_LABOR].maxload)jobs.push_back(c);
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
   int radius=mode==BUS?6:10;std::vector<bool> work(ss.size(),false);
   std::vector<int> jobWalk(ss.size(),radius+1);
   for(size_t i=0;i<ss.size();++i)for(auto* job:jobs)jobWalk[i]=std::min(jobWalk[i],walkDistance(w,*ss[i],*job,radius));
   std::vector<std::vector<int>> ride(ss.size(),std::vector<int>(ss.size(),cells));
   for(size_t i=0;i<ss.size();++i){std::vector<int> d(cells,-1);std::queue<int> q;for(int v:touches[ss[i]])if(comp[v]==entry.first){d[v]=0;q.push(v);}while(!q.empty()){int v=q.front();q.pop();for(int z:{v-1,v+1,v-length,v+length})if(z>=0&&z<cells&&std::abs(z%length-v%length)+std::abs(z/length-v/length)==1&&links[z]&&d[z]<0){d[z]=d[v]+1;q.push(z);}}for(size_t j=0;j<ss.size();++j)for(int v:touches[ss[j]])if(d[v]>=0)ride[i][j]=std::min(ride[i][j],d[v]);}
   for(size_t i=0;i<ss.size();++i)for(auto* job:jobs)if(jobWalk[i]<=radius){work[i]=true;break;}
   int groupRiders=0,capacity=0;for(auto* stop:ss) capacity+=stop->monthlyCapacity();
   for(auto* h:homes){int id=h->point.y*length+h->point.x;if(servedHomes.count(id))continue;int origin=-1,destination=-1;
    int journey=cells;
    for(size_t i=0;i<ss.size();++i){int walk=walkDistance(w,*h,*ss[i],radius);if(walk>radius||!ss[i]->active)continue;for(size_t j=0;j<ss.size();++j)if(j!=i&&work[j]&&ss[j]->active&&ride[i][j]<cells){int cost=3*(walk+jobWalk[j])+ride[i][j];if(cost<journey){journey=cost;origin=i;destination=j;}}}
    if(origin<0)continue;auto* residence=dynamic_cast<Residence*>(h);int people=residence?residence->local_population:0;
    int days=std::min(ss[origin]->busy,ss[destination]->busy);int demand=std::max(0,people)*2*60/(60+journey);
    int requested = demand * std::clamp(days,0,100) / 100;
    int riders = std::min(capacity-groupRiders, requested);
    if(mode == BUS) {
     for(auto* stop : {ss[origin], ss[destination]}) {
      stop->requestedTrips += requested;
      riders = std::min(riders, stop->monthlyCapacity() - stop->endpointTrips);
     }
     ss[origin]->endpointTrips += riders;
     ss[destination]->endpointTrips += riders;
    }
    if(riders<=0)continue;
    servedHomes.insert(id);groupRiders+=riders;ss[origin]->riders+=riders;
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
   for(size_t a=0;a<origin.stops.size()&&!served;++a){if(!origin.stops[a]->active||walkDistance(w,*home,*origin.stops[a],origin.mode==BUS?6:10)>(origin.mode==BUS?6:10))continue;
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
 if(account){assessedMonth=month;w.income(faresMonth,w.stats.income.transit_fares);int trackCost=int(tunnels.size()+viaducts.size())*2;w.expense(trackCost,w.stats.expenses.transport);upkeepMonth=trackCost;}
}
