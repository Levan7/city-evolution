// SPDX-License-Identifier: GPL-2.0-or-later
#include "city_evolution.hpp"
#include "modules/civic_buildings.hpp"
#include "modules/organic_farm.hpp"
#include "modules/market.hpp"
#include "modules/light_industry.hpp"
#include "modules/church.hpp"
#include <deque>
#include <vector>
#include "world.hpp"
#include "lintypes.hpp"
#include "modules/city_business.hpp"
#include "modules/all_modules.hpp"
#include "modules/school.hpp"
#include "modules/university.hpp"
#include <limits>
#include <cmath>
// Monthly: train, lose and allocate skilled people. Deterministic, integer only.
void CityEvolution::updateWorkforce(World& world) {
  residents = world.stats.population.population_m.stat;
  schoolsWorking = universitiesWorking = campuses = lightFactories = heavyFactories = 0;
  int schoolDays = 0, uniDays = 0;
  for(auto* c : world.map.constructions) {
    const int g = c->constructionGroup->group;
    const int di = districtAt(c->point.x, c->point.y);
    const bool uniDistrict = di >= 0 && districts[di].type == DIST_UNIVERSITY; // University districts train 25% more
    const int boost = uniDistrict ? UNIVERSITY_DISTRICT_TRAINING : 100;
    if(g == GROUP_SCHOOL) { int d = std::clamp(static_cast<School*>(c)->busy, 0, 100); schoolDays += d * boost / 100; schoolsWorking += d > 0; }
    else if(g == GROUP_UNIVERSITY) { int d = std::clamp(static_cast<University*>(c)->busy, 0, 100); uniDays += d * boost / 100; universitiesWorking += d > 0; }
    else if(g == GROUP_TECH_CAMPUS) ++campuses;
    else if(g == GROUP_INDUSTRY_L) ++lightFactories;
    else if(g == GROUP_INDUSTRY_H) ++heavyFactories;
  }
  techTrainedMonth = schoolDays * TECHS_PER_SCHOOL_MONTH / 100;
  engTrainedMonth = uniDays * ENGINEERS_PER_UNI_MONTH / 100;
  // Retirement and emigration, with fractions carried over so small groups are treated like big ones.
  const int rate = lossPermille(approval);
  const int techTotal = technicians * rate + techCarry, engTotal = engineers * rate + engCarry;
  const int techLoss = techTotal / 1000, engLoss = engTotal / 1000;
  techCarry = techTotal % 1000; engCarry = engTotal % 1000;
  skilledLeftMonth = techLoss + engLoss;
  technicians = std::max(0, technicians + techTrainedMonth - techLoss);
  engineers = std::max(0, engineers + engTrainedMonth - engLoss);
  // Technicians are limited by housing; engineers are not.
  technicians = std::min(technicians, residents / RESIDENTS_PER_TECH);
  engineers = std::min(engineers, residents); // no housing limit, but never more engineers than people
  techNeed = campuses * CAMPUS_TECHS + lightFactories * FACTORY_L_TECHS + heavyFactories * FACTORY_H_TECHS;
  engNeed = campuses * CAMPUS_ENGINEERS + lightFactories * FACTORY_L_ENGINEERS + heavyFactories * FACTORY_H_ENGINEERS;
  techCoverage = techNeed ? std::min(1000, technicians * 1000 / techNeed) : 1000;
  engCoverage = engNeed ? std::min(1000, engineers * 1000 / engNeed) : 1000;
}

