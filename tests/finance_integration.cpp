// SPDX-License-Identifier: GPL-2.0-or-later
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/world.hpp"
#include <stdexcept>
#include <iostream>
#include <filesystem>
static void check(bool b,const char*m){if(!b)throw std::runtime_error(m);}
int main(){try{initializeModules();auto w=new_barren_city(40);w->buildConstruction(residenceLLConstructionGroup,{10,10});auto* r=static_cast<Residence*>(w->map({10,10})->reportingConstruction);r->local_population=100;w->evolution.mayorMode=0;w->total_money=500;w->money_rates.income_tax=8;w->civic.lastLaborTaxBase=10000;w->civic.autoTax=true;w->civic.adjustTaxes(*w);check(w->money_rates.income_tax==9,"independent auto tax did not adjust");for(int i=0;i<20;++i){w->total_time=i;w->civic.adjustTaxes(*w);}check(w->money_rates.income_tax==9,"repeat clicks changed tax within same month");w->total_time=100;w->civic.adjustTaxes(*w);check(w->money_rates.income_tax==10,"next monthly review missing");check(w->money_rates.goods_tax==1,"auto tax changed goods tax");check(w->civic.budget[0]==100,"auto tax changed service funding");int day=w->total_time;w->evolution.pedestrians=!w->evolution.pedestrians;w->evolution.changePlanFocus(*w);check(w->total_time==day,"UI state operation advanced date");auto file=std::filesystem::temp_directory_path()/"finance16.scn.gz";w->save(file);auto loaded=World::load(file);std::filesystem::remove(file);check(loaded->civic.lastLaborTaxBase==10000,"tax base lost on save/load");check(loaded->civic.taxReviewedDay==100,"tax review cooldown lost on save/load");check(!w->civic.financialAdvice(*w).empty(),"missing advisor"); { // "Auto-adjust taxes" acts at once, in any month, by the size of the gap, and explains itself.
  auto tw=new_barren_city(40);tw->buildConstruction(residenceLLConstructionGroup,{10,10});
  auto* home=static_cast<Residence*>(tw->map({10,10})->reportingConstruction);home->local_population=50;
  auto& c=tw->civic;auto& r=tw->money_rates;
  r.income_tax=25;c.taxCeiling=15;c.taxReviewedDay=tw->total_time; // already reviewed this month
  std::string note=c.applyRecommendedTax(*tw);
  check(r.income_tax==15&&note.find("ceiling")!=std::string::npos,"pressing auto tax must act immediately even after a review this month, and respect the ceiling");
  c.hasMonthlyHistory=true;c.lastMonth={1000,1500,1000};c.lastLaborTaxBase=10000;r.income_tax=10;c.taxCeiling=30; // deficit 500, 100 per point
  check(c.recommendedTax(*tw)==15,"a deficit of 500 at 100 per point needs five more points");
  c.taxCeiling=12;std::string why;check(c.recommendedTax(*tw,&why)==12 && why.find("$300/month")!=std::string::npos,"advisor should explain the remaining deficit at the ceiling");c.taxCeiling=30;
  tw->total_money=10000000;c.lastMonth={3000,1000,3000};c.lastLaborTaxBase=25000;r.income_tax=12; // surplus 2000, 250 per point
  check(c.recommendedTax(*tw)==8,"a large surplus with plenty of cash lowers the rate (never below 8)");
  tw->total_money=100;check(c.recommendedTax(*tw)==13,"cash below the reserve nudges the rate up one point");
  tw->total_money=10000000;c.lastMonth={1000,1500,1000};c.lastLaborTaxBase=10000;r.income_tax=10;
  c.autoTax=true;c.taxReviewedDay=-1;c.adjustTaxes(*tw);check(r.income_tax==11,"monthly automation moves one point toward the recommendation");
  c.adjustTaxes(*tw);check(r.income_tax==11,"only one review per month");
  tw->total_time+=NUMOF_DAYS_IN_MONTH;c.adjustTaxes(*tw);check(r.income_tax==12&&!c.taxNote.empty(),"next month moves one more point and records why");
  auto ew=new_barren_city(40);ew->money_rates.income_tax=20;ew->civic.taxCeiling=15;
  check(ew->civic.recommendedTax(*ew)==15,"with no tax base only the ceiling applies");ew->money_rates.income_tax=5;check(ew->civic.recommendedTax(*ew)==5,"with no tax base the rate is left alone");
  auto actual = new_barren_city(40);
  actual->total_time = NUMOF_DAYS_IN_MONTH - 2;
  actual->total_money = 100000;
  actual->taxable.labor = 10000;
  actual->money_rates.income_tax = 20;
  actual->do_time_step();
  check(actual->total_money == 102000 && actual->stats.income.income_tax.acc == -2000,
        "tax must increase cash and be stored as a credit");
  actual->do_time_step();
  check(actual->civic.monthlyFiscal(*actual).revenue == 2000
        && actual->civic.lastLaborTaxBase == 10000, "advisor must read actual monthly credits");
  check(actual->civic.fiscal(*actual).revenue == 0, "monthly posting changed last year's total");
  check(actual->total_money == 102000 && actual->taxable.labor == 0,
        "rolling over the month double-counted taxes");
  auto save = std::filesystem::temp_directory_path()/"finance-monthly.scn.gz";
  actual->income(17,actual->stats.income.goods_tax);
  actual->save(save);
  auto restored=World::load(save);
  std::filesystem::remove(save);
  check(restored->civic.monthToDate.revenue==17 && restored->civic.lastMonth.revenue==2000
        && restored->civic.hasMonthlyHistory,"monthly tax ledger lost on save/load");
  actual->total_money = -100;
  actual->civic.budget.fill(5);
  actual->civic.adjustFunding(*actual, true);
  check(actual->civic.budget[CivicServices::EDUCATION]==75,"auto funding should apply immediately");
  auto& education = actual->civic.budget[CivicServices::EDUCATION];
  education=5;
  int spent=0;
  for(int i=0;i<100;++i) spent+=actual->civic.cost(CivicServices::EDUCATION,2,i,7);
  check(spent==10,"school upkeep should average 5% across 100 days");
  education=200;
  check(actual->civic.cost(CivicServices::EDUCATION,2,0,7)==4,"200% should double school upkeep");
  actual->tech_level=MAX_TECH_LEVEL;
  actual->buildConstruction(schoolConstructionGroup,{10,10});
  check(actual->stats.groupCount[GROUP_SCHOOL]==1,"school should build with a negative balance");
  actual->people_pool=25;
  check(actual->evolution.advise(*actual).front().title=="Housing shortage",
        "unhoused residents should be reported independently of unemployment");
  std::cout<<"PASS auto tax: immediate apply, deficit/surplus/cash rules, ceiling, monthly step\n";
 }
 std::cout<<"PASS tax independence, one review/month, goods/funding unchanged, UI state/date separation, save/load and advisor\n";}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
