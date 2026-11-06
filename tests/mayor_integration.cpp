// SPDX-License-Identifier: GPL-2.0-or-later
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/world.hpp"
#include <iostream>
#include <stdexcept>
#include <filesystem>
int main(int argc,char**argv) {
 try {
  initializeModules();auto w=new_barren_city(50);w->total_money=200000;w->tech_level=MAX_TECH_LEVEL;
  for(int i=0;i<4;++i)w->buildConstruction(residenceLLConstructionGroup,MapPoint(2+i*4,2));
  for(int i=2;i<30;++i)w->buildConstruction(trackConstructionGroup,MapPoint(i,8));
  auto require=[](bool ok,const char* s){if(!ok)throw std::runtime_error(s);};
  w->total_money=200000;auto& e=w->evolution;int count=w->map.constructions.size(),money=w->total_money;
  e.runMayor(*w);require(int(w->map.constructions.size())==count&&w->total_money==money,"advice mode spent money");
  e.mayorMode=2;e.mayorBudget=50000;e.runMayor(*w);
  require(int(w->map.constructions.size())==count+1,"authorized mayor did not build service");
  require(w->total_money<money&&e.mayorSpent==money-w->total_money,"construction not charged to mayor budget");
  require(w->stats.groupCount[GROUP_WATERWELL]==1,"water need not addressed");
  require(w->total_money>=25000,"reserve breached");
  int funded=w->total_money;int structures=w->map.constructions.size();
  w->total_money=25000;e.runMayor(*w);require(int(w->map.constructions.size())==structures,"mayor breached cash reserve to start project");
  w->total_money=funded;e.mayorServices=false;e.mayorParks=true;e.runMayor(*w);
  require(w->stats.groupCount[GROUP_PARKLAND]==1,"permitted greenery project not built");
  e.mayorServices=true;
  int before=w->total_money;e.mayorBudget=e.mayorSpent;e.runMayor(*w);require(w->total_money==before,"annual cap breached");
  e.mayorMode=1;e.districtX=1;e.districtY=25;e.mayorBudget=50000;e.runMayor(*w);require(w->total_money==before,"mayor acted outside district");
  e.mayorMode=0;e.runMayor(*w);require(w->total_money==before,"revoked mayor authority still spent");
  require(w->stats.groupCount[GROUP_RESIDENCE_LL]==4,"existing houses demolished");
  e.mayorMode=2;e.mayorRoads=true;e.mayorServices=false;e.mayorParks=false;
  { // Five-year plan: propose, decide, steer construction, judge after five years, persist.
    auto pw=new_barren_city(50);pw->total_money=300000;pw->tech_level=MAX_TECH_LEVEL;auto& pe=pw->evolution;
    for(int i=0;i<12;++i)pw->buildConstruction(residenceLLConstructionGroup,MapPoint(2+(i%6)*4,2+(i/6)*3));
    for(int i=2;i<30;++i)pw->buildConstruction(trackConstructionGroup,MapPoint(i,8));
    pw->total_money=5000000;pe.residents=600;pe.approval=65;pe.mayorMode=0;
    const int built=pw->map.constructions.size(),cash=pw->total_money;
    pe.updatePlan(*pw);
    require(pe.planState==CityEvolution::PLAN_PROPOSED&&pe.planFocus==CityEvolution::FOCUS_SKILLS,"mayor did not propose a skills plan for a 12-home city");
    require(pe.tgtSchools==3&&pe.tgtUniversities==1&&pe.planEstimate>0&&pe.planBudgetYear>=5000,"plan goals or cost missing");
    require(int(pw->map.constructions.size())==built&&pw->total_money==cash,"a proposed plan must not build or spend");
    pe.mayorMode=2;pe.runMayor(*pw);require(pe.planSpent==0&&pw->stats.groupCount[GROUP_SCHOOL]==0&&pe.planState==CityEvolution::PLAN_PROPOSED,"an unapproved plan must not steer or charge the mayor");
    // Reject: nothing happens for a year, then a new proposal arrives.
    { auto rw=new_barren_city(50);auto& re=rw->evolution;for(int i=0;i<6;++i)rw->buildConstruction(residenceLLConstructionGroup,MapPoint(2+i*4,2));
      re.residents=300;re.updatePlan(*rw);require(re.planState==CityEvolution::PLAN_PROPOSED,"no proposal for rejection test");
      re.rejectPlan(*rw);require(re.planState==CityEvolution::PLAN_REJECTED,"reject not recorded");
      rw->total_time+=NUMOF_DAYS_IN_YEAR/2;re.updatePlan(*rw);require(re.planState==CityEvolution::PLAN_REJECTED,"mayor pestered the player before a year passed");
      rw->total_time+=NUMOF_DAYS_IN_YEAR;re.updatePlan(*rw);require(re.planState==CityEvolution::PLAN_PROPOSED&&re.planNumber==2,"mayor did not re-propose after a year"); }
    // Change focus to technology hub: housing for only 20 engineers must be called out.
    pe.changePlanFocus(*pw);
    require(pe.planFocus==CityEvolution::FOCUS_TECH&&pe.planNumber==1,"focus change should keep plan number");
    require(pe.tgtCampuses==1,"technology plan wants one new campus");
    pe.changePlanFocus(*pw);pe.changePlanFocus(*pw);require(pe.planFocus==CityEvolution::FOCUS_SKILLS,"focus does not cycle");
    // Approve: the mayor is authorised, budget set, and the plan drives building (schools first).
    pe.mayorMode=0;pe.approvePlan(*pw);
    require(pe.planState==CityEvolution::PLAN_ACTIVE&&pe.mayorMode==2&&pe.mayorBudget==pe.planBudgetYear,"approval did not authorise the mayor");
    const int budgetBefore=pe.planBudgetYear;pe.adjustPlanBudget(*pw,1000000);require(pe.planBudgetYear==budgetBefore+1000000&&pe.mayorBudget==pe.planBudgetYear,"budget change not applied");
    pe.mayorPace=1; // one project per call so this test can follow the first school
    const int cashBefore=pw->total_money;pe.runMayor(*pw);
    if(!(pw->stats.groupCount[GROUP_SCHOOL]==1&&pe.planSpent==cashBefore-pw->total_money&&pe.planSpent>0))require(pw->stats.groupCount[GROUP_SCHOOL]==1&&pe.planSpent==cashBefore-pw->total_money&&pe.planSpent>0,"plan did not build its first school and charge it");
    for(int i=0;i<6;++i)pe.runMayor(*pw);
    require(pw->stats.groupCount[GROUP_SCHOOL]==3,"plan did not reach its school goal");
    // A campus must wait for engineers and housing, not stand empty.
    ConstructionGroup* g=nullptr;std::string why;pe.tgtCampuses=1;pe.tgtUniversities=pw->stats.groupCount[GROUP_UNIVERSITY];pe.tgtHealth=pe.baseHealth;pe.tgtParks=pe.baseParks;
    pe.engineers=0;require(!pe.planNextBuild(*pw,g,why)&&why.find("waiting for engineers")!=std::string::npos,"campus was not held back for lack of engineers");
    pe.engineers=25;pe.residents=1500;require(pe.planNextBuild(*pw,g,why)&&g==&techCampusConstructionGroup,"campus not built once engineers exist");
    pe.residents=300;require(pe.planNextBuild(*pw,g,why)&&g==&techCampusConstructionGroup,"engineers must not be limited by housing");
    // Save and load keep the plan.
    auto planFile=std::filesystem::path(argc>1?std::string(argv[1])+".plan.scn.gz":"plan-check.scn.gz");pw->save(planFile);auto pc=World::load(planFile);std::filesystem::remove(planFile);
    require(pc->evolution.planState==CityEvolution::PLAN_ACTIVE&&pc->evolution.tgtSchools==3&&pc->evolution.planWhy==pe.planWhy&&pc->evolution.planSpent==pe.planSpent,"plan not persisted");
    // Five years later: judged against its own goals, authority restored.
    pw->total_time=pe.planStartDay+5*NUMOF_DAYS_IN_YEAR;pe.updatePlan(*pw);
    require(pe.planState==CityEvolution::PLAN_IDLE&&pe.plansDone==1&&pe.planResult.find("goals met")!=std::string::npos,"five-year review missing");
    require(pe.planResult.find("MISSED")!=std::string::npos&&pe.planResult.find("PARTLY")!=std::string::npos||pe.planResult.find("SUCCESS")!=std::string::npos,"review does not name which goals were missed or met");
    require(pe.mayorMode==0&&pe.mayorBudget==10000,"authority not restored after the plan");
    // Autonomous mayors adopt their own plans.
    { auto aw=new_barren_city(50);auto& ae=aw->evolution;for(int i=0;i<6;++i)aw->buildConstruction(residenceLLConstructionGroup,MapPoint(2+i*4,2));
      ae.residents=300;ae.planAuto=1;ae.mayorMode=0;ae.updatePlan(*aw);
      require(ae.planState==CityEvolution::PLAN_PROPOSED&&ae.mayorMode==0,"'decides alone' must not switch an OFF mayor on");
      ae.mayorMode=2;ae.updatePlan(*aw);require(ae.planState==CityEvolution::PLAN_ACTIVE,"autonomous mayor did not adopt a plan"); }
  }
  { // A plan obeys the same switches as the mayor, and finishing it never grants him authority back.
    auto qw=new_barren_city(50);qw->tech_level=MAX_TECH_LEVEL;auto& qe=qw->evolution;
    for(int i=0;i<12;++i)qw->buildConstruction(residenceLLConstructionGroup,MapPoint(2+(i%6)*4,2+(i/6)*3));
    for(int i=2;i<30;++i)qw->buildConstruction(trackConstructionGroup,MapPoint(i,8));
    qw->total_money=5000000;qe.residents=600;qe.approval=65;qe.mayorMode=2;qe.mayorBudget=2000000;
    qe.updatePlan(*qw);require(qe.planState==CityEvolution::PLAN_PROPOSED,"no plan for switch test");
    qe.approvePlan(*qw);qe.mayorServices=false;qe.mayorParks=false;
    ConstructionGroup* g=nullptr;std::string why;
    require(!qe.planNextBuild(*qw,g,why)&&why.find("switches")!=std::string::npos,"plan must respect the blocked-services switch");
    const int n0=qw->map.constructions.size();qe.runMayor(*qw);
    require(int(qw->map.constructions.size())==n0,"nothing may be built while services and parks are blocked");
    qe.mayorServices=true;require(qe.planNextBuild(*qw,g,why)&&g==&schoolConstructionGroup,"plan builds again once services are allowed");
    qe.mayorMode=0; // the player turns the mayor OFF during the plan
    qw->total_time=qe.planStartDay+5*NUMOF_DAYS_IN_YEAR;qe.updatePlan(*qw);
    require(qe.planState==CityEvolution::PLAN_IDLE&&qe.mayorMode==0,"finishing a plan must not switch a revoked mayor back on");
  }
  { // Mayor mandate: on/off, area, pace, road building, city name.
    auto mk=[&](int pace,bool roads){ auto mw=new_barren_city(60);mw->tech_level=MAX_TECH_LEVEL;
      for(int i=0;i<12;++i)mw->buildConstruction(residenceLLConstructionGroup,MapPoint(2+(i%6)*4,2+(i/6)*3));
      for(int i=2;i<30;++i)mw->buildConstruction(trackConstructionGroup,MapPoint(i,8));
      mw->total_money=5000000;auto& me=mw->evolution;me.residents=600;me.mayorMode=2;me.mayorBudget=2000000;me.mayorPace=pace;
      me.mayorRoads=roads;return mw; };
    { auto a=mk(1,false);a->evolution.runMayor(*a);require(a->evolution.mayorBuiltMonth==1,"careful pace builds one project a month");
      auto b=mk(3,false);b->evolution.runMayor(*b);require(b->evolution.mayorBuiltMonth==3&&b->stats.groupCount[GROUP_SCHOOL]==1&&b->stats.groupCount[GROUP_WATERWELL]==1,"fast pace builds several projects in one month"); }
    { auto off=mk(3,true);off->evolution.mayorMode=0;const int n0=off->map.constructions.size(),c0=off->total_money;off->evolution.runMayor(*off);
      require(int(off->map.constructions.size())==n0&&off->total_money==c0&&off->evolution.mayorReport.find("I suggest")==0,"a mayor that is OFF must only advise"); }
    { auto z=mk(3,false);auto& ze=z->evolution;ze.setMayorScopeZone(*z,55,55,40,40);int a,b,c,d;ze.mayorBounds(*z,a,b,c,d);
      require(a==40&&b==40&&c==56&&d==56&&ze.mayorScope==2,"drawn zone bounds (corners may be dragged in any order)");
      const int n0=z->map.constructions.size();ze.runMayor(*z);require(int(z->map.constructions.size())==n0,"a mayor must not build outside his zone");
      ze.setMayorScopeZone(*z,0,0,30,12);ze.runMayor(*z);require(int(z->map.constructions.size())>n0,"a mayor builds inside his zone");
      require(ze.addDistrict(*z,CityEvolution::DIST_INDUSTRIAL,2,2,20,10,"Old Town"),"district for mayor scope");
      ze.setMayorScopeDistrict(0);ze.mayorBounds(*z,a,b,c,d);require(a==2&&b==2&&c==21&&d==11&&ze.mayorScopeText().find("Old Town")!=std::string::npos,"district scope");
      ze.removeDistrict(0);require(ze.mayorScope==0,"deleting the mayor's district returns him to the whole city"); }
    { // Roads: he connects a far building to the network, a little more each month by pace.
      for(int pace:{1,3}) { auto r=mk(pace,true);auto& re=r->evolution;re.mayorServices=false;re.mayorParks=false;
        r->buildConstruction(industryLightConstructionGroup,MapPoint(10,24));
        const int n0=r->map.constructions.size();re.runMayor(*r);
        const int added=int(r->map.constructions.size())-n0;
        if(pace==1) require(re.mayorRoadsMonth==4&&added==4,"careful pace lays 4 road tiles a month");
        else { require(re.mayorRoadsMonth>=10&&added==re.mayorRoadsMonth,"fast pace lays a whole road in one month");
               for(int m=0;m<3;++m) re.runMayor(*r);
               require(re.mayorRoadsMonth==0,"once connected he stops building roads"); } } }
    { auto cw=new_barren_city(30);auto& ce=cw->evolution;ce.renameCity("  თბილისი | Test  ");
      require(ce.cityName=="თბილისი  Test","city name cleaned");
      auto cf=std::filesystem::path(argc>1?std::string(argv[1])+".city.scn.gz":"city-check.scn.gz");cw->save(cf);auto cl=World::load(cf);std::filesystem::remove(cf);
      require(cl->evolution.cityName=="თბილისი  Test","city name not persisted"); }
    std::cout<<"PASS mayor mandate: on/off, area, pace, road building, city name\n";
  }
  auto file=std::filesystem::path(argc>1?argv[1]:"mayor-check.scn.gz");w->save(file);auto copy=World::load(file);
  require(copy->evolution.mayorMode==2&&copy->evolution.mayorRoads&&!copy->evolution.mayorServices&&!copy->evolution.mayorParks,"permissions not persisted");
  require(copy->evolution.mayorSpent==e.mayorSpent,"spending not persisted");
  copy->evolution.mayorMode=0;for(int day=0;day<300;++day)copy->do_time_step();
  std::filesystem::remove(file);
  std::cout<<"PASS: real service construction, real expense, reserve, annual cap, district scope, revoked authority, preserved buildings and save/load\n";
 }catch(const Message::Exception&ex){std::cerr<<ex.getMessage()->str()<<'\n';return 1;}catch(const std::exception&ex){std::cerr<<ex.what()<<'\n';return 1;}
}
