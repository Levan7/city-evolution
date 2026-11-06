// SPDX-License-Identifier: GPL-2.0-or-later
#include "world_generation_checks.hpp"
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/modules/city_business.hpp"
#include "lincity/world.hpp"
#include <iostream>
#include <stdexcept>
#include <filesystem>
static int checks=0;
static void require(bool ok,const char* text) { ++checks;if(!ok)throw std::runtime_error(text); }
int main(int argc,char**argv) {
 try {
  initializeModules();
  auto w=new_barren_city(50);
  CityEvolution sky;
  for(int i=0;i<4*60*60*4;++i) { // four active hours at 250ms/tick
   sky.advance(250);
   require(sky.darkness()>=0&&sky.darkness()<=1000,"darkness outside range");
   require(sky.solarPercent()>=0&&sky.solarPercent()<=100,"solar outside range");
   if(sky.skyPhase==0) require(sky.skyDuration>=480000&&sky.skyDuration<=960000,"day too short");
   if(sky.skyPhase==2) require(sky.skyDuration>=180000&&sky.skyDuration<=360000,"night too short");
  }
  sky.skyMode=1;sky.weatherMode=1;require(sky.solarPercent()==100,"clear daylight output");
  sky.weatherMode=3;require(sky.solarPercent()==45,"rain output");
  sky.skyMode=2;require(sky.solarPercent()==0,"night output must stop");
  w->total_money=100000000;
  w->tech_level=MAX_TECH_LEVEL;
  Message::ptr reason;
  require(!techCampusConstructionGroup.can_build(*w,reason),"campus without university");
  require(!cafeConstructionGroup.can_build(*w,reason),"cafe without demand");
  for(int i=0;i<8;++i) w->buildConstruction(residenceLLConstructionGroup,MapPoint(2+i*4,2));
  for(int i=0;i<4;++i) w->buildConstruction(schoolConstructionGroup,MapPoint(2+i*5,20));
  w->buildConstruction(universityConstructionGroup,MapPoint(2,8));
  require(cafeConstructionGroup.can_build(*w,reason),"cafe with demand");
  require(techCampusConstructionGroup.can_build(*w,reason),"campus with university");
  w->buildConstruction(warehouseConstructionGroup,MapPoint(10,10));
  w->buildConstruction(techCampusConstructionGroup,MapPoint(15,10));
  w->buildConstruction(cafeConstructionGroup,MapPoint(20,10));
  w->buildConstruction(foodHallConstructionGroup,MapPoint(25,10));
  auto* cafe=w->map(MapPoint(20,10))->reportingConstruction;
  auto* tech=w->map(MapPoint(15,10))->reportingConstruction;
  for(auto* c : {cafe,tech}) for(Commodity stuff : {STUFF_LABOR,STUFF_WATER,STUFF_LOVOLT,STUFF_FOOD,STUFF_STEEL})
    if(c->constructionGroup->commodityRuleCount[stuff].maxload)c->commodityCount[stuff]=1000;
  cafe->commodityCount[STUFF_GOODS]=100;
  int money=w->total_money;
  cafe->update(); require(w->total_money==money+12,"cafe income");
  require(cafe->commodityCount[STUFF_FOOD]==990,"cafe consumes food");
  cafe->commodityCount[STUFF_LOVOLT]=0;money=w->total_money;
  cafe->update();require(w->total_money==money,"unpowered cafe must close");
  tech->update();require(tech->commodityCount[STUFF_GOODS]==60,"campus produces goods");
  require(tech->commodityCount[STUFF_STEEL]==996,"campus consumes steel");
  w->buildConstruction(railConstructionGroup,MapPoint(9,10));
  w->buildConstruction(roadConstructionGroup,MapPoint(12,10));
  auto* rail=w->map(MapPoint(9,10))->reportingConstruction;
  auto* warehouse=w->map(MapPoint(10,10))->reportingConstruction;
  auto* road=w->map(MapPoint(12,10))->reportingConstruction;
  rail->commodityCount[STUFF_GOODS]=1000;
  for(int i=0;i<20;++i) { rail->trade();warehouse->trade();road->trade(); }
  require(road->commodityCount[STUFF_GOODS]>0,"rail cargo must reach road through warehouse");
  require(rail->commodityCount[STUFF_GOODS]+warehouse->commodityCount[STUFF_GOODS]+road->commodityCount[STUFF_GOODS]==1000,"warehouse conserves cargo");
  w->buildConstruction(solarPowerConstructionGroup,MapPoint(32,30));
  auto* solar=w->map(MapPoint(32,30))->reportingConstruction;
  solar->commodityCount[STUFF_LABOR]=10000;
  w->evolution.skyMode=1;w->evolution.weatherMode=1;
  solar->update(); require(solar->commodityCount[STUFF_HIVOLT]>0,"solar produces in daylight");
  w->evolution.skyMode=2;solar->commodityCount[STUFF_HIVOLT]=0;
  solar->update(); require(solar->commodityCount[STUFF_HIVOLT]==0,"solar stops at night");
  auto file=std::filesystem::path(argc>1?argv[1]:"evolution-check.scn.gz");
  w->evolution.skyMode=2;w->evolution.weatherMode=3;w->evolution.skyElapsed=12234;
  w->evolution.homes=8; w->evolution.poweredHomes=3; w->evolution.amenities=2;
  w->evolution.headline("Election persistence & resource checks");
  w->save(file);auto copy=World::load(file);
  require(copy->stats.groupCount[GROUP_WAREHOUSE]==1,"warehouse saved");
  require(copy->stats.groupCount[GROUP_TECH_CAMPUS]==1,"campus saved");
  require(copy->evolution.skyMode==2&&copy->evolution.weatherMode==3,"atmosphere saved");
  require(copy->evolution.skyElapsed==12234,"clock saved");
  require(copy->evolution.homes==8 && copy->evolution.poweredHomes==3 && copy->evolution.amenities==2,"dashboard saved");
  require(copy->evolution.news==w->evolution.news,"news saved");
  require(copy->map(MapPoint(15,10))->reportingConstruction->commodityCount[STUFF_GOODS]==60,"cargo saved");
  copy->total_time=4800;copy->evolution.updateCity(*copy);
  require(copy->evolution.electionCount==1,"election held");
  require(copy->evolution.nextElection==9600,"election rescheduled");
  require(copy->map.constructions.size()==w->map.constructions.size(),"election retains city control");
  for(int day=0;day<300;++day)copy->do_time_step();
  { // Skilled workforce: gradual training, housing limit, emigration, staffing speed, advisor causes.
    CityEvolution& e=w->evolution; e=CityEvolution{};
    for(auto* c:w->map.constructions) {
      if(c->constructionGroup->group==GROUP_SCHOOL) static_cast<School*>(c)->busy=100;
      if(c->constructionGroup->group==GROUP_UNIVERSITY) static_cast<University*>(c)->busy=100;
    }
    w->stats.population.population_m.stat=3000; e.approval=70;
    e.updateWorkforce(*w);
    require(e.campuses==1&&e.engNeed==40&&e.techNeed==20,"campus skill need");
    require(e.engineers==5&&e.technicians==24,"training is gradual, not instant");
    require(e.engCoverage==125&&e.campusSpeed()==125,"campus speed follows staffing");
    auto adv=e.advise(*w); bool found=false;
    for(const auto& a:adv) if(a.title=="Staff are still being trained"&&a.text.find("about 7 months")!=std::string::npos) found=true;
    require(found,"advisor explains training time with numbers");
    for(int m=2;m<=7;++m) e.updateWorkforce(*w);
    require(e.engineers==35&&e.campusSpeed()==875,"seven months of training");
    e.updateWorkforce(*w);
    require(e.engineers==40&&e.campusSpeed()==1000,"eight months fully staffs one campus");
    e.approval=20; const int before=e.engineers; e.updateWorkforce(*w);
    require(e.skilledLeftMonth>0&&e.engineers<before+5,"unhappy city loses skilled people");
    adv=e.advise(*w);
    e.approval=70; e.engineers=10; e.technicians=30; w->stats.population.population_m.stat=300; e.updateWorkforce(*w);
    require(e.technicians<=30,"housing limits technicians");
    require(e.engineers>10,"engineers must not be limited by housing");
    auto* cb=static_cast<CityBusiness*>(tech); cb->skillCredit=0; cb->workingDays=0;
    e.engCoverage=500; e.techCoverage=1000;
    for(int d=0;d<100;++d) {
      cb->commodityCount[STUFF_LABOR]=100;cb->commodityCount[STUFF_WATER]=100;cb->commodityCount[STUFF_LOVOLT]=1000;
      cb->commodityCount[STUFF_STEEL]=100;cb->commodityCount[STUFF_GOODS]=0;cb->commodityCount[STUFF_WASTE]=0;
      cb->update();
    }
    require(cb->workingDays>=49&&cb->workingDays<=51,"campus at 50% staffing works about half the days");
    std::cout<<"PASS skilled workforce model\n";
  }
  { // Factories run without skilled staff, but slowly (25%), reaching 100% when fully staffed.
    auto fw=new_barren_city(40);fw->total_money=5000000;fw->tech_level=MAX_TECH_LEVEL;
    fw->buildConstruction(industryLightConstructionGroup,MapPoint(5,5));
    fw->buildConstruction(industryHeavyConstructionGroup,MapPoint(15,5));
    auto& fe=fw->evolution;fe.residents=0;fe.updateWorkforce(*fw);
    require(fe.lightFactories==1&&fe.heavyFactories==1&&fe.techNeed==16&&fe.engNeed==4,"factory staff need");
    require(fe.industrySpeed()==250,"unstaffed factories must run at 25%");
    fe.techCoverage=1000;fe.engCoverage=1000;require(fe.industrySpeed()==1000,"fully staffed factory speed");
    fe.engCoverage=500;require(fe.industrySpeed()==625,"half staffed factory speed");
    int cr=0,days=0;for(int d=0;d<100;++d)days+=CityEvolution::skilledDay(cr,250);require(days==25,"duty cycle gives exactly 25 of 100 days");
    auto* li=static_cast<IndustryLight*>(fw->map(MapPoint(5,5))->reportingConstruction);
    auto run=[&](int speedCov){fe.techCoverage=speedCov;fe.engCoverage=speedCov;li->skillCredit=0;int worked=0;
      for(int d=0;d<100;++d){li->commodityCount[STUFF_LABOR]=2000;li->commodityCount[STUFF_ORE]=2000;li->commodityCount[STUFF_GOODS]=0;li->commodityCount[STUFF_WASTE]=0;li->update();worked+=li->commodityCount[STUFF_GOODS]>0;}
      return worked;};
    const int poor=run(0),good=run(1000);
    require(poor>=24&&poor<=26,"unstaffed light industry should work about 1 day in 4");
    require(good==100,"fully staffed light industry should work every ready day");
    std::cout<<"PASS factories work poorly without skilled staff\n";
  }
  { // Districts: player-drawn, typed, named zones with real effects.
    auto dw=new_barren_city(60);dw->total_money=50000000;dw->tech_level=MAX_TECH_LEVEL;auto& de=dw->evolution;
    dw->buildConstruction(industryLightConstructionGroup,MapPoint(5,5));
    dw->buildConstruction(industryLightConstructionGroup,MapPoint(10,5));
    dw->buildConstruction(industryLightConstructionGroup,MapPoint(45,45));
    require(!de.addDistrict(*dw,99,0,0,10,10,"x"),"bad district type accepted");
    require(!de.addDistrict(*dw,CityEvolution::DIST_INDUSTRIAL,5,5,5,5,"tiny"),"1x1 zone accepted");
    require(de.addDistrict(*dw,CityEvolution::DIST_INDUSTRIAL,20,15,2,2,"  North | Works  "),"drag from bottom-right to top-left must work");
    require(de.districts[0].x0==2&&de.districts[0].y0==2&&de.districts[0].x1==20&&de.districts[0].y1==15,"zone corners not normalised");
    require(de.districts[0].name=="North  Works","name not cleaned");
    require(de.districtStats[0].factories==2&&de.districtStats[0].buildings==2,"district must count the buildings already inside it");
    require(de.districtBonusAt(5,5,0)==200&&de.districtBonusAt(45,45,0)==0,"cluster bonus only inside the district");
    require(de.districtAt(5,5)==0&&de.districtAt(30,30)==-1,"district lookup");
    de.retypeDistrict(*dw,0,CityEvolution::DIST_TECH);require(de.districtBonusAt(5,5,0)==0&&de.districts[0].type==CityEvolution::DIST_TECH,"retype must change the effect");
    require(de.districtStats[0].fit==0&&de.districtStats[0].bestType==CityEvolution::DIST_INDUSTRIAL,"a tech district full of factories should suggest industrial");
    de.retypeDistrict(*dw,0,CityEvolution::DIST_INDUSTRIAL);
    require(de.addDistrict(*dw,CityEvolution::DIST_TRADE,8,3,30,12,"Overlap"),"second district");
    require(de.districtAt(10,5)==1&&de.districtAt(5,5)==0,"newest district must win where zones overlap");
    de.removeDistrict(1);require(de.districts.size()==1&&int(de.districtStats.size())==1,"remove district");
    // Farming: a full farm cannot send its harvest out.
    dw->buildConstruction(organic_farmConstructionGroup,MapPoint(30,30));
    require(de.addDistrict(*dw,CityEvolution::DIST_FARMING,26,26,40,40,"Green Valley"),"farming district");
    auto* farm=dw->map(MapPoint(30,30))->reportingConstruction;
    farm->commodityCount[STUFF_FOOD]=farm->constructionGroup->commodityRuleCount[STUFF_FOOD].maxload;
    de.updateDistricts(*dw);require(de.districtStats[1].farms==1&&de.districtStats[1].backedUpFarms==1,"full farm not detected");
    de.residents=100;bool harvest=false;for(const auto& a:de.advise(*dw)) if(a.title=="Harvest cannot leave"&&a.text.find("100%")!=std::string::npos&&a.text.find("Green Valley")!=std::string::npos)harvest=true;
    require(harvest,"advisor must name the district and the share of harvest at risk");
    // University district: schools inside train 25% more (6 -> 7 technicians a month).
    dw->buildConstruction(schoolConstructionGroup,MapPoint(50,8));
    for(auto* c:dw->map.constructions) if(c->constructionGroup->group==GROUP_SCHOOL) static_cast<School*>(c)->busy=100;
    de.updateWorkforce(*dw);require(de.techTrainedMonth==6,"baseline school training");
    require(de.addDistrict(*dw,CityEvolution::DIST_UNIVERSITY,47,5,56,14,"Campus Hill"),"university district");
    de.updateWorkforce(*dw);require(de.techTrainedMonth==7,"university district must raise training by 25%");
    // Save and reload keep zones and names, including Georgian script.
    de.renameDistrict(2,"თბილისი");
    auto dfile=std::filesystem::path(argc>1?std::string(argv[1])+".dist.scn.gz":"dist.scn.gz");dw->save(dfile);auto dl=World::load(dfile);std::filesystem::remove(dfile);
    require(dl->evolution.districts.size()==3&&dl->evolution.districts[2].name=="თბილისი"&&dl->evolution.districts[0].name=="North  Works","district names not persisted");
    require(dl->evolution.districts[1].type==CityEvolution::DIST_FARMING&&dl->evolution.districts[1].x0==26&&dl->evolution.districts[1].y1==40,"district zone not persisted");
    // At most 12 districts.
    auto mw=new_barren_city(60);int added=0;for(int i=0;i<20;++i)added+=mw->evolution.addDistrict(*mw,CityEvolution::DIST_TRADE,1,1,10,10,"");
    require(added==12&&mw->evolution.districts.size()==12,"district limit");
    std::cout<<"PASS districts: zones, names, clusters, farm warning, training boost, persistence\n";
  }
  { // Retirement is fair to small groups, and forecasts use the same rules as the game.
    const int small=CityEvolution::projectSkilled(10,0,120,1000),big=CityEvolution::projectSkilled(100,0,120,1000);
    require(small<10&&small>=6,"a group of 10 must also lose people to retirement over 10 years (about 3 to 4)");
    require(big>=66&&big<=73,"100 people lose about 30% to retirement over 10 years");
    require(CityEvolution::monthsToReach(5,40,5,1000)==7,"7 months to train 35 more engineers at 5 a month");
    require(CityEvolution::monthsToReach(0,500,1,1000)==-1,"1 graduate a month can never reach 500 against retirement");
    require(CityEvolution::lossPermille(70)==3&&CityEvolution::lossPermille(20)==33,"unhappy cities lose skilled people ten times faster");
    // the real monthly update loses small groups too
    CityEvolution e;e.approval=70;e.technicians=0;e.engineers=10;auto cw=new_barren_city(20);cw->stats.population.population_m.stat=3000;
    int before=e.engineers;for(int m=0;m<120;++m)e.updateWorkforce(*cw);
    require(e.engineers<before&&e.engineers>=6,"monthly update must retire people from a small group");
    std::cout<<"PASS retirement model\n";
  }
  std::filesystem::remove(file);
  require(verifyWorldGeneration(file.parent_path()/"world-checks",false)==0,"world generation and legacy scenarios");
  if(argc>2) for(const auto& entry : std::filesystem::directory_iterator(argv[2])) {
    if(entry.path().extension()!=".gz" || entry.path().stem().extension()!=".scn") continue;
    auto legacy=World::load(entry.path());
    int count=legacy->map.constructions.size();
    legacy->save(file);auto migrated=World::load(file);
    require(int(migrated->map.constructions.size())==count,"legacy city migration");
    for(int i=0;i<10;++i)migrated->do_time_step();
    std::cout<<"PASS legacy scenario "<<entry.path().filename()<<'\n';
    std::filesystem::remove(file);
  }
  if(argc>3) {
    w->total_money=2000000;
    w->given_scene="City Evolution showcase";
    w->evolution=CityEvolution{};
    w->evolution.nextElection=300;
    w->evolution.headline("Showcase sandbox: first election after 300 days; normal cities use four-year terms.");
    auto placeRoad=[&](MapPoint p) { if(w->map(p)->is_bare()) w->buildConstruction(roadConstructionGroup,p); };
    for(int x=2;x<=39;++x)placeRoad(MapPoint(x,14));
    for(int x=2;x<=34;++x)placeRoad(MapPoint(x,5));
    for(int y=6;y<14;++y)placeRoad(MapPoint(35,y));
    for(int y=11;y<14;++y)placeRoad(MapPoint(20,y));
    for(int y=12;y<14;++y) { placeRoad(MapPoint(12,y));placeRoad(MapPoint(26,y)); }
    placeRoad(MapPoint(15,13));
    w->buildConstruction(marketConstructionGroup,MapPoint(2,12));
    w->buildConstruction(waterwellConstructionGroup,MapPoint(6,12));
    w->buildConstruction(organic_farmConstructionGroup,MapPoint(32,18));
    for(int y=15;y<18;++y)placeRoad(MapPoint(33,y));
    for(auto* c:w->map.constructions) c->bootstrap_commodities(60);
    tech->commodityCount[STUFF_GOODS]=0;
    w->map.recentPoint=MapPoint(20,11);
    w->save(argv[3]);auto demo=World::load(argv[3]);
    require(demo->stats.groupCount[GROUP_TECH_CAMPUS]==1,"showcase reload");
    std::cout<<"PASS playable showcase saved and reloaded\n";
  }
  std::cout<<"PASS: "<<checks<<" atmosphere, resource, demand, civic and save/load integration checks\n";
 }catch(const Message::Exception&ex){std::cerr<<"FAIL: "<<ex.getMessage()->str()<<'\n';return 1;}catch(const std::exception&ex){std::cerr<<"FAIL: "<<ex.what()<<'\n';return 1;}
}
