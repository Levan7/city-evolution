// SPDX-License-Identifier: GPL-2.0-or-later
// The mayor's five-year plan. Deterministic: no randomness, integers only.
#include "city_evolution.hpp"
#include "world.hpp"
#include "lintypes.hpp"
#include "modules/city_business.hpp"
#include "modules/all_modules.hpp"
#include "modules/school.hpp"
#include "modules/university.hpp"
#include "modules/health_centre.hpp"
#include "modules/parkland.hpp"

namespace {
std::string n(int v) { return std::to_string(v); }
std::string money(int v) {
  std::string d = std::to_string(v < 0 ? -v : v), out;
  for(size_t i = 0; i < d.size(); ++i) { if(i && (d.size() - i) % 3 == 0) out += ','; out += d[i]; }
  return std::string(v < 0 ? "-$" : "$") + out;
}
const char* focusName(int f) { return f == 1 ? "Livable city" : f == 2 ? "Skills and education" : f == 3 ? "Technology hub" : "None"; }

struct Counts { int homes, schools, health, parks, unis, campuses; };
Counts countNow(World& w) {
  Counts c{0, int(w.stats.groupCount[GROUP_SCHOOL]), int(w.stats.groupCount[GROUP_HEALTH]), int(w.stats.groupCount[GROUP_PARKLAND]),
           int(w.stats.groupCount[GROUP_UNIVERSITY]), int(w.stats.groupCount[GROUP_TECH_CAMPUS])};
  for(auto* k : w.map.constructions) c.homes += GROUP_IS_RESIDENCE(k->constructionGroup->group);
  return c;
}
int costOf(World& w, ConstructionGroup& g) { return g.getCosts(w); }
} // namespace

