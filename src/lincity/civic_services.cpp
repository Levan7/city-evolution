// SPDX-License-Identifier: GPL-2.0-or-later
#include "civic_services.hpp"
#include "world.hpp"
#include "modules/residence.hpp"
#include "modules/school.hpp"
#include "modules/university.hpp"
#include "modules/firestation.hpp"
#include "modules/health_centre.hpp"
#include "modules/fire.hpp"
#include <algorithm>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <limits>
namespace {
int idOf(const World& w,MapPoint p){return p.y*w.map.len()+p.x;}
MapPoint pointOf(const World& w,int id){return {id%w.map.len(),id/w.map.len()};}
Construction* at(const World& w,int id){int n=w.map.len();return id>=0&&id<n*n?w.map(pointOf(w,id))->reportingConstruction:nullptr;}
int serviceFor(int kind){return kind==CivicServices::FIRE_CALL?CivicServices::FIRE:kind==CivicServices::MEDICAL_CALL?CivicServices::HEALTH:CivicServices::POLICE;}
int groupFor(int service){return service==CivicServices::FIRE?GROUP_FIRESTATION:service==CivicServices::HEALTH?GROUP_HEALTH:GROUP_POLICE;}
}
bool CivicServices::funded(int s,int day,int seed)const {int b=std::clamp(budget[s],0,100);return ((1LL*day+seed)*b)%100<b;}
int CivicServices::cost(int s,int base)const{return (1LL*base*std::clamp(budget[s],0,150)+99)/100;}
int CivicServices::illAt(int id)const{auto i=sick.find(id);return i==sick.end()?0:i->second;}
bool CivicServices::burningAt(int id)const{for(auto& c:incidents)if(c.kind==FIRE_CALL&&c.target==id&&c.severity>0)return true;return false;}
bool CivicServices::fireResponseAt(int id)const{for(auto& u:units)if(!u.returning&&u.kind==FIRE_CALL&&u.step+1>=int(u.path.size()))for(auto& c:incidents)if(c.id==u.call&&c.target==id)return true;return false;}
bool CivicServices::beginFire(World& w,Construction& c){int id=idOf(w,c.point);if(forceBurn==id||c.constructionGroup->group==GROUP_FIRE)return false;if(burningAt(id))return true;bool added=addIncident(FIRE_CALL,id,60);if(added)w.evolution.headline("Fire reported: crews are responding.");return added;}
bool CivicServices::addIncident(int kind,int target,int severity){
 for(auto& c:incidents)if(c.kind==kind&&c.target==target)return true;
 if(incidents.size()>=64){if(kind!=FIRE_CALL&&kind!=ZOMBIE_CALL)return false;auto it=std::find_if(incidents.begin(),incidents.end(),[](auto& c){return c.kind==MEDICAL_CALL||c.kind==CRIME_CALL;});if(it==incidents.end())return false;incidents.erase(it);}
 incidents.push_back({nextId++,kind,target,std::clamp(severity,1,100),0});return true;
}
bool CivicServices::stationReady(const Construction& c,int s)const{
 if(budget[s]<=0)return false;
 if(s==FIRE)return static_cast<const FireStation&>(c).active&&c.commodityCount[STUFF_LABOR]>=6&&c.commodityCount[STUFF_GOODS]>=3;
 if(s==HEALTH)return static_cast<const HealthCentre&>(c).active&&c.commodityCount[STUFF_LABOR]>=6&&c.commodityCount[STUFF_GOODS]>=40;
 return c.commodityCount[STUFF_LABOR]>=4&&c.commodityCount[STUFF_GOODS]>=1;
}
int CivicServices::terrainCost(const World& w,int id,int home,int target)const{
 int n=w.map.len();if(id<0||id>=n*n)return 0;auto p=pointOf(w,id);if(!w.map.is_visible(p))return 0;auto* t=w.map(p);auto* c=t->reportingConstruction;
 if(c){int root=idOf(w,c->point),g=c->constructionGroup->group;if(root==home||root==target)return 15;if(g==GROUP_ROAD||g==GROUP_TRACK||g==GROUP_ROAD_BRIDGE||g==GROUP_TRACK_BRIDGE)return 3+(c->commodityCount[STUFF_LABOR]>500?2:0);return 0;}
 if(t->group==GROUP_WATER)return 0;
 return t->group==GROUP_TREE||t->group==GROUP_TREE2||t->group==GROUP_TREE3?24:15;
}
std::vector<int> CivicServices::route(const World& w,int home,int target)const{
 int n=w.map.len();auto* dest=at(w,target);if(!dest||home<0||home>=n*n)return {};
 std::vector<int> d(n*n,std::numeric_limits<int>::max()),prev(n*n,-1);using Node=std::pair<int,int>;std::priority_queue<Node,std::vector<Node>,std::greater<Node>> q;d[home]=0;q.push({0,home});int end=-1;
 while(!q.empty()){auto [distance,id]=q.top();q.pop();if(distance!=d[id])continue;auto p=pointOf(w,id);int x=std::clamp(p.x,dest->point.x,dest->point.x+dest->constructionGroup->size-1),y=std::clamp(p.y,dest->point.y,dest->point.y+dest->constructionGroup->size-1);if(std::abs(p.x-x)+std::abs(p.y-y)<=1){end=id;break;}
  for(auto v:{p.n(),p.s(),p.e(),p.w()}){if(!w.map.is_visible(v))continue;int next=idOf(w,v),cost=terrainCost(w,next,home,target);if(cost&&distance+cost<d[next]){d[next]=distance+cost;prev[next]=id;q.push({d[next],next});}}
 }
 if(end<0)return {};std::vector<int> path;for(int id=end;id>=0;id=prev[id])path.push_back(id);std::reverse(path.begin(),path.end());return path;
}
void CivicServices::monthly(World& w){
 int pop=0,unemployed=0,capacity=0,extreme=0,worst=-1,worstPol=-1,police=0;std::vector<int> homes;std::map<int,int> next;
 for(auto* c:w.map.constructions){if(c->isDead())continue;int g=c->constructionGroup->group;
  if(g==GROUP_SCHOOL)capacity+=static_cast<School*>(c)->busy*5;
  if(g==GROUP_UNIVERSITY)capacity+=static_cast<University*>(c)->busy*8;
  if(g==GROUP_POLICE&&stationReady(*c,POLICE))++police;
  if(!GROUP_IS_RESIDENCE(g))continue;auto* r=static_cast<Residence*>(c);int count=r->local_population;if(count<=0)continue;int id=idOf(w,c->point);homes.push_back(id);pop+=count;if(!(c->flags&FLAG_EMPLOYED))unemployed+=count;
  int pollution=w.map(c->point)->pollution;if(pollution>1500)extreme+=count;if(pollution>worstPol){worstPol=pollution;worst=id;}
  int risk=std::clamp((pollution-80)/12,0,350)*(1000-education/3)/1000;int target=count*risk/1000,old=illAt(id);int recovery=(w.map(c->point)->flags&FLAG_HEALTH_COVER)&&budget[HEALTH]>0?std::max(1,old*budget[HEALTH]/300):old/10;int value=std::clamp(old+(target-old)/3-recovery,0,count);if(value)next[id]=value;if(value>=3)addIncident(MEDICAL_CALL,id,std::min(30,value));
 }
 sick=std::move(next);sickTotal=0;for(auto& [id,count]:sick)sickTotal+=count;
 int target=pop?std::min(1000,int(1LL*capacity*1000*budget[EDUCATION]/100/pop)):0;int delta=target-education;education=std::clamp(education+delta/12+(delta>0?1:delta<0?-1:0),0,1000);
 crime=pop?std::clamp(100-education/12+unemployed*100/pop-w.evolution.approval/4-std::min(25,police*10)-std::min(10,w.money_rates.dole/5),1,200):0;
 if(!homes.empty()&&random()%1000<unsigned(crime)){int id=homes[random()%homes.size()];if(addIncident(CRIME_CALL,id,5)){auto* c=at(w,id);c->commodityCount[STUFF_GOODS]-=std::min(5,c->commodityCount[STUFF_GOODS]);}}
 exposureMonths=pop>=100&&extreme*2>=pop?exposureMonths+1:std::max(0,exposureMonths-2);if(zombieCooldown>0)--zombieCooldown;
 if(zombiesEnabled&&exposureMonths>=24&&w.total_time>=5*NUMOF_DAYS_IN_YEAR&&zombieCooldown==0&&worst>=0&&random()%240==0){addIncident(ZOMBIE_CALL,worst,8);zombieCooldown=120;zombieNoticePending=true;w.evolution.headline("Your city was so polluted that zombies appeared. S.T.A.R.S. report for duty!");}
 if(autoTax)adjustTaxes(w);
 if(autoBudget) {
  const Fiscal accounts = fiscal(w);
  const bool underPressure = w.total_money < reserve || accounts.operating > accounts.revenue;
  const bool outbreak = pop > 0 && sickTotal + w.flu.active > pop / 20;
  for(int service = 0; service < 4; ++service) {
   int desired = underPressure ? 75 : 100;
   if(service == HEALTH && outbreak) desired = underPressure ? 100 : 125;
   budget[service] += std::clamp(desired - budget[service], -10, 10);
  }
 }


}
void CivicServices::update(World& w){
 if(w.total_time%NUMOF_DAYS_IN_MONTH==0)monthly(w);
 if(!zombiesEnabled)incidents.erase(std::remove_if(incidents.begin(),incidents.end(),[](auto& c){return c.kind==ZOMBIE_CALL;}),incidents.end());
 for(auto* c:w.map.constructions)if(!c->isDead()&&c->constructionGroup->group==GROUP_FIRE&&static_cast<Fire*>(c)->burning_days<FIRE_LENGTH)addIncident(FIRE_CALL,idOf(w,c->point),60);
 for(auto& c:incidents){++c.age;auto* b=at(w,c.target);if(c.kind==FIRE_CALL&&b&&b->constructionGroup->group!=GROUP_FIRE&&c.age>=600){forceBurn=c.target;b->torch();forceBurn=-1;c.age=0;}}
 incidents.erase(std::remove_if(incidents.begin(),incidents.end(),[&](auto& c){auto* b=at(w,c.target);return !b||c.severity<=0||(c.kind==FIRE_CALL&&b->constructionGroup->group==GROUP_FIRE&&static_cast<Fire*>(b)->burning_days>=FIRE_LENGTH)||(c.kind==MEDICAL_CALL&&(!GROUP_IS_RESIDENCE(b->constructionGroup->group)||illAt(c.target)+w.flu.infectiousAt(c.target)<=0))||(c.kind==CRIME_CALL&&c.age>=600)||(c.kind==ZOMBIE_CALL&&c.age>=1200);}),incidents.end());
 for(auto it=units.begin();it!=units.end();){auto& u=*it;int service=serviceFor(u.kind);auto* station=at(w,u.home);if(!station||idOf(w,station->point)!=u.home||station->constructionGroup->group!=groupFor(service)){it=units.erase(it);continue;}
  auto call=std::find_if(incidents.begin(),incidents.end(),[&](auto& c){return c.id==u.call;});if(call==incidents.end()&&!u.returning){u.path.resize(std::min(int(u.path.size()),u.step+1));std::reverse(u.path.begin(),u.path.end());u.step=0;u.credit=0;u.returning=true;}
  if(u.path.empty()){it=units.erase(it);continue;}
  if(!u.returning&&(!stationReady(*station,service)||!funded(service,w.total_time,u.home))){++it;continue;}
  if(u.step+1<int(u.path.size())){u.credit+=3;int target=u.returning?u.home:call->target;int cost=terrainCost(w,u.path[u.step+1],u.home,target);if(!cost){auto newPath=route(w,u.path[u.step],target);if(newPath.empty()){it=units.erase(it);continue;}u.path=std::move(newPath);u.step=0;u.credit=0;}else if(u.credit>=cost){u.credit-=cost;++u.step;}}
  else if(u.returning){it=units.erase(it);continue;}
  else if(call!=incidents.end()){++u.working;auto* b=at(w,call->target);if(u.kind==FIRE_CALL){if(b&&b->constructionGroup->group==GROUP_FIRE)static_cast<Fire*>(b)->burning_days+=std::max(1,60*budget[FIRE]/100);else call->severity-=3;}else if(u.working%5==0){--call->severity;if(u.kind==MEDICAL_CALL){w.flu.treat(call->target,1);auto i=sick.find(call->target);if(i!=sick.end()&&i->second>0)--i->second;}}if(call->severity<=0){++resolved;w.evolution.headline(u.kind==FIRE_CALL?"Fire contained: the building was saved.":"Emergency resolved; crew returning.");}}
  ++it;
 }
 if(w.total_time%10==0)for(auto& c:incidents){if(c.severity<=0)continue;bool assigned=false;for(auto& u:units)if(!u.returning&&u.call==c.id)assigned=true;if(assigned||units.size()>=128)continue;int s=serviceFor(c.kind),best=-1,lowest=std::numeric_limits<int>::max();std::vector<int> chosen;
  for(auto* b:w.map.constructions){if(b->isDead()||b->constructionGroup->group!=groupFor(s)||!stationReady(*b,s))continue;int home=idOf(w,b->point),out=0;for(auto& u:units)if(u.home==home)++out;if(out>=std::max(1,budget[s]/50))continue;auto path=route(w,home,c.target);if(path.empty())continue;int distance=0;for(int id:path)distance+=terrainCost(w,id,home,c.target);if(distance<lowest){lowest=distance;best=home;chosen=std::move(path);}}
  if(best>=0){Unit u;u.kind=c.kind;u.home=best;u.call=c.id;u.path=std::move(chosen);units.push_back(std::move(u));}
 }
 sickTotal=0;for(auto& [id,count]:sick)sickTotal+=count;
}
std::string CivicServices::save()const{
 std::ostringstream o;o<<1<<' ';for(int b:budget)o<<b<<' ';o<<autoBudget<<' '<<autoTax<<' '<<zombiesEnabled<<' '<<zombieNoticePending<<' '<<taxCeiling<<' '<<reserve<<' '<<education<<' '<<crime<<' '<<sickTotal<<' '<<exposureMonths<<' '<<zombieCooldown<<' '<<nextId<<' '<<resolved<<' '<<rng<<' '<<sick.size()<<' ';for(auto& [id,count]:sick)o<<id<<' '<<count<<' ';o<<incidents.size()<<' ';for(auto& c:incidents)o<<c.id<<' '<<c.kind<<' '<<c.target<<' '<<c.severity<<' '<<c.age<<' ';o<<units.size()<<' ';for(auto& u:units){o<<u.kind<<' '<<u.home<<' '<<u.call<<' '<<u.step<<' '<<u.credit<<' '<<u.working<<' '<<u.returning<<' '<<u.path.size()<<' ';for(int id:u.path)o<<id<<' ';}return o.str();
}
void CivicServices::load(const std::string& text,int side){
 CivicServices v;std::istringstream in(text);int version;auto require=[&](bool ok){if(!ok)throw std::runtime_error("Invalid civic service save state");};require(bool(in>>version)&&version==1);for(int& b:v.budget){in>>b;require(b>=0&&b<=150);}in>>v.autoBudget>>v.autoTax>>v.zombiesEnabled>>v.zombieNoticePending>>v.taxCeiling>>v.reserve>>v.education>>v.crime>>v.sickTotal>>v.exposureMonths>>v.zombieCooldown>>v.nextId>>v.resolved>>v.rng;require(bool(in)&&v.taxCeiling>=1&&v.taxCeiling<=30&&v.reserve>=0&&v.reserve<=1000000&&v.education>=0&&v.education<=1000&&v.crime>=0&&v.crime<=200&&v.exposureMonths>=0&&v.zombieCooldown>=0&&v.nextId>0&&v.resolved>=0);int cells=side*side,n;auto valid=[&](int id){return id>=0&&id<cells;};in>>n;require(bool(in)&&n>=0&&n<=cells);for(int j=0;j<n;++j){int id,count;in>>id>>count;require(bool(in)&&valid(id)&&count>=0&&count<=100000);v.sick[id]=count;}in>>n;require(bool(in)&&n>=0&&n<=64);std::set<int> ids;for(int j=0;j<n;++j){Incident c;in>>c.id>>c.kind>>c.target>>c.severity>>c.age;require(bool(in)&&c.id>0&&c.id<v.nextId&&ids.insert(c.id).second&&c.kind>=0&&c.kind<=3&&valid(c.target)&&c.severity>=-3&&c.severity<=100&&c.age>=0);v.incidents.push_back(c);}in>>n;require(bool(in)&&n>=0&&n<=128);for(int j=0;j<n;++j){Unit u;int steps;in>>u.kind>>u.home>>u.call>>u.step>>u.credit>>u.working>>u.returning>>steps;require(bool(in)&&u.kind>=0&&u.kind<=3&&valid(u.home)&&steps>0&&steps<=cells&&u.step>=0&&u.step<steps&&u.credit>=0&&u.credit<=100&&u.working>=0);for(int k=0;k<steps;++k){int id;in>>id;require(bool(in)&&valid(id));if(k){int prev=u.path.back();require(std::abs(prev%side-id%side)+std::abs(prev/side-id/side)==1);}u.path.push_back(id);}v.units.push_back(std::move(u));}in>>std::ws;require(in.eof());*this=std::move(v);
}

