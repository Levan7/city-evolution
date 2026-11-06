// SPDX-License-Identifier: GPL-2.0-or-later
// City Evolution additions, GPL-2.0-or-later.
#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
class World;
class ConstructionGroup;
// One plain-language message from a city advisor. severity: 0 note, 1 warning, 2 urgent.
struct Advice { int severity; std::string title, text; };
struct CityEvolution {
  // ---- Skilled workforce (people, not buildings) -------------------------------
  // Technicians are trained by working schools, engineers by working universities.
  // Training is gradual: one school adds about 6 technicians a month, one university
  // about 5 engineers a month (scaled by how many days it really worked).
  static constexpr int TECHS_PER_SCHOOL_MONTH = 6, ENGINEERS_PER_UNI_MONTH = 5;
  static constexpr int CAMPUS_TECHS = 20, CAMPUS_ENGINEERS = 40;
  // Only technicians are limited by housing (1 per 10 residents). Engineers have no housing limit.
  static constexpr int RESIDENTS_PER_TECH = 10;
  // Factories work without skilled staff, but slowly: 25% speed unprepared, 100% when fully staffed.
  static constexpr int FACTORY_L_TECHS = 6, FACTORY_L_ENGINEERS = 1, FACTORY_H_TECHS = 10, FACTORY_H_ENGINEERS = 3;
  static constexpr int FACTORY_UNSKILLED_SPEED = 250; // per mille
  // Skilled people retire slowly (3 per 1000 a month, about a 28-year career). If approval is below 40%
  // 30 more per 1000 a month emigrate. Fractions carry over, so small groups lose people too.
  static constexpr int RETIRE_PERMILLE = 3, EMIGRATE_PERMILLE = 30;
  static int lossPermille(int approval) { return RETIRE_PERMILLE + (approval < 40 ? EMIGRATE_PERMILLE : 0); }
  // Forecast with exactly the monthly rules of updateWorkforce: loss first, then new graduates, then the cap.
  static int projectSkilled(int start, int perMonth, int months, int cap, int ratePermille = RETIRE_PERMILLE) {
    int v = start, carry = 0;
    for(int m = 0; m < months; ++m) { const int tot = v * ratePermille + carry; carry = tot % 1000; v = std::min(cap, std::max(0, v + perMonth - tot / 1000)); }
    return v;
  }
  // Months until `have` reaches `need`, or -1 if training can never beat retirement within 50 years.
  static int monthsToReach(int have, int need, int perMonth, int cap, int ratePermille = RETIRE_PERMILLE) {
    if(have >= need) return 0;
    int v = have, carry = 0;
    for(int m = 1; m <= 600; ++m) { const int tot = v * ratePermille + carry; carry = tot % 1000; v = std::min(cap, std::max(0, v + perMonth - tot / 1000)); if(v >= need) return m; }
    return -1;
  }
  int techCarry = 0, engCarry = 0;          // fractions of a person (per 1000) not yet lost
  int technicians = 0, engineers = 0;      // skilled people living in the city
  int techNeed = 0, engNeed = 0, campuses = 0;
  int techCoverage = 1000, engCoverage = 1000; // per mille of the need that is staffed
  int churchesWorking=0,spiritualCoverage=0;
  int residents = 0, schoolsWorking = 0, universitiesWorking = 0, lightFactories = 0, heavyFactories = 0;
  int techTrainedMonth = 0, engTrainedMonth = 0, skilledLeftMonth = 0;
  int campusSpeed() const { return std::min(techCoverage, engCoverage); }   // per mille
  // Factories: 25% speed with no skilled staff, rising to 100% as the staff they need appear.
  int industrySpeed() const { return FACTORY_UNSKILLED_SPEED + (1000 - FACTORY_UNSKILLED_SPEED) * campusSpeed() / 1000; }
  // A machine running at `speed` per mille works that share of its ready days (deterministic).
  static bool skilledDay(int& credit, int speed) {
    credit = std::min(credit + speed, 2000);
    if(credit < 1000) return false;
    credit -= 1000; return true;
  }
  std::vector<Advice> advise(World& world) const;