void CityEvolution::proposePlan(World& w, int forced) {
  const Counts c = countNow(w);
  const int cap = residents, techCap = residents / RESIDENTS_PER_TECH; // engineers have no housing limit
  const int months = PLAN_YEARS * 12;
  int focus = forced;
  if(!focus) focus = (c.unis >= 1 && c.schools >= 2 && w.tech_level >= techCampusConstructionGroup.tech) ? FOCUS_TECH
                   : c.homes >= 8 ? FOCUS_SKILLS : FOCUS_LIVABLE;
  std::string why, note;
  tgtSchools = c.schools; tgtHealth = c.health; tgtParks = c.parks; tgtUniversities = c.unis; tgtCampuses = c.campuses;
  tgtApproval = 0;
  if(focus == FOCUS_LIVABLE) {
    tgtSchools = std::max(c.schools, std::max(1, c.homes / 8));
    tgtHealth = std::max(c.health, c.homes >= 16 ? 1 : 0);
    tgtParks = std::max(c.parks, std::max(1, c.homes / 8));
    tgtApproval = 70;
    why = "Your city has " + n(c.homes) + " housing buildings and is still small. Good basics (school, clinic, parks) come before industry.";
  } else if(focus == FOCUS_SKILLS) {
    tgtSchools = std::max(c.schools, std::min(c.homes / 6 + 1, c.schools + 3));
    tgtUniversities = std::max(c.unis, 1);
    tgtHealth = std::max(c.health, c.homes >= 16 ? 1 : 0);
    tgtApproval = 60;
    why = "You have " + n(c.schools) + " school(s) and " + n(c.unis) + " university(ies). Advanced industry needs trained people first, and training takes years.";
  } else { // technology hub
    tgtSchools = std::max(c.schools, 2);
    tgtUniversities = std::max(c.unis, 1);
    tgtHealth = std::max(c.health, c.homes >= 16 ? 1 : 0);
    tgtApproval = 60;
    tgtCampuses = c.campuses + 1;
    why = "You already have a university and schools, so a technology campus can now employ your graduates.";
  }
  // Skilled-worker goals come from a real projection that uses the same monthly rules as the game.
  // A new school takes about 6 months to start working, a new university about 12.
  const int engCap = std::max(cap, engineers), techCapV = std::max(techCap, technicians);
  const int uniDelay = tgtUniversities > c.unis ? 12 : 0, schoolDelay = tgtSchools > c.schools ? 6 : 0;
  const int projEng = projectSkilled(projectSkilled(engineers, c.unis * ENGINEERS_PER_UNI_MONTH, uniDelay, engCap),
                                     tgtUniversities * ENGINEERS_PER_UNI_MONTH, months - uniDelay, engCap);
  const int projTech = projectSkilled(projectSkilled(technicians, c.schools * TECHS_PER_SCHOOL_MONTH, schoolDelay, techCapV),
                                      tgtSchools * TECHS_PER_SCHOOL_MONTH, months - schoolDelay, techCapV);
  // Goals cover what existing factories and campuses already need, plus the planned new campuses.
  const int newCampuses = std::max(0, tgtCampuses - c.campuses);
  tgtEngineers = focus == FOCUS_LIVABLE ? engineers : std::min(projEng, std::max(engineers, engNeed + newCampuses * CAMPUS_ENGINEERS));
  tgtTechnicians = focus == FOCUS_LIVABLE ? technicians : std::min(projTech, std::max(technicians, techNeed + newCampuses * CAMPUS_TECHS));
  if(focus != FOCUS_LIVABLE && (lightFactories + heavyFactories) && industrySpeed() < 1000)
    note += " Your " + n(lightFactories + heavyFactories) + " factory(ies) run at " + n(industrySpeed() / 10) + "% speed for lack of skilled staff; more training lifts them.";
  // Cost: every missing building at today's price, plus 20% for basics the mayor adds along the way.
  int est = 0;
  est += std::max(0, tgtSchools - c.schools) * costOf(w, schoolConstructionGroup);
  est += std::max(0, tgtHealth - c.health) * costOf(w, healthCentreConstructionGroup);
  est += std::max(0, tgtParks - c.parks) * costOf(w, parklandConstructionGroup);
  est += std::max(0, tgtUniversities - c.unis) * costOf(w, universityConstructionGroup);
  est += std::max(0, tgtCampuses - c.campuses) * costOf(w, techCampusConstructionGroup);
  est = est + est / 5;
  planEstimate = est;
  planBudgetYear = std::clamp(((est / PLAN_YEARS) / 1000 + 1) * 1000, 5000, MAX_PLAN_BUDGET);
  if(w.total_money < est / 2)
    note += " Money warning: the plan costs about " + money(est) + " over five years and you have " + money(w.total_money) + ". Taxes must fund the rest, or the mayor will build slowly.";
  baseSchools = c.schools; baseHealth = c.health; baseParks = c.parks; baseUniversities = c.unis; baseCampuses = c.campuses;
  baseTechnicians = technicians; baseEngineers = engineers; baseApproval = approval;
  planFocus = focus; planState = PLAN_PROPOSED;
  planWhy = why + note;
  ++planNumber;
  headline("Mayor proposes five-year plan " + n(planNumber) + ": " + focusName(focus) + ". Open City Hall > Mayor to decide.");
}

void CityEvolution::approvePlan(World& w) {
  if(planState != PLAN_PROPOSED) return;
  planState = PLAN_ACTIVE; planStartDay = w.total_time; planSpent = 0;
  planBudgetBefore = mayorBudget; planModeBefore = mayorMode;
  if(mayorMode == 0) mayorMode = 2; // approving a plan authorises the mayor to carry it out
  mayorBudget = planBudgetYear;
  headline("Plan " + n(planNumber) + " approved: " + focusName(planFocus) + ", budget " + money(planBudgetYear) + " a year until year "
    + n(w.total_time / NUMOF_DAYS_IN_YEAR + PLAN_YEARS) + ".");
}

void CityEvolution::rejectPlan(World& w) {
  if(planState != PLAN_PROPOSED) return;
  planState = PLAN_REJECTED; planNextProposalDay = w.total_time + NUMOF_DAYS_IN_YEAR;
  headline("Plan " + n(planNumber) + " rejected. The mayor will bring a new proposal in one year.");
}

