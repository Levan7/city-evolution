// SPDX-License-Identifier: GPL-2.0-or-later
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/world.hpp"
#include <stdexcept>
#include <iostream>
#include <filesystem>
static void check(bool b,const char*m){if(!b)throw std::runtime_error(m);}
int main(){try{initializeModules();auto w=new_barren_city(40);w->buildConstruction(residenceLLConstructionGroup,{10,10});auto* r=static_cast<Residence*>(w->map({10,10})->reportingConstruction);r->local_population=100;w->evolution.mayorMode=0;w->total_money=500;w->money_rates.income_tax=8;w->civic.autoTax=true;w->civic.adjustTaxes(*w);check(w->money_rates.income_tax==9,"independent auto tax did not adjust");for(int i=0;i<20;++i){w->total_time=i;w->civic.adjustTaxes(*w);}check(w->money_rates.income_tax==9,"repeat clicks changed tax within same month");w->total_time=100;w->civic.adjustTaxes(*w);check(w->money_rates.income_tax==10,"next monthly review missing");check(w->money_rates.goods_tax==1,"auto tax changed goods tax");check(w->civic.budget[0]==100,"auto tax changed service funding");int day=w->total_time;w->evolution.pedestrians=!w->evolution.pedestrians;w->evolution.changePlanFocus(*w);check(w->total_time==day,"UI state operation advanced date");auto file=std::filesystem::temp_directory_path()/"finance16.scn.gz";w->save(file);auto loaded=World::load(file);std::filesystem::remove(file);check(loaded->civic.taxReviewedDay==100,"tax review cooldown lost on save/load");check(!w->civic.financialAdvice(*w).empty(),"missing advisor");std::cout<<"PASS tax independence, one review/month, goods/funding unchanged, UI state/date separation, save/load and advisor\n";}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