  // ---- The mayor's mandate: where he works, how fast, and how much he may spend -------
  // mayorMode: 0 = OFF (he only advises and builds nothing), 2 = ON (builds inside his area and budget).
  // (mayorMode 1 is the old square "district manager", kept so old cities still load.)
  bool fullAutonomy = false;
  void setFullAutonomy(World&,bool);
  int autonomousStep(World&,bool,bool,bool&,std::string&);
  int mayorScope = 0;      // 0 whole city, 1 one district, 2 a zone drawn on the map
  int mayorDistrict = -1;  // index into `districts` when mayorScope == 1
  int mayorX0 = 0, mayorY0 = 0, mayorX1 = 0, mayorY1 = 0; // zone when mayorScope == 2 (inclusive)
  int mayorPace = 2;       // 1 careful, 2 normal, 3 fast
  int mayorBuiltMonth = 0, mayorRoadsMonth = 0;
  std::string cityName;
  int mayorBuildingsPerMonth() const { static const int v[] = {1, 3, 6}; return v[std::clamp(mayorPace, 1, 3) - 1]; }
  int mayorRoadTilesPerMonth() const { static const int v[] = {4, 12, 30}; return v[std::clamp(mayorPace, 1, 3) - 1]; }
  void mayorBounds(World& world, int& x0, int& y0, int& x1, int& y1) const; // upper bounds exclusive
  void setMayorScopeCity();
  void setMayorScopeDistrict(int index);
  void setMayorScopeZone(World& world, int x0, int y0, int x1, int y1);
  std::string mayorScopeText() const;
  int mayorStep(World& world, bool allowBuilding, bool allowRoad, bool& builtRoad, std::string& report);
  void renameCity(std::string name);

  // ---- Districts: zones the player draws on the map, types and names -----------------
  // A district is a rectangle (it may cover existing buildings). Its type says what the player
  // wants it to become. Matching buildings inside it form a cluster with real effects:
  //   Industrial: 2+ factories inside -> they run 20% faster (helps while staff are short).
  //   Technology: a campus with a school or university in the same district runs 20% faster.
  //   University: schools and universities inside train 25% more people.
  //   Farming / Trade: no speed effect yet; advisors check storage, roads and ports.
  enum DistrictType { DIST_INDUSTRIAL = 1, DIST_TECH = 2, DIST_FARMING = 3, DIST_TRADE = 4, DIST_UNIVERSITY = 5 };
  static constexpr int MAX_DISTRICTS = 12, DISTRICT_CLUSTER_BONUS = 200 /*per mille*/, UNIVERSITY_DISTRICT_TRAINING = 125 /*percent*/;
  struct District { int type = 0, x0 = 0, y0 = 0, x1 = 0, y1 = 0; std::string name; };
  struct DistrictStats {
    int buildings = 0, homes = 0, schools = 0, universities = 0, campuses = 0, factories = 0, farms = 0;
    int backedUpFarms = 0, ports = 0, warehouses = 0, markets = 0, fit = 0, bestType = 0;
  };
  std::vector<District> districts;
  std::vector<DistrictStats> districtStats; // recomputed each month, not saved
  static const char* districtTypeName(int type);
  int districtAt(int x, int y) const;                // index of the district covering a tile, or -1
  bool addDistrict(World& world, int type, int x0, int y0, int x1, int y1, std::string name); // false if invalid or full
  void removeDistrict(int index);
  void renameDistrict(int index, std::string name);
  void retypeDistrict(World& world, int index, int type);
  void updateDistricts(World& world);
  // Extra speed (per mille, multiplicative) a machine at (x,y) gets from its district. kind: 0 factory, 1 campus.
  int districtBonusAt(int x, int y, int kind) const;
  std::string describeDistrict(int index) const;     // one line for City Hall