void CityEvolution::changePlanFocus(World& w) {
  if(planState != PLAN_PROPOSED) return;
  const int next = planFocus % 3 + 1;
  --planNumber; // same plan, different focus
  proposePlan(w, next);
}

void CityEvolution::adjustPlanBudget(World& w, int delta) {
  if(planState != PLAN_PROPOSED && planState != PLAN_ACTIVE) return;
  planBudgetYear = std::clamp(planBudgetYear + delta, 5000, MAX_PLAN_BUDGET);
  if(planState == PLAN_ACTIVE) mayorBudget = planBudgetYear;
}

// Which building should the plan add next? Order matters: people first, campus last.
bool CityEvolution::planNextBuild(World& w, ConstructionGroup*& group, std::string& reason) {
  if(planState != PLAN_ACTIVE) return false;
  const Counts c = countNow(w);
  const int yearsIn = (w.total_time - planStartDay) / NUMOF_DAYS_IN_YEAR;
  std::string blocked; // things the plan wants but your switches forbid
  auto fail = [&](const std::string& why) { reason = blocked.empty() ? why : why + "; also held back by your switches: " + blocked; return false; };
  if(c.schools < tgtSchools) {
    if(mayorServices) { group = &schoolConstructionGroup; reason = "plan: more schools (" + n(c.schools) + " of " + n(tgtSchools) + ")"; return true; }
    blocked += "schools (Basic services are blocked) ";
  }
  if(c.health < tgtHealth) {
    if(mayorServices) { group = &healthCentreConstructionGroup; reason = "plan: health coverage"; return true; }
    blocked += "a clinic (Basic services are blocked) ";
  }
  if(c.unis < tgtUniversities) {
    if(mayorServices) { group = &universityConstructionGroup; reason = "plan: a university to train engineers"; return true; }
    blocked += "a university (Basic services are blocked) ";
  }
  if(c.parks < tgtParks) {
    if(mayorParks) { group = &parklandConstructionGroup; reason = "plan: green space"; return true; }
    blocked += "parks (Parks are blocked) ";
  }
  if(c.campuses < tgtCampuses) {
    // Do not build a campus that would stand empty: wait until half of its engineers exist (or year 3).
    if(engineers * 2 >= CAMPUS_ENGINEERS || yearsIn >= 3) { group = &techCampusConstructionGroup; reason = "plan: technology campus"; return true; }
    return fail("plan: waiting for engineers (" + n(engineers) + " of " + n(CAMPUS_ENGINEERS / 2) + " needed) before building the campus");
  }
  if(!blocked.empty()) return fail("plan: nothing it may build right now");
  return false;
}