void CityEvolution::updateCity(World& world) {
  if(world.total_time % NUMOF_DAYS_IN_MONTH) return;
  homes = poweredHomes = amenities = operatingBusinesses = newJobs = 0;
  int fed = 0, employed = 0, pollution = 0, spiritualHomes=0;
  churchesWorking=0;
  for(auto* c : world.map.constructions) {
    const int g = c->constructionGroup->group;
    if(GROUP_IS_RESIDENCE(g)) {
      ++homes;
      if(churchServes(world,c->point))++spiritualHomes;
      fed += bool(c->flags & FLAG_FED);
      employed += bool(c->flags & FLAG_EMPLOYED);
      poweredHomes += bool(c->flags & FLAG_HAD_POWER);
      pollution += world.map(c->point)->pollution;
    }
    if(g==GROUP_CHURCH&&static_cast<GeorgianChurch*>(c)->active)++churchesWorking;
    if(g >= GROUP_WAREHOUSE && g <= GROUP_FOODHALL) {
      auto* b = static_cast<CityBusiness*>(c);
      if(b->busy > 0) {
        ++operatingBusinesses;
        newJobs += g == GROUP_TECH_CAMPUS ? 8 : g == GROUP_CAFE ? 4 : g == GROUP_FOODHALL ? 8 : 0;
        if(g == GROUP_CAFE || g == GROUP_FOODHALL) ++amenities;
      }
    }
  }
  updateDistricts(world);
  updateWorkforce(world);
  int target = homes ? 25 + fed*30/homes + employed*20/homes + poweredHomes*10/homes
    + std::min(10, amenities*20/homes) - std::min(25, pollution/homes/20) : 60;
  spiritualCoverage=homes?spiritualHomes*100/homes:0;
  target+=spiritualCoverage*6/100;
  if(world.total_money < 0) target -= 10;
  approval = std::clamp((approval*2 + target)/3, 5, 95);
  if(!fullAutonomy) updatePlan(world);
  runMayor(world);
  // Old cities schedule a fresh term from their first assessment.
  if(nextElection <= 0) nextElection = world.total_time + 4*NUMOF_DAYS_IN_YEAR;
  if(world.total_time >= nextElection) {
    ++electionCount;
    lastVote = approval;
    nextElection = world.total_time + 4*NUMOF_DAYS_IN_YEAR;
    headline("Election " + std::to_string(electionCount) + ": " + std::to_string(lastVote)
      + "% approval. " + (lastVote >= 50 ? "Council endorses your programme." : "Council requests better services.")
      + " You remain City Steward.");
  }
}

// Deterministic offline planning. No demolition and no authority over player tools.
// ---- Road path-finding for the mayor ---------------------------------------------------------
// Finds the tile that extends the existing road network one step toward an unconnected building.
static bool roadPassable(World& w, int x, int y, int x0, int y0, int x1, int y1) {
  if(x<x0||y<y0||x>=x1||y>=y1) return false;
  MapPoint p(x,y); if(!w.map.is_visible(p)) return false;
  auto* t=w.map(p);
  return t->is_bare() && (t->group==GROUP_BARE||t->group==GROUP_DESERT) && !t->reportingConstruction;
}
static bool roadTouchesNetwork(World& w, int x, int y, Construction* target) {
  static const int dx[4]={1,-1,0,0}, dy[4]={0,0,1,-1};
  for(int k=0;k<4;++k) {
    MapPoint n(x+dx[k],y+dy[k]); if(!w.map.is_visible(n)) continue;
    auto* c=w.map(n)->reportingConstruction;
    if(c && c!=target && (c->flags & FLAG_IS_TRANSPORT)) return true;
  }
  return false;
}
static MapPoint nextRoadTile(World& w, Construction* target, int x0, int y0, int x1, int y1) {
  const int len=w.map.len(), size=target->constructionGroup->size;
  std::vector<int> parent(size_t(len)*len,-2); std::deque<int> queue;
  for(int j=-1;j<=size;++j) for(int i=-1;i<=size;++i) {
    if(i!=-1&&j!=-1&&i!=size&&j!=size) continue;       // only the ring around the building
    if((i==-1||i==size)&&(j==-1||j==size)) continue;   // roads connect edge to edge, not corner to corner
    const int x=target->point.x+i, y=target->point.y+j;
    if(roadPassable(w,x,y,x0,y0,x1,y1)) { parent[y*len+x]=-1; queue.push_back(y*len+x); }
  }
  static const int dx[4]={1,-1,0,0}, dy[4]={0,0,1,-1};
  while(!queue.empty()) {
    const int cur=queue.front(); queue.pop_front();
    const int x=cur%len, y=cur/len;
    if(roadTouchesNetwork(w,x,y,target)) return MapPoint(x,y); // nearest tile that touches a road: build it
    for(int k=0;k<4;++k) {
      const int nx=x+dx[k], ny=y+dy[k];
      if(roadPassable(w,nx,ny,x0,y0,x1,y1) && parent[ny*len+nx]==-2) { parent[ny*len+nx]=cur; queue.push_back(ny*len+nx); }
    }
  }
  return MapPoint(-1,-1);
}

