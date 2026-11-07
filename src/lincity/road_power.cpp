// SPDX-License-Identifier: GPL-2.0-or-later
#include "road_power.hpp"
#include "world.hpp"
#include "modules/all_modules.hpp"
#include "street_routes.hpp"
#include <queue>
#include <vector>
#include <algorithm>
namespace {
std::vector<int> access(const World&w,const Construction&c){std::vector<int>r;int n=w.map.len(),size=c.constructionGroup->size;for(int k=0;k<size;++k)for(auto p:{c.point.e(k).n(),c.point.e(k).s(size),c.point.s(k).w(),c.point.s(k).e(size)})if(isStreetTile(w,p))r.push_back(p.y*n+p.x);return r;}
}
void distributeRoadPower(World&w){
 std::vector<Construction*>sources;std::vector<Residence*>homes;
 for(auto*c:w.map.constructions){if(c->isDead())continue;int g=c->constructionGroup->group;if((g==GROUP_WINDMILL||g==GROUP_SUBSTATION)&&c->commodityCount[STUFF_LOVOLT]>0)sources.push_back(c);if(GROUP_IS_RESIDENCE(g)){auto*r=static_cast<Residence*>(c);if(r->local_population>0)homes.push_back(r);}}
 if(sources.empty()||homes.empty())return;int n=w.map.len();
 // Rotate priority daily so a distant neighborhood is not permanently last.
 std::rotate(homes.begin(),homes.begin()+w.total_time%homes.size(),homes.end());
 for(auto*source:sources){std::vector<int>d(n*n,-1);std::queue<int>q;for(int id:access(w,*source)){d[id]=0;q.push(id);}while(!q.empty()){int id=q.front();q.pop();MapPoint p(id%n,id/n);for(auto z:{p.n(),p.e(),p.s(),p.w()})if(isStreetTile(w,z)&&d[z.y*n+z.x]<0){d[z.y*n+z.x]=d[id]+1;q.push(z.y*n+z.x);}}
 for(auto*home:homes){int need=POWER_RES_OVERHEAD+POWER_USE_PER_PERSON*home->local_population-home->commodityCount[STUFF_LOVOLT];if(need<=0)continue;int distance=n*n;for(int id:access(w,*home))if(d[id]>=0)distance=std::min(distance,d[id]);if(distance==n*n)continue;
 // Roads abstract a local low-voltage distribution grid. At 100 street tiles,
 // half the dispatched energy arrives; losses never create free electricity.
 int stock=source->commodityCount[STUFF_LOVOLT];int delivered=std::min(need,int(100LL*stock/(100+distance)));if(delivered<=0)continue;int sent=int((1LL*delivered*(100+distance)+99)/100);int loss=sent-delivered;
 source->commodityCount[STUFF_LOVOLT]-=delivered;source->consumeStuff(STUFF_LOVOLT,loss);home->commodityCount[STUFF_LOVOLT]+=delivered;
 }}
}
