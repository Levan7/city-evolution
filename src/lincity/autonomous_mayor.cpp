// SPDX-License-Identifier: GPL-2.0-or-later
// Rolling city planner. Uses normal construction costs/rules and the player's area.
#include "city_evolution.hpp"
#include "world.hpp"
#include "modules/all_modules.hpp"
#include <queue>
#include <map>
#include <limits>
#include <numeric>

void CityEvolution::setFullAutonomy(World& w,bool enabled) {
 fullAutonomy=enabled;
 if(enabled){mayorMode=2;mayorServices=mayorRoads=mayorParks=true;mayorPace=3;w.civic.autoTax=w.civic.autoBudget=true;}
 mayorReport=enabled?"Full autonomy ON: establish supply chains, connect projects and grow within "+mayorScopeText()+". No free money or population.":"Full autonomy OFF. Limited mayor permissions apply.";
 if(enabled)runMayor(w);
}
int CityEvolution::autonomousStep(World& w,bool allowBuilding,bool allowRoad,bool& builtRoad,std::string& report) {
 builtRoad=false;if(!fullAutonomy||mayorMode==0){report="Mayor is OFF.";return 0;}
 int x0,y0,x1,y1;mayorBounds(w,x0,y0,x1,y1);const int n=w.map.len(),inf=100000000;
 auto inside=[&](int x,int y){return x>=x0&&y>=y0&&x<x1&&y<y1&&w.map.is_visible({x,y});};
 auto roadAt=[&](int x,int y){if(!inside(x,y))return false;auto*c=w.map({x,y})->reportingConstruction;return c&&(c->flags&FLAG_IS_TRANSPORT);};
 auto free=[&](int x,int y){return inside(x,y)&&!w.map({x,y})->reportingConstruction&&(w.map({x,y})->is_bare()||w.map({x,y})->is_water());};
 const int gridX=(x0+x1)/2,gridY=(y0+y1)/2;
 auto corridor=[&](int x,int y){return (x-gridX+6*n)%6==0||(y-gridY+6*n)%6==0;};
 std::map<int,int> count;int pop=0,capacity=0,homes=0,ax=0,ay=0,jams=0;long long food=0,goods=0,labor=0,ore=0,coal=0;std::vector<Construction*> existing;
 for(auto*c:w.map.constructions){auto p=c->point;if(!inside(p.x,p.y))continue;existing.push_back(c);int g=c->constructionGroup->group;++count[g];
  food+=c->commodityCount[STUFF_FOOD];goods+=c->commodityCount[STUFF_GOODS];labor+=c->commodityCount[STUFF_LABOR];ore+=c->commodityCount[STUFF_ORE];coal+=c->commodityCount[STUFF_COAL];
  if(GROUP_IS_RESIDENCE(g)){auto*r=static_cast<Residence*>(c);pop+=r->local_population;capacity+=r->max_population;++homes;ax+=p.x;ay+=p.y;}
  if(auto*t=dynamic_cast<Transport*>(c))for(int k=0;k<STUFF_COUNT;++k)if(t->constructionGroup->commodityRuleCount[k].maxload>0&&t->trafficCount[k]*100LL/t->constructionGroup->commodityRuleCount[k].maxload>80){++jams;break;}
 }
 MapPoint anchor(homes?ax/homes:(x0+x1)/2,homes?ay/homes:(y0+y1)/2);
 // Full control replaces the limited mayor's fixed $25k lock, but never spends cash it lacks.
 const int reserve=pop<1000?std::min(1000,std::max(0,w.total_money/10)):std::min(w.civic.reserve,std::max(1000,w.total_money/5));
 const int available=std::max(0,w.total_money-reserve);
 auto& road=w.tech_level>=roadConstructionGroup.tech?roadConstructionGroup:trackConstructionGroup;
 Message::ptr message;
 auto account=[&](int before,ConstructionGroup& g,MapPoint p,const std::string& why,int roads){int spent=std::max(0,before-w.total_money);mayorSpent+=spent;++mayorProjects;mayorRoadsMonth+=roads;report="Built "+g.name+" at "+std::to_string(p.x)+","+std::to_string(p.y)+" ($"+std::to_string(spent)+"): "+why+".";if(g.group!=GROUP_TRACK&&g.group!=GROUP_ROAD)headline("Mayor: "+report);};
 if(!allowBuilding&&!allowRoad){report="Monthly construction allowance used.";return 0;}
 if(available<road.getCosts(w)){report="Waiting for revenue: $"+std::to_string(w.total_money)+" treasury, $"+std::to_string(reserve)+" adaptive reserve. No borrowing or free funds.";return 0;}
 // One reusable route field per decision; trees are buildable terrain, buildings are obstacles.
 std::vector<int> dist(n*n,inf),parent(n*n,-1);using Node=std::pair<int,int>;std::priority_queue<Node,std::vector<Node>,std::greater<Node>> q;
 int roadsFound=0;
 for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x)if(roadAt(x,y)){int id=y*n+x;dist[id]=0;q.push({0,id});++roadsFound;}
 if(!roadsFound){
  if(!allowRoad){report="Need a starter street; monthly road limit reached.";return 0;}
  MapPoint best(-1,-1);int score=inf;
  for(int y=y0+2;y<y1-2;++y)for(int x=x0+2;x<x1-2;++x){if(!free(x,y)||w.map({x,y})->is_water())continue;int d=std::abs(x-anchor.x)+std::abs(y-anchor.y);if(d<score&&road.can_build_here(w,{x,y},message)){best={x,y};score=d;}}
  if(best.x<0){report="No buildable land for a starter street inside the selected area.";return 0;}
  int before=w.total_money;w.buildConstruction(road,best);account(before,road,best,"start a connected settlement",0);builtRoad=true;return 1;
 }
 const int dx[]={1,-1,0,0},dy[]={0,0,1,-1};
 while(!q.empty()){auto[d,id]=q.top();q.pop();if(d!=dist[id]||d>80)continue;int x=id%n,y=id/n;for(int k=0;k<4;++k){int xx=x+dx[k],yy=y+dy[k];if(!free(xx,yy))continue;int ni=yy*n+xx,nd=d+(w.map({xx,yy})->is_water()?8:1);if(nd<dist[ni]){dist[ni]=nd;parent[ni]=id;q.push({nd,ni});}}}
 auto ring=[&](MapPoint p,int size){int best=-1;for(int a=0;a<size;++a)for(auto t:{p.e(a).n(),p.e(a).s(size),p.s(a).w(),p.s(a).e(size)})if(inside(t.x,t.y)&&dist[t.y*n+t.x]<inf&&(best<0||dist[t.y*n+t.x]<dist[best]))best=t.y*n+t.x;return best;};
 auto path=[&](int id){std::vector<MapPoint> route;while(id>=0&&dist[id]>0&&route.size()<80){route.emplace_back(id%n,id/n);id=parent[id];}std::reverse(route.begin(),route.end());return route;};
 // Repair access before duplicating idle workplaces or services.
 if(allowRoad)for(auto*c:existing){if(c->flags&FLAG_IS_TRANSPORT)continue;int id=ring(c->point,c->constructionGroup->size);if(id<0||dist[id]==0)continue;auto route=path(id);if(route.empty())continue;MapPoint p=route.front();if(road.can_build_here(w,p,message)){int before=w.total_money;w.buildConstruction(road,p);account(before,road,p,"connect isolated "+c->constructionGroup->name,0);builtRoad=true;return 1;}}
 if(!allowBuilding){report="Monthly building allowance used; next assessment after this month.";return 0;}
 struct Need{ConstructionGroup*g;std::string why;};std::vector<Need> needs;
 auto want=[&](bool yes,ConstructionGroup&g,const char*why){if(yes)needs.push_back({&g,why});};
 const int farms=count[GROUP_ORGANIC_FARM],markets=count[GROUP_MARKET],communes=count[GROUP_COMMUNE];
 const int workshops=count[GROUP_POTTERY]+count[GROUP_MILL]+count[GROUP_BLACKSMITH]+count[GROUP_INDUSTRY_L];
 want(!count[GROUP_WATERWELL],waterwellConstructionGroup,"establish clean water");
 want(!farms,organic_farmConstructionGroup,"establish food production");
 want(!markets,marketConstructionGroup,"distribute food, water, labor and goods");
 ConstructionGroup& starterHome=homes%3==0?residenceLLConstructionGroup:homes%3==1?residenceMLConstructionGroup:residenceHLConstructionGroup;
 want(homes<4,starterHome,"provide starter housing for arriving residents");
 want(communes<2,communeConstructionGroup,"supply renewable coal and ore before factories");
 want(!workshops,potteryConstructionGroup,"provide early jobs and locally produced goods");
 // No repeated farm spam if labor is missing; add capacity as actual population rises.
 want(farms<std::max(1,(pop+349)/350)&&food<pop*8LL,organic_farmConstructionGroup,"food stocks are low for this population");
 want(count[GROUP_WATERWELL]<std::max(1,homes/6),waterwellConstructionGroup,"extend water capacity");
 want(pop>80&&goods<pop*2LL&&workshops<std::max(2,pop/220)&&ore>40&&coal>20,potteryConstructionGroup,"goods shortage with raw materials available");
 want(pop>50&&goods>100&&!count[GROUP_CHURCH],churchConstructionGroup,"support wellbeing and informal learning while formal education unlocks");
 want(homes>=4&&goods>100&&!count[GROUP_SCHOOL],schoolConstructionGroup,"train workers once essentials are supplied");
 want(pop>150&&!count[GROUP_WINDMILL]&&!count[GROUP_WIND_POWER]&&!count[GROUP_COAL_POWER],windmillConstructionGroup,"power the productive street network");
 want(pop>150&&farms&&markets&&capacity>0&&((pop*100/capacity>70)||(homes<8&&w.people_pool>20)),starterHome,"expand housing where demand exceeds spare rooms");
 want(markets<std::max(1,homes/10),marketConstructionGroup,"extend neighborhood distribution");
 want(pop>250&&!count[GROUP_FIRESTATION],fireStationConstructionGroup,"provide fire response");
 want(pop>300&&!count[GROUP_HEALTH],healthCentreConstructionGroup,"provide healthcare");
 want(pop>400&&!count[GROUP_POLICE]&&w.civic.crime>30,policeGroup,"provide police response");
 want(pop>400&&count[GROUP_SCHOOL]<std::max(2,pop/500)&&goods>pop,schoolConstructionGroup,"expand education with funded supplies");
 want(pop>500&&workshops>1&&goods>pop&&!count[GROUP_UNIVERSITY],universityConstructionGroup,"train engineers for industry");
 want(pop>400&&goods<pop*3LL&&ore>200&&coal>100&&count[GROUP_INDUSTRY_L]<std::max(1,pop/900),industryLightConstructionGroup,"productive jobs with raw materials already present");
 want(pop>500&&approval<60&&count[GROUP_PARKLAND]<std::max(1,homes/12),parklandConstructionGroup,"improve neighborhood wellbeing");
 want(pop>700&&goods>pop&&!count[GROUP_CHURCH],churchConstructionGroup,"support neighborhood wellbeing");
 want(pop>700&&jams>2&&count[GROUP_BUS_STOP]<2,busStopGroup,"reduce congestion with connected neighborhood stops");
 want(pop>1000&&engineers>=20&&technicians>=10&&goods>pop&&!count[GROUP_TECH_CAMPUS],techCampusConstructionGroup,"use the trained workforce for higher value jobs");
 want(pop>1000&&pop*100/std::max(1,capacity)>80&&w.total_money>150000,ivoryTowerGroup,"add high density housing where demand is strong");
 // Funding/technology failures do not block cheaper necessities farther down the list.
 std::string blocked;
 for(auto need:needs){auto&g=*need.g;if(!g.can_build(w,message)||g.getCosts(w)>available){if(blocked.empty())blocked=g.name+" needs technology or $"+std::to_string(g.getCosts(w));continue;}
  MapPoint best(-1,-1);std::vector<MapPoint> bestRoute;int bestScore=inf;
  for(int y=std::max(y0,anchor.y-30);y+g.size<=std::min(y1,anchor.y+31);++y)for(int x=std::max(x0,anchor.x-30);x+g.size<=std::min(x1,anchor.x+31);++x){MapPoint p(x,y);bool clear=true;int dry=0;
   for(int j=0;j<g.size&&clear;++j)for(int i=0;i<g.size;++i){auto*t=w.map(p.e(i).s(j));if(!t->is_bare()||t->reportingConstruction||corridor(x+i,y+j)){clear=false;break;}dry+=!(t->flags&FLAG_HAS_UNDERGROUND_WATER);}
   if(!clear||(g.group==GROUP_WATERWELL&&dry)||!g.can_build_here(w,p,message))continue;
   int id=ring(p,g.size);if(id<0)continue;auto route=path(id);if(!route.empty()&&(!allowRoad||int(route.size())>mayorRoadTilesPerMonth()-mayorRoadsMonth))continue;
   int cost=g.getCosts(w)+int(route.size())*road.getCosts(w);if(cost>available)continue;
   int score=(std::abs(x-anchor.x)+std::abs(y-anchor.y))*2+int(route.size())*5;
   if(g.group==GROUP_ORGANIC_FARM)score+=dry*10;
   if(GROUP_IS_RESIDENCE(g.group))score+=w.map(p)->pollution/20;
   // Avoid merging all new bus coverage onto one stop.
   if(g.group==GROUP_BUS_STOP)for(auto*c:existing)if(c->constructionGroup->group==GROUP_BUS_STOP&&std::abs(c->point.x-x)+std::abs(c->point.y-y)<10)score+=1000;
   if(score<bestScore){best=p;bestScore=score;bestRoute=std::move(route);}
  }
  if(best.x<0){if(blocked.empty())blocked=g.name+" needs a buildable site with an affordable access route";continue;}
  // Preflight the entire road project. No player building is demolished or replaced.
  bool valid=true;for(auto p:bestRoute)valid&=road.can_build_here(w,p,message);if(!valid)continue;
  int before=w.total_money;
  try{for(auto p:bestRoute)w.buildConstruction(road,p);w.buildConstruction(g,best);account(before,g,best,need.why,int(bestRoute.size()));return 1;}
  catch(const Message::Exception&){mayorSpent+=std::max(0,before-w.total_money);report="Project deferred after ordinary construction checks.";return 0;}
 }
 // Upgrade loaded dirt tracks when road technology and cash permit, rather than adding idle factories.
 if(allowRoad&&w.tech_level>=roadConstructionGroup.tech)for(auto*c:existing)if(c->constructionGroup->group==GROUP_TRACK){auto*t=static_cast<Transport*>(c);bool loaded=false;for(int k=0;k<STUFF_COUNT;++k)loaded|=t->trafficCount[k]>t->constructionGroup->commodityRuleCount[k].maxload*3/4&&t->trafficCount[k]>0;MapPoint p=c->point;if(loaded&&roadConstructionGroup.getCosts(w)<=available&&roadConstructionGroup.can_build_here(w,p,message)){int before=w.total_money;w.buildConstruction(roadConstructionGroup,p);account(before,roadConstructionGroup,p,"upgrade a congested track",0);builtRoad=true;return 1;}}
 report=blocked.empty()?"Monitoring supplies, housing demand and service operation. No justified affordable project this month.":"Waiting: "+blocked+". Other priorities checked; treasury $"+std::to_string(w.total_money)+".";
 return 0;
}