  // ---- Five-year plan: the mayor's long-term strategy -------------------------------
  // The mayor proposes a plan (goals, cost, expected outcome). You approve, change focus or
  // budget, or reject it. An approved plan steers what the mayor builds. After five years
  // (5 x NUMOF_DAYS_IN_YEAR days) the plan is judged against its goals.
  enum PlanState { PLAN_IDLE = 0, PLAN_PROPOSED = 1, PLAN_ACTIVE = 2, PLAN_REJECTED = 3 };
  enum PlanFocus { FOCUS_LIVABLE = 1, FOCUS_SKILLS = 2, FOCUS_TECH = 3 };
  static constexpr int PLAN_YEARS = 5, MAX_PLAN_BUDGET = 2000000;
  int planState = 0, planFocus = 0, planNumber = 0, planStartDay = 0, planNextProposalDay = 0;
  int planBudgetYear = 0, planEstimate = 0, planSpent = 0, planBudgetBefore = 0, planModeBefore = 0, planAuto = 0;
  int tgtSchools = 0, tgtHealth = 0, tgtParks = 0, tgtUniversities = 0, tgtCampuses = 0;
  int tgtTechnicians = 0, tgtEngineers = 0, tgtApproval = 0;
  int baseSchools = 0, baseHealth = 0, baseParks = 0, baseUniversities = 0, baseCampuses = 0;
  int baseTechnicians = 0, baseEngineers = 0, baseApproval = 0;
  int plansDone = 0, plansSucceeded = 0;
  std::string planWhy, planResult;
  void updatePlan(World& world);                       // monthly: propose, adopt, evaluate
  void proposePlan(World& world, int forcedFocus = 0); // builds goals, cost and expected outcome
  void approvePlan(World& world);
  void rejectPlan(World& world);
  void changePlanFocus(World& world);                  // propose again with the next focus
  void adjustPlanBudget(World& world, int delta);
  bool planNextBuild(World& world, ConstructionGroup*& group, std::string& reason);
  std::vector<std::string> planLines(World& world) const; // text for the City Hall page
  // Real active-play milliseconds, deliberately independent of simulation days.
  int skyMode = 0; // 0 occasional nights, 1 permanent day, 2 permanent night
  int weatherMode = 0; // 0 changing, 1 clear, 2 clouds, 3 rain, 4 storm
  int skyPhase = 0; // daylight, dusk, night, dawn
  int skyElapsed = 0;
  int skyDuration = 720000;
  int weather = 0;
  int weatherElapsed = 0;
  int weatherDuration = 240000;
  int visualTime = 0;
  unsigned int randomState = 173831;
  int approval = 60, electionCount = 0, lastVote = 60;
  int nextElection = 4800;
  int poweredHomes = 0, homes = 0, amenities = 0;
  int operatingBusinesses = 0, newJobs = 0;
  bool pedestrians = true;
  int mayorMode = 0; // advice only, district manager, city manager
  int mayorBudget = 10000, mayorSpent = 0, mayorYear = -1;
  int districtX = 0, districtY = 0, mayorProjects = 0;
  bool mayorServices = true, mayorParks = true, mayorRoads = false;
  std::string mayorReport = "Advice only: no spending authority granted.";
  void runMayor(World& world);
  std::vector<std::string> news;
  unsigned int random() { randomState ^= randomState << 13; randomState ^= randomState >> 17; randomState ^= randomState << 5; return randomState; }
  void advance(int milliseconds) {
    milliseconds = std::clamp(milliseconds, 0, 250);
    visualTime = (visualTime + milliseconds) % 3600000;
    skyElapsed += milliseconds;
    if(skyElapsed >= skyDuration) {
      skyElapsed -= skyDuration;
      skyPhase = (skyPhase + 1) % 4;
      skyDuration = (skyPhase == 0) ? 480000 + random()%480001 :
        (skyPhase == 2) ? 180000 + random()%180001 : 45000;
    }
    weatherElapsed += milliseconds;
    if(weatherElapsed >= weatherDuration) {
      weatherElapsed -= weatherDuration;
      int roll = random()%100;
      weather = roll < 48 ? 0 : roll < 73 ? 1 : roll < 94 ? 2 : 3;
      weatherDuration = 180000 + random()%300001;
    }
  }
  int darkness() const {
    if(skyMode == 1) return 0;
    if(skyMode == 2) return 1000;
    if(skyPhase == 0) return 0;
    if(skyPhase == 2) return 1000;
    int fade = std::clamp(skyElapsed * 1000 / std::max(1, skyDuration), 0, 1000);
    return skyPhase == 1 ? fade : 1000 - fade;
  }
  int currentWeather() const { return weatherMode ? weatherMode - 1 : weather; }
  int solarPercent() const {
    const int sky = 100 - darkness()/10;
    const int cloud = currentWeather() == 0 ? 100 : currentWeather() == 1 ? 70 : currentWeather() == 2 ? 45 : 25;
    return sky * cloud / 100;
  }
  void headline(const std::string& text) {
    news.insert(news.begin(), text);
    if(news.size() > 8) news.resize(8);
  }
  void updateWorkforce(World& world);
  void updateCity(World& world);
};
