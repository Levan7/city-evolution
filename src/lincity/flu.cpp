// SPDX-License-Identifier: GPL-2.0-or-later
#include "flu.hpp"
#include "world.hpp"
#include "modules/residence.hpp"
#include <algorithm>
#include <numeric>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <vector>
namespace {
template<size_t N> int sum(const std::array<int,N>& a){return std::accumulate(a.begin(),a.end(),0);}
template<size_t N> int advance(std::array<int,N>& a){int end=a.back();for(int i=N-1;i>0;--i)a[i]=a[i-1];a[0]=0;return end;}
bool street(int group){return group==GROUP_ROAD||group==GROUP_TRACK||group==GROUP_RAIL||group==GROUP_ROAD_BRIDGE||group==GROUP_TRACK_BRIDGE||group==GROUP_RAIL_BRIDGE;}
int total(const FluSystem::Household& h){return sum(h.exposed)+sum(h.infectious)+sum(h.immune);}
void fit(FluSystem::Household& h,int population){int excess=std::max(0,total(h)-population);auto trim=[&](auto& values){for(int i=int(values.size())-1;i>=0&&excess>0;--i){int n=std::min(excess,values[i]);values[i]-=n;excess-=n;}};trim(h.immune);trim(h.exposed);trim(h.infectious);}
}
int FluSystem::infectiousAt(int id)const{auto it=households.find(id);return it==households.end()?0:sum(it->second.infectious);}
void FluSystem::recount(){active=latent=recovered=0;for(auto& [id,h]:households){active+=sum(h.infectious);latent+=sum(h.exposed);recovered+=sum(h.immune);}}
void FluSystem::clear(){households.clear();active=latent=recovered=0;}
int FluSystem::seed(const World& w,int id,int count){
 if(!enabled||count<=0||id<0||id>=w.map.len()*w.map.len())return 0;
 auto* c=w.map({id%w.map.len(),id/w.map.len()})->construction;if(!c||!GROUP_IS_RESIDENCE(c->constructionGroup->group))return 0;
 auto& h=households[id];int population=static_cast<Residence*>(c)->local_population;fit(h,population);int n=std::clamp(count,0,std::max(0,population-total(h)));h.exposed[0]+=n;recount();return n;
}
int FluSystem::treat(int id,int count){auto it=households.find(id);if(it==households.end()||count<=0)return 0;auto& h=it->second;int healed=0;for(int i=int(h.infectious.size())-1;i>=0&&count>0;--i){int n=std::min(count,h.infectious[i]);h.infectious[i]-=n;h.immune[0]+=n;count-=n;healed+=n;}active-=healed;recovered+=healed;return healed;}
void FluSystem::update(World& w){
 if(!enabled){clear();return;}if(w.total_time%5!=0||lastDay==w.total_time)return;lastDay=w.total_time;
 const int n=w.map.len(),binWidth=(n+7)/8;
 struct Home {int id,pop,x,y,component=-1;};std::vector<Home> homes;int population=0;std::map<int,int> present;
 for(auto* c:w.map.constructions)if(!c->isDead()&&GROUP_IS_RESIDENCE(c->constructionGroup->group)){int pop=std::max(0,static_cast<Residence*>(c)->local_population);if(!pop)continue;int id=c->point.y*n+c->point.x;homes.push_back({id,pop,c->point.x,c->point.y});present[id]=pop;population+=pop;}
 std::sort(homes.begin(),homes.end(),[](auto& a,auto& b){return a.id<b.id;});
 for(auto it=households.begin();it!=households.end();){auto p=present.find(it->first);if(p==present.end()){it=households.erase(it);continue;}fit(it->second,p->second);auto& h=it->second;int recovered=advance(h.infectious);advance(h.immune);h.immune[0]+=recovered;h.infectious[0]+=advance(h.exposed);++it;}
 recount();
 int allowance=std::max(0,population/5-active-latent);
 if(w.total_time>=600&&w.total_time%NUMOF_DAYS_IN_MONTH==0&&population>=100&&allowance>0&&!homes.empty()&&random()%12==0) {int seeded=seed(w,homes[random()%homes.size()].id,std::min(2,allowance));allowance-=seeded;}
 if(active==0){for(auto it=households.begin();it!=households.end();)if(total(it->second)==0)it=households.erase(it);else ++it;recount();return;}
 // Road-connected commuting plus local neighbours; no city-wide teleporting infection.
 std::vector<int> component(n*n,-1);int components=0;std::queue<int> queue;
 for(int y=1;y<n-1;++y)for(int x=1;x<n-1;++x){int id=y*n+x;if(component[id]>=0||!street(w.map({x,y})->getTopConstructionGroup()->group))continue;component[id]=components;queue.push(id);while(!queue.empty()){int at=queue.front();queue.pop();MapPoint p(at%n,at/n);for(auto q:{p.n(),p.s(),p.e(),p.w()})if(w.map.is_visible(q)){int next=q.y*n+q.x;if(component[next]<0&&street(w.map(q)->getTopConstructionGroup()->group)){component[next]=components;queue.push(next);}}}++components;}
 std::vector<int> netPop(components),netInfectious(components);std::vector<std::vector<int>> bins(binWidth*binWidth);
 for(size_t i=0;i<homes.size();++i){auto& h=homes[i];auto* c=w.map({h.x,h.y})->construction;int size=c->constructionGroup->size;for(int y=h.y-1;y<=h.y+size;++y)for(int x=h.x-1;x<=h.x+size;++x){if(x<0||y<0||x>=n||y>=n)continue;int group=component[y*n+x];if(group>=0&&(h.component<0||group<h.component))h.component=group;}if(h.component>=0){netPop[h.component]+=h.pop;netInfectious[h.component]+=infectiousAt(h.id);}bins[h.y/8*binWidth+h.x/8].push_back(i);}
 std::map<int,int> newCases;
 for(auto& home:homes){auto it=households.find(home.id);int susceptible=home.pop-(it==households.end()?0:total(it->second));if(susceptible<=0||allowance<=0)continue;int nearbyPop=0,nearbyCases=0;
  for(int by=std::max(0,home.y/8-1);by<=std::min(binWidth-1,home.y/8+1);++by)for(int bx=std::max(0,home.x/8-1);bx<=std::min(binWidth-1,home.x/8+1);++bx)for(int i:bins[by*binWidth+bx]){auto& other=homes[i];if(other.id==home.id||std::abs(home.x-other.x)+std::abs(home.y-other.y)>8)continue;nearbyPop+=other.pop;nearbyCases+=infectiousAt(other.id);}
  int pressure=infectiousAt(home.id)*350/std::max(1,home.pop)+nearbyCases*180/std::max(1,nearbyPop);
  if(home.component>=0)pressure+=netInfectious[home.component]*80/std::max(1,netPop[home.component]);
  pressure=pressure*(1000-std::min(300,w.civic.education/3))/1000;auto* tile=w.map({home.x,home.y});if((tile->flags&FLAG_HEALTH_COVER)&&w.civic.budget[CivicServices::HEALTH]>=50)pressure=pressure*7/10;
  int month=(w.total_time/NUMOF_DAYS_IN_MONTH)%12;pressure=pressure*(month<2||month>=10?14:8)/10;
  int scaled=susceptible*pressure,exposed=scaled/1000;if(scaled%1000&&random()%1000<unsigned(scaled%1000))++exposed;exposed=std::min({exposed,susceptible,allowance,std::max(1,home.pop/8)});if(exposed){newCases[home.id]=exposed;allowance-=exposed;}
 }
 for(auto [id,count]:newCases)households[id].exposed[0]+=count;
 for(auto& home:homes){int ill=infectiousAt(home.id);if(!ill)continue;if((w.map({home.x,home.y})->flags&FLAG_HEALTH_COVER)&&w.civic.budget[CivicServices::HEALTH]>=75&&w.total_time%10==0)treat(home.id,std::max(1,ill*w.civic.budget[CivicServices::HEALTH]/2000));if(infectiousAt(home.id)>=3)w.civic.addIncident(CivicServices::MEDICAL_CALL,home.id,std::min(30,infectiousAt(home.id)));}
 for(auto it=households.begin();it!=households.end();)if(total(it->second)==0)it=households.erase(it);else ++it;
 recount();if(active>=5&&w.total_time-lastNotice>=300){w.evolution.headline("Flu cases: "+std::to_string(active)+". Healthcare supports recovery; education lowers transmission.");lastNotice=w.total_time;}
}
std::string FluSystem::save()const{std::ostringstream out;out<<1<<' '<<enabled<<' '<<lastDay<<' '<<lastNotice<<' '<<rng<<' '<<households.size()<<' ';for(auto& [id,h]:households){out<<id<<' ';for(int v:h.exposed)out<<v<<' ';for(int v:h.infectious)out<<v<<' ';for(int v:h.immune)out<<v<<' ';}return out.str();}
void FluSystem::load(const std::string& text,int side){FluSystem value;std::istringstream in(text);int version=0,count=0;auto check=[&](bool ok){if(!ok)throw std::runtime_error("Invalid flu save state");};in>>version>>value.enabled>>value.lastDay>>value.lastNotice>>value.rng>>count;check(bool(in)&&version==1&&count>=0&&count<=side*side/9&&value.lastDay>=-1&&value.lastNotice>=-1200&&value.rng!=0);for(int i=0;i<count;++i){int id=-1;Household h;in>>id;check(bool(in)&&id>=0&&id<side*side&&!value.households.count(id));auto read=[&](auto& fields){for(int& v:fields){in>>v;check(bool(in)&&v>=0&&v<=1000000);}};read(h.exposed);read(h.infectious);read(h.immune);check(total(h)<=1000000);value.households[id]=h;}in>>std::ws;check(in.eof());value.recount();*this=std::move(value);}