CivicServices::Fiscal CivicServices::fiscal(const World& w) const {
 const auto& in=w.stats.income; const auto& ex=w.stats.expenses; Fiscal f;
 f.revenue=1LL*in.income_tax.stat+in.goods_tax.stat+in.coal_tax.stat+in.export_tax.stat+in.transit_fares.stat;
 f.operating=1LL*ex.import.stat+ex.unemployment.stat+ex.transport.stat+ex.windmill.stat+ex.university.stat+ex.recycle.stat+ex.deaths.stat+ex.health.stat+ex.school.stat+ex.police.stat+ex.church.stat+ex.firestation.stat+ex.cricket.stat+ex.interest.stat;
 f.incomeTax=in.income_tax.stat;
 for(auto*c:w.map.constructions) if(auto*r=dynamic_cast<Residence*>(c)) f.population+=r->local_population;
 return f;
}

int CivicServices::recommendedTax(const World& w, std::string* why) const {
 const int rate=w.money_rates.income_tax, ceiling=std::clamp(taxCeiling,1,30);
 const Fiscal f=fiscal(w);
 auto money=[](long long v){return "$"+std::to_string(v);};
 auto say=[&](const std::string& text){if(why)*why=text;};
 int target=rate;
 if(f.population<=0 && f.revenue==0) say("There are no residents or tax income yet, so there is nothing to base a tax rate on.");
 else {
  // What one point of income tax is worth: measured from last year if possible, else a rough guess.
  const long long perPoint=(f.incomeTax>0 && rate>0) ? std::max<long long>(1,f.incomeTax/rate) : std::max<long long>(10,f.population/2);
  const long long gap=f.operating-f.revenue; // positive = deficit
  if(gap>0) {
   const long long need=(gap+perPoint-1)/perPoint;
   target=int(std::min<long long>(30,rate+need));
   say("Last year running costs ("+money(f.operating)+") were "+money(gap)+" more than tax income ("+money(f.revenue)+"). One income-tax point brings about "+money(perPoint)+", so "+std::to_string(need)+" more point(s) close the gap.");
  } else if(w.total_money<reserve) {
   target=rate+1;
   say("Taxes cover last year's costs, but cash ("+money(w.total_money)+") is below your reserve ("+money(reserve)+"), so the rate goes up one point.");
  } else if(w.total_money>4LL*reserve && -gap>f.operating/4 && rate>8) {
   const long long cut=std::min<long long>(rate-8,std::max<long long>(1,(-gap)/(2*perPoint)));
   target=rate-int(cut);
   say("Taxes brought in "+money(-gap)+" more than running costs and cash is comfortable ("+money(w.total_money)+"), so a lower rate of "+std::to_string(target)+"% still covers costs.");
  } else say("Tax income covers running costs and cash is in a healthy range: no change needed.");
 }
 if(target>ceiling) { target=ceiling; if(why)*why+=" The tax ceiling ("+std::to_string(ceiling)+"%) limits the rate; raise the ceiling to allow more."; }
 return std::clamp(target,0,30);
}

