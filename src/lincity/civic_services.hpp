// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <map>
#include <string>
#include <vector>
class World; class Construction;
struct CivicServices {
 enum Service { EDUCATION, HEALTH, FIRE, POLICE };
 enum Kind { FIRE_CALL, MEDICAL_CALL, CRIME_CALL, ZOMBIE_CALL };
 struct Incident { int id,kind,target,severity,age; };
 struct Unit { int kind,home,call,step=0,credit=0,working=0; bool returning=false; std::vector<int> path; };
 std::array<int,4> budget{100,100,100,100};
 bool autoBudget=false,autoTax=false,zombiesEnabled=true,zombieNoticePending=false;
 int taxReviewedDay=-1;
 int taxCeiling=15,reserve=25000,education=0,crime=0,sickTotal=0,exposureMonths=0,zombieCooldown=0,nextId=1,resolved=0,forceBurn=-1;
 unsigned rng=728391;
 std::map<int,int> sick; std::vector<Incident> incidents; std::vector<Unit> units; std::string pendingLoad;
 bool funded(int s,int day,int seed) const;
 int cost(int s,int base) const;
 int illAt(int id) const;
 bool burningAt(int id) const;
 bool fireResponseAt(int id) const;
 bool beginFire(World&,Construction&);
 bool addIncident(int kind,int target,int severity);
 std::vector<int> route(const World&,int home,int target) const;
 int terrainCost(const World&,int id,int home,int target) const;
 bool stationReady(const Construction&,int service) const;
 void adjustTaxes(World&); std::string financialAdvice(const World&) const;
 void update(World&); void monthly(World&);
 std::string save() const; void load(const std::string&,int side);
 unsigned random(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
};