int CityEvolution::mayorStep(World& w, bool allowBuilding, bool allowRoad, bool& builtRoad, std::string& mayorReport) {
  if(fullAutonomy) return autonomousStep(w,allowBuilding,allowRoad,builtRoad,mayorReport);
  builtRoad=false;
  int x0,y0,x1,y1; mayorBounds(w,x0,y0,x1,y1);
  int housing=0,schools=0,wells=0,parks=0,health=0,fire=0,pop=0,capacity=0,farms=0,markets=0,factories=0,police=0,food=0,goods=0;
  MapPoint anchor((x0+x1)/2,(y0+y1)/2); int distanceScore=-1;
  Construction* isolated=nullptr;
  for(auto* c:w.map.constructions) {
    auto p=c->point;if(p.x<x0||p.y<y0||p.x>=x1||p.y>=y1)continue;
    int g=c->constructionGroup->group;
    if(GROUP_IS_RESIDENCE(g)) { ++housing; anchor=p;auto* r=static_cast<Residence*>(c);pop+=r->local_population;capacity+=r->max_population;food+=c->commodityCount[STUFF_FOOD];goods+=c->commodityCount[STUFF_GOODS]; }
    farms+=g==GROUP_ORGANIC_FARM;markets+=g==GROUP_MARKET;factories+=g==GROUP_INDUSTRY_L;police+=g==GROUP_POLICE;
    schools+=g==GROUP_SCHOOL;wells+=g==GROUP_WATERWELL;parks+=g==GROUP_PARKLAND;
    health+=g==GROUP_HEALTH;fire+=g==GROUP_FIRESTATION;
    if(!c->constructionGroup->commodityRuleCount[STUFF_LABOR].maxload)continue;
    bool connected=false;
    for(auto* n:c->neighbors)connected|=bool(n->flags & FLAG_IS_TRANSPORT);
    if(!connected&&!GROUP_IS_RESIDENCE(g))isolated=c;
  }
  ConstructionGroup* group=nullptr;std::string reason,planWait;
  if(planState==PLAN_ACTIVE && !planNextBuild(w,group,reason)) planWait=reason;
  if(group) {}
  else if(mayorRoads && isolated) {
    group=w.tech_level>=roadConstructionGroup.tech ? static_cast<ConstructionGroup*>(&roadConstructionGroup) : &trackConstructionGroup;
    anchor=isolated->point; reason="connect isolated "+isolated->constructionGroup->name;
  } else if(housing && mayorServices && wells<std::max(1,housing/8)) {group=&waterwellConstructionGroup;reason="water provision";}
  else if(housing>=4 && mayorServices && schools<std::max(1,housing/8)) {group=&schoolConstructionGroup;reason="education capacity";}
  else if(housing>=16 && mayorServices && !health) {group=&healthCentreConstructionGroup;reason="health coverage";}
  else if(housing>=8 && mayorServices && !police && w.civic.crime>30) {group=&policeGroup;reason="police response capacity";}
  else if(housing>=24 && mayorServices && !fire) {group=&fireStationConstructionGroup;reason="fire protection";}
  else if(housing>=4 && mayorParks && parks<std::max(1,housing/8)) {group=&parklandConstructionGroup;reason="green space near housing";}
  else if(pop>=100 && housing>=4 && mayorServices && !markets) {group=&marketConstructionGroup;reason="food and goods distribution";}
  else if(housing>=4 && mayorServices && farms<std::max(1,pop/500) && food<pop*2) {group=&organic_farmConstructionGroup;reason="food supply";}
  else if(markets && farms && mayorServices && factories<std::max(1,pop/1000) && goods<pop/2 && engCoverage>=500) {group=&industryLightConstructionGroup;reason="jobs and goods supply";}
  else if(capacity>0 && pop*100/capacity>=85 && w.people_pool>100 && mayorServices) {group=&residenceLLConstructionGroup;reason="housing shortage";}
  if(!group) {mayorReport=planWait.empty() ? "Nothing needs building here right now. I check again each month." : "Nothing to build today. I am following the plan: "+planWait+".";return 0;}
  mayorReport="I suggest "+group->name+" for "+reason+". It needs road access and supplies.";
  if(mayorMode==0)return 0;
  const bool isRoad=group==&roadConstructionGroup||group==&trackConstructionGroup;
  if(isRoad&&!allowRoad) {mayorReport="Road work for this month is done.";return 0;}
  if(!isRoad&&!allowBuilding) {mayorReport="Building limit for this month reached. Next: "+group->name+" ("+reason+").";return 0;}
  Message::ptr message;
  if(!group->can_build(w,message)) {mayorReport="Waiting: "+group->name+" is not unlocked or affordable.";return 0;}
  int cost=group->getCosts(w);
  if(cost>std::max(0,mayorBudget-mayorSpent)||w.total_money-cost<25000) {
    mayorReport="I cannot spend yet. I must keep $25,000 safe. My yearly limit is $"+std::to_string(mayorBudget)+"/year.";return 0;
  }
  MapPoint best(-1,-1);int bestScore=std::numeric_limits<int>::max();
  if(isRoad && isolated) best=nextRoadTile(w,isolated,x0,y0,x1,y1);
  else for(int y=y0;y+group->size<=y1;++y)for(int x=x0;x+group->size<=x1;++x) {
    MapPoint p(x,y);int d=std::abs(x-anchor.x)+std::abs(y-anchor.y);
    if(d>12||d>=bestScore)continue;
    bool clear=true,access=false,atTarget=false;
    for(int j=0;j<group->size;++j)for(int i=0;i<group->size;++i) {
      auto* t=w.map(p.e(i).s(j));
      clear &= t->is_bare() && (t->group==GROUP_BARE||t->group==GROUP_DESERT);
    }
    if(!clear)continue;
    for(int j=-1;j<=group->size;++j)for(int i=-1;i<=group->size;++i) {
      if(i!=-1&&j!=-1&&i!=group->size&&j!=group->size)continue;
      MapPoint n=p.e(i).s(j);if(!w.map.is_visible(n))continue;
      auto* c=w.map(n)->reportingConstruction;
      access |= c && bool(c->flags & FLAG_IS_TRANSPORT);
      atTarget |= c && c==isolated;
    }
    if(group==&roadConstructionGroup||group==&trackConstructionGroup) {if(!atTarget||!access)continue;}
    else if(!access)continue;
    if(!group->can_build_here(w,p,message))continue;
    best=p;bestScore=d;
  }
  if(best.x<0) {mayorReport=isRoad ? "I found no free land to run a road to the "+isolated->constructionGroup->name+". I will not knock anything down."
    : "I need empty land beside a road. I will not knock anything down.";return 0;}
  if(isRoad && !group->can_build_here(w,best,message)) {mayorReport="The next road tile is blocked. I will not knock anything down.";return 0;}
  int before=w.total_money;
  try {
    w.buildConstruction(*group,best);
    mayorSpent+=std::max(0,before-w.total_money);++mayorProjects;
    if(planState==PLAN_ACTIVE) planSpent+=std::max(0,before-w.total_money);
    mayorReport="Built "+group->name+" at "+std::to_string(best.x)+","+std::to_string(best.y)+" for $"+std::to_string(cost)+".";
    builtRoad=isRoad;
    if(!isRoad) headline("Mayor: "+mayorReport);
    return 1;
  } catch(const Message::Exception&) {
    mayorReport="Project deferred after construction checks. No authority to demolish.";
  }
  return 0;
}