std::string CivicServices::applyRecommendedTax(World& w) {
 std::string why; const int target=recommendedTax(w,&why); int& rate=w.money_rates.income_tax; const int old=rate;
 rate=target; taxReviewedDay=w.total_time; // counts as this month's review
 taxNote = old==target ? "Income tax stays at "+std::to_string(target)+"%. "+why
                       : "Income tax set to "+std::to_string(target)+"% (was "+std::to_string(old)+"%). "+why;
 return taxNote;
}

void CivicServices::adjustTaxes(World& w) {
 if(taxReviewedDay>=0 && taxReviewedDay/NUMOF_DAYS_IN_MONTH==w.total_time/NUMOF_DAYS_IN_MONTH)return;
 taxReviewedDay=w.total_time;
 std::string why; const int target=recommendedTax(w,&why); int& rate=w.money_rates.income_tax; const int old=rate;
 if(rate>target) --rate; else if(rate<target) ++rate; // smooth: one point per monthly review
 if(rate!=old) taxNote="Automatic review: income tax "+std::to_string(old)+"% to "+std::to_string(rate)+"% (heading for "+std::to_string(target)+"%). "+why;
}
std::string CivicServices::debtAdvice(const World& w) const {
 if(w.total_money >= 0) return {};
 const Fiscal f = fiscal(w);
 const auto& ex = w.stats.expenses;
 const long long debt = -1LL * w.total_money;
 const long long balance = f.revenue - f.operating;
 std::string text = "Treasury is $" + std::to_string(debt) + " below zero. ";
 if(ex.interest.stat > 0)
  text += "Interest cost $" + std::to_string(ex.interest.stat) + " last year. ";
 if(f.operating == 0 && f.revenue == 0)
  return text + "There is no completed-year operating history yet. Hold off on optional construction and establish a tax base.";
 if(balance > 0) {
  const long long years = (debt + balance - 1) / balance;
  return text + "The last year's operating surplus was $" + std::to_string(balance)
    + ". At that pace, with no new construction, recovery takes about "
    + std::to_string(years) + " game year(s).";
 }
 text += "Pause optional construction until recurring income exceeds running costs. ";
 const long long services = 1LL * ex.school.stat + ex.university.stat
   + ex.health.stat + ex.firestation.stat + ex.police.stat;
 if(ex.import.stat > 0 && ex.import.stat >= services && ex.import.stat >= ex.transport.stat)
  text += "Review port imports first. They cost $" + std::to_string(ex.import.stat) + " last year.";
 else if(ex.transport.stat > 0 && ex.transport.stat >= services)
  text += "Review idle transit and freight facilities. Transport cost $" + std::to_string(ex.transport.stat) + " last year.";
 else if(services > 0)
  text += "Public services cost $" + std::to_string(services) + " last year. Use Auto funding or reduce unused capacity.";
 else
  text += "Connect homes to food and workplaces to grow recurring tax income.";
 return text;
}

