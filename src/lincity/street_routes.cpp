// SPDX-License-Identifier: GPL-2.0-or-later
#include "street_routes.hpp"
#include "world.hpp"
#include "lintypes.hpp"
#include <algorithm>
#include <deque>
bool isStreetTile(const World&w,MapPoint p){if(!w.map.is_visible(p))return false;int g=w.map(p)->getGroup();return g==GROUP_ROAD||g==GROUP_TRACK||g==GROUP_ROAD_BRIDGE||g==GROUP_TRACK_BRIDGE;}
std::vector<MapPoint> planStreetTrip(const World&w,MapPoint start,unsigned seed){
 if(!isStreetTile(w,start))return {};int n=w.map.len(),root=start.y*n+start.x;std::vector<int> parent(n*n,-1),distance(n*n,-1),seen;std::deque<int> q;parent[root]=root;distance[root]=0;q.push_back(root);int farthest=0;
 while(!q.empty()&&seen.size()<4096){int id=q.front();q.pop_front();seen.push_back(id);farthest=std::max(farthest,distance[id]);if(distance[id]>=64)continue;MapPoint p(id%n,id/n);for(auto next:{p.n(),p.e(),p.s(),p.w()})if(isStreetTile(w,next)){int ni=next.y*n+next.x;if(distance[ni]<0){distance[ni]=distance[id]+1;parent[ni]=id;q.push_back(ni);}}}
 if(farthest<2)return {};int minimum=std::min(farthest,std::max(6,farthest/2));std::vector<int> destinations,edges;
 for(int id:seen)if(distance[id]>=minimum){MapPoint p(id%n,id/n);bool nearBuilding=false;for(auto next:{p.n(),p.e(),p.s(),p.w()})if(w.map.is_visible(next)){auto*c=w.map(next)->reportingConstruction;nearBuilding|=c&&!(c->flags&FLAG_IS_TRANSPORT);}if(nearBuilding)destinations.push_back(id);edges.push_back(id);}
 auto& targets=destinations.empty()?edges:destinations;if(targets.empty())return {};seed^=seed>>16;seed*=0x7feb352dU;seed^=seed>>15;seed*=0x846ca68bU;seed^=seed>>16;int goal=targets[seed%targets.size()];std::vector<MapPoint> route;
 for(int id=goal;id!=root;id=parent[id]){if(id<0||route.size()>4096)return {};route.emplace_back(id%n,id/n);}std::reverse(route.begin(),route.end());return route;
}