void CityEvolution::runMayor(World& w) {
  const int year=w.total_time/NUMOF_DAYS_IN_YEAR;
  if(mayorYear!=year) { mayorYear=year; mayorSpent=0; }
  mayorBuiltMonth=mayorRoadsMonth=0;
  const int maxBuildings=mayorBuildingsPerMonth(), maxRoads=mayorRoadTilesPerMonth(), spentBefore=mayorSpent;
  std::string last;
  // He keeps working this month until his building and road allowance, his budget or his ideas run out.
  for(int guard=0; guard<maxBuildings+maxRoads; ++guard) {
    bool road=false; std::string report;
    const int result=mayorStep(w,mayorBuiltMonth<maxBuildings,mayorRoadsMonth<maxRoads,road,report);
    last=report;
    if(result!=1) break;
    if(road) ++mayorRoadsMonth; else ++mayorBuiltMonth;
  }
  if(mayorBuiltMonth+mayorRoadsMonth>0)
    mayorReport="This month I built "+std::to_string(mayorBuiltMonth)+" building(s) and "+std::to_string(mayorRoadsMonth)+" road tile(s) for $"
      +std::to_string(mayorSpent-spentBefore)+". Then: "+last;
  else mayorReport=last;
}

void CityEvolution::mayorBounds(World& w, int& x0, int& y0, int& x1, int& y1) const {
  const int len=w.map.len();
  x0=1;y0=1;x1=len-1;y1=len-1;
  if(mayorMode==1) { x0=districtX;y0=districtY;x1=std::min(len-1,x0+25);y1=std::min(len-1,y0+25);return; }
  int ax0=-1,ay0=-1,ax1=-1,ay1=-1;
  if(mayorScope==1 && mayorDistrict>=0 && mayorDistrict<int(districts.size())) {
    const auto& d=districts[mayorDistrict]; ax0=d.x0;ay0=d.y0;ax1=d.x1;ay1=d.y1;
  } else if(mayorScope==2) { ax0=mayorX0;ay0=mayorY0;ax1=mayorX1;ay1=mayorY1; }
  if(ax0<0) return;
  x0=std::clamp(ax0,1,len-2);y0=std::clamp(ay0,1,len-2);x1=std::clamp(ax1+1,x0+1,len-1);y1=std::clamp(ay1+1,y0+1,len-1);
}
void CityEvolution::setMayorScopeCity() { mayorScope=0; mayorDistrict=-1; }
void CityEvolution::setMayorScopeDistrict(int index) {
  if(index<0||index>=int(districts.size())) return;
  mayorScope=1; mayorDistrict=index;
}
void CityEvolution::setMayorScopeZone(World& w, int ax0, int ay0, int ax1, int ay1) {
  const int len=w.map.len();
  if(ax0>ax1) std::swap(ax0,ax1); if(ay0>ay1) std::swap(ay0,ay1);
  mayorX0=std::clamp(ax0,0,len-1);mayorX1=std::clamp(ax1,0,len-1);mayorY0=std::clamp(ay0,0,len-1);mayorY1=std::clamp(ay1,0,len-1);
  mayorScope=2;
}
std::string CityEvolution::mayorScopeText() const {
  if(mayorScope==1&&mayorDistrict>=0&&mayorDistrict<int(districts.size())) return "district "+districts[mayorDistrict].name;
  if(mayorScope==2) return "a zone you drew ("+std::to_string(mayorX1-mayorX0+1)+"x"+std::to_string(mayorY1-mayorY0+1)+" tiles)";
  return "the whole city";
}
void CityEvolution::renameCity(std::string name) {
  std::string out; for(unsigned char c:name) if(c>=32&&c!=127&&c!='|') out+=char(c);
  while(!out.empty()&&out.back()==' ') out.pop_back();
  size_t st=0; while(st<out.size()&&out[st]==' ') ++st; out=out.substr(st);
  if(out.size()>32) { out.resize(32); while(!out.empty()&&(static_cast<unsigned char>(out.back())&0xC0)==0x80) out.pop_back();
    if(!out.empty()&&static_cast<unsigned char>(out.back())>=0xC0) out.pop_back(); }
  if(!out.empty()) { cityName=out; headline("The city is now called "+out+"."); }
}