std::string CivicServices::financialAdvice(const World& w) const {
 const Fiscal f=fiscal(w); const auto& in=w.stats.income; const auto& ex=w.stats.expenses;
 int farms=0,markets=0,workshops=0;
 for(auto*c:w.map.constructions){int g=c->constructionGroup->group;farms+=g==GROUP_ORGANIC_FARM;markets+=g==GROUP_MARKET;workshops+=g==GROUP_POTTERY||g==GROUP_INDUSTRY_L;}
 std::string why; const int target=recommendedTax(w,&why);
 std::string result="Recommended income tax: "+std::to_string(target)+"% (now "+std::to_string(w.money_rates.income_tax)+"%). "+why+" ";
 result+="Last year: revenue $"+std::to_string(f.revenue)+", running costs $"+std::to_string(f.operating)+" (construction excluded). ";
 if(f.revenue==0)result+="No tax base yet: connect occupied housing to food, water, markets and jobs before raising taxes. ";
 else if(f.operating>f.revenue)result+="To earn more without raising taxes, check idle services and missing workers or supplies, and add workplaces for unemployed people. ";
 if(!farms)result+="Build a farm on suitable land for local food. ";
 if(!markets)result+="Add a connected market to distribute food and goods. ";
 if(!workshops)result+="Consider a pottery workshop once workers, ore and coal are available. ";
 if(ex.unemployment.stat>in.income_tax.stat/2)result+="Social support is a large expense: connect homes to jobs. ";
 return result;
}