void CityEvolution::updatePlan(World& w) {
  const Counts c = countNow(w);
  if(planState == PLAN_PROPOSED && planAuto && mayorMode != 0) approvePlan(w); // 'decides alone' still needs the mayor ON
  if((planState == PLAN_IDLE || (planState == PLAN_REJECTED && w.total_time >= planNextProposalDay)) && c.homes >= 4 && residents > 0) {
    if(planState == PLAN_REJECTED) planState = PLAN_IDLE;
    proposePlan(w);
    if(planAuto && mayorMode != 0) approvePlan(w);
    return;
  }
  if(planState != PLAN_ACTIVE || w.total_time < planStartDay + PLAN_YEARS * NUMOF_DAYS_IN_YEAR) return;
  // ---- Five years are over: judge the plan against its own goals. ----
  struct Goal { const char* name; int have, want, from; };
  const Goal goals[] = {
    {"schools", c.schools, tgtSchools, baseSchools}, {"clinics", c.health, tgtHealth, baseHealth}, {"parks", c.parks, tgtParks, baseParks},
    {"universities", c.unis, tgtUniversities, baseUniversities}, {"campuses", c.campuses, tgtCampuses, baseCampuses},
    {"technicians", technicians, tgtTechnicians, baseTechnicians}, {"engineers", engineers, tgtEngineers, baseEngineers},
    {"approval %", approval, tgtApproval, -1}};
  int total = 0, met = 0; std::string detail;
  for(const Goal& g : goals) {
    if(g.want <= g.from) continue; // the plan asked for nothing new here
    ++total; const bool ok = g.have >= g.want; met += ok;
    detail += std::string(detail.empty() ? "" : "; ") + g.name + " " + n(g.have) + "/" + n(g.want) + (ok ? " ok" : " MISSED");
  }
  const bool onBudget = planSpent <= planEstimate + planEstimate / 4;
  const char* verdict = met == total ? "SUCCESS" : met * 2 >= total ? "PARTLY ACHIEVED" : "FAILED";
  planResult = std::string("Plan ") + n(planNumber) + " (" + focusName(planFocus) + "): " + verdict + ". " + n(met) + " of " + n(total)
    + " goals met. " + detail + ". Spent " + money(planSpent) + " of " + money(planEstimate) + (onBudget ? " (within budget)." : " (over budget).");
  ++plansDone; plansSucceeded += met == total;
  headline("Five-year review: " + planResult);
  // Finishing a plan never gives the mayor more authority than you left him with.
  mayorBudget = planBudgetBefore ? planBudgetBefore : 10000; mayorMode = std::min(mayorMode, planModeBefore);
  planState = PLAN_IDLE; planNextProposalDay = w.total_time;
}

std::vector<std::string> CityEvolution::planLines(World& w) const {
  std::vector<std::string> L;
  const Counts c = countNow(const_cast<World&>(w));
  if(planState == PLAN_IDLE || planState == PLAN_REJECTED) {
    L.push_back(planState == PLAN_REJECTED ? "Plan rejected. The mayor will propose a new one in about a year." : "No plan yet. The mayor proposes one once you have a few homes.");
    if(!planResult.empty()) L.push_back("Last review: " + planResult);
    return L;
  }
  const int year = planState == PLAN_ACTIVE ? std::min(PLAN_YEARS, (w.total_time - planStartDay) / NUMOF_DAYS_IN_YEAR + 1) : 0;
  L.push_back(std::string(planState == PLAN_PROPOSED ? "PROPOSED PLAN " : "ACTIVE PLAN ") + n(planNumber) + ": " + focusName(planFocus)
    + (planState == PLAN_ACTIVE ? " (year " + n(year) + " of " + n(PLAN_YEARS) + ")" : ""));
  L.push_back("Why: " + planWhy);
  std::string goals;
  auto add = [&](const char* name, int have, int want, int from) { if(want > from) goals += std::string(goals.empty() ? "" : ", ") + name + " " + n(have) + " -> " + n(want); };
  add("schools", c.schools, tgtSchools, baseSchools); add("clinics", c.health, tgtHealth, baseHealth); add("parks", c.parks, tgtParks, baseParks);
  add("universities", c.unis, tgtUniversities, baseUniversities); add("campuses", c.campuses, tgtCampuses, baseCampuses);
  add("technicians", technicians, tgtTechnicians, baseTechnicians); add("engineers", engineers, tgtEngineers, baseEngineers);
  if(tgtApproval > baseApproval) goals += std::string(goals.empty() ? "" : ", ") + "approval " + n(approval) + "% -> " + n(tgtApproval) + "%";
  L.push_back("Goals in 5 years: " + (goals.empty() ? std::string("keep everything running well") : goals));
  L.push_back("Cost: about " + money(planEstimate) + " in total, " + money(planBudgetYear) + " a year. "
    + (planState == PLAN_ACTIVE ? "Spent so far: " + money(planSpent) + "." : "Nothing is built until you approve."));
  L.push_back(planState == PLAN_PROPOSED ? "Focus button: Livable city (basics), Skills and education (schools, universities) or Technology hub (campus). Goals and cost change with it." : "The focus is locked while the plan runs; choose another after the five-year review.");
  return L;
}