// ---------------------------------------------------------------------------
// Advisors. Every message names the cause and uses the city's real numbers.
// Sometimes the right advice is NOT to build something.
// ---------------------------------------------------------------------------
std::vector<Advice> CityEvolution::advise(World& w) const {
  std::vector<Advice> out;
  auto add = [&](int sev, const std::string& title, const std::string& text) { out.push_back({sev, title, text}); };
  auto num = [](int v) { return std::to_string(v); };
  const int unis = w.stats.groupCount[GROUP_UNIVERSITY], schools = w.stats.groupCount[GROUP_SCHOOL];
  int darkHomes = 0, jammed = 0; std::string jamWhere;
  for(const Construction* c : w.map.constructions) {
    const int g = c->constructionGroup->group;
    if(GROUP_IS_RESIDENCE(g) && !(c->flags & FLAG_HAD_POWER)) ++darkHomes;
    if(g == GROUP_INDUSTRY_L || g == GROUP_INDUSTRY_H || g == GROUP_PORT || g == GROUP_WAREHOUSE) {
      bool linked = false; for(const Construction* n : c->neighbors) linked |= bool(n->flags & FLAG_IS_TRANSPORT);
      if(!linked && !jammed++) jamWhere = c->constructionGroup->name + " at " + num(c->point.x) + "," + num(c->point.y);
    }
  }
  if(!homes && !residents) {
    add(1, "First settlement", "Nobody lives here yet. Place housing, a water well, a market and a farm, and join them with roads.");
    return out;
  }
  // 1. Skilled workforce.
  if(campuses > 0 && campusSpeed() < 1000) {
    const bool engShort = engCoverage <= techCoverage;
    const std::string who = engShort ? "engineers" : "technicians";
    const int have = engShort ? engineers : technicians, need = engShort ? engNeed : techNeed;
    const int perMonth = engShort ? engTrainedMonth : techTrainedMonth;
    const int cap = engShort ? residents : residents / RESIDENTS_PER_TECH; // engineers: no housing limit
    const std::string head = "Your " + num(campuses) + " technology campus(es) work at " + num(campusSpeed()/10)
      + "% speed: " + num(have) + " " + who + " live here but " + num(need) + " are needed. ";
    const int trainers = engShort ? unis : schools, working = engShort ? universitiesWorking : schoolsWorking;
    if(!trainers)
      add(2, "No one is training " + who, head + (engShort ? "Build a university" : "Build a school") + " and keep it supplied with workers and goods.");
    else if(!working)
      add(2, "Training has stopped", head + (engShort ? "Your university" : "Your school") + " is built but did no work last month. It is missing workers or goods: check its roads.");
    else if(approval < 40)
      add(2, "Skilled people are leaving", head + "Approval is only " + num(approval) + "%, so " + num(skilledLeftMonth) + " skilled people left last month. Another "
        + (engShort ? "university" : "school") + " would not help. Fix food, power and housing first.");
    else if(!engShort && have >= cap && cap < need)
      add(1, "No room for more " + who, head + "A city of " + num(residents) + " people only has room for about " + num(cap)
        + ". Build more housing, not more " + (engShort ? "universities." : "schools."));
    else {
      const int months = monthsToReach(have, need, perMonth, cap, lossPermille(approval));
      add(1, "Staff are still being trained", head + (months > 0 ? "At +" + num(perMonth) + " a month the gap closes in about " + num(months) + " months. Be patient."
        : "Training is too slow to ever close the gap. Add a second " + std::string(engShort ? "university." : "school.")));
    }
  } else if(campuses > 0) {
    add(0, "Campuses fully staffed", "All " + num(campuses) + " technology campus(es) have enough engineers (" + num(engineers) + ") and technicians (" + num(technicians) + ").");
  }
  if((lightFactories || heavyFactories) && industrySpeed() < 1000) {
    const int fac = lightFactories + heavyFactories;
    add(1, "Factories run slowly", num(fac) + " factory(ies) work at " + num(industrySpeed()/10) + "% speed. They still run without skilled staff, but slowly. "
      "Your city has " + num(technicians) + " technicians and " + num(engineers) + " engineers; about " + num(techNeed) + " and " + num(engNeed) + " are needed. "
      + (schools ? "Schools add about " + num(techTrainedMonth) + " technicians a month." : "Build a school to train technicians.")
      + (unis ? "" : " A university trains the engineers."));
  }
  if(unis > 0 && campuses == 0 && engineers > 0)
    add(0, "Engineers without work", num(engineers) + " engineers live here but no advanced employer exists. Build a technology campus before they move away.");
  if(schools > 0 && !unis && w.tech_level >= MAX_TECH_LEVEL / 4)
    add(0, "Time for a university", "Your technology level is high enough. Without a university you cannot train the engineers a technology campus needs.");
  if(!schools)
    add(1, "No schools", "Your " + num(residents) + " residents have no school. Technicians come from schools; without them no factory or campus can grow.");
  // 2. Basic needs and logistics.
  if(darkHomes) add(2, "Homes without power", num(darkHomes) + " of " + num(homes) + " housing buildings have no electricity. Check generators, power lines and substations.");
  if(jammed) add(1, "Cut-off industry", num(jammed) + " industrial building(s) touch no road, track or rail, first one: " + jamWhere + ". Connect it or its goods cannot leave.");
  if(approval < 50) add(1, "People are unhappy", "Approval is " + num(approval) + "%. Unhappy cities also lose skilled workers.");
  for(size_t i = 0; i < districts.size() && i < districtStats.size(); ++i) {
    const District& d = districts[i]; const DistrictStats& s = districtStats[i];
    const std::string who = d.name + " (" + districtTypeName(d.type) + " district)";
    if(!s.buildings) { add(0, "Empty district", who + " has no " + std::string(districtTypeName(d.type)) + " buildings yet."); continue; }
    if(s.buildings >= 3 && s.fit < 50 && s.bestType && s.bestType != d.type)
      add(0, "District fits another type", who + " is only " + num(s.fit) + "% " + districtTypeName(d.type) + "; its buildings look more like a " + districtTypeName(s.bestType) + " district.");
    if(d.type == DIST_INDUSTRIAL && s.factories == 1) add(0, "Cluster needs more factories", who + " has 1 factory. A second one switches on the cluster bonus (+20% speed).");
    if(d.type == DIST_TECH && s.campuses && !(s.universities + s.schools)) add(1, "No research bonus", who + " has a campus but no school or university inside it.");
    if(d.type == DIST_UNIVERSITY && !(s.universities + s.schools)) add(0, "No training here", who + " has no school or university to train people.");
    if(d.type == DIST_FARMING && s.backedUpFarms) add(1, "Harvest cannot leave", num(s.backedUpFarms) + " of " + num(s.farms) + " farms in " + who + " are full ("
      + num(s.backedUpFarms * 100 / std::max(1, s.farms)) + "% of the harvest is at risk). Connect them to roads or rail and add a market or warehouse.");
    else if(d.type == DIST_FARMING && s.farms && !s.warehouses) add(0, "No storage", who + " has " + num(s.farms) + " farms but no warehouse. Storage and freight links keep the harvest from spoiling.");
    if(d.type == DIST_TRADE && !(s.ports + s.warehouses)) add(1, "No trade facility", who + " needs a port or a warehouse to trade.");
  }
  if(out.empty()) add(0, "All quiet", "No serious problem found this month.");
  std::stable_sort(out.begin(), out.end(), [](const Advice& a, const Advice& b) { return a.severity > b.severity; });
  return out;
}
