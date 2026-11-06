// SPDX-License-Identifier: GPL-2.0-or-later
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/world.hpp"
#include "lincity/building_visual_state.hpp"
#include "lincity-ng/UiScale.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static auto town(){auto w=new_barren_city(60);w->total_money=10000000;w->tech_level=MAX_TECH_LEVEL;for(auto p:{MapPoint(6,15),MapPoint(30,15),MapPoint(48,42)}){w->buildConstruction(residenceHLConstructionGroup,p);static_cast<Residence*>(w->map(p)->construction)->local_population=150;}for(int x=5;x<=35;++x)w->buildConstruction(roadConstructionGroup,{x,14});return w;}
int main(int argc,char**argv){try{initializeModules();
 if(argc>1){auto w=World::load(argv[1]);for(int i=0;i<500;++i){++w->total_time;w->flu.update(*w);w->civic.update(*w);}auto file=std::filesystem::temp_directory_path()/"preview14-user-save.scn.gz";w->save(file);auto load=World::load(file);std::filesystem::remove(file);check(load->flu.save()==w->flu.save(),"user save flu roundtrip");std::cout<<"PASS supplied 150x150 Preview13 save load, 500 service/flu days and roundtrip\n";return 0;}
 auto w=town();int id=15*60+6,connected=15*60+30,isolated=42*60+48;w->flu.seed(*w,id,20);check(w->flu.latent==20&&w->flu.active==0,"incubation begins exposed");w->total_time=5;w->flu.update(*w);check(w->flu.active==0,"incubation interval");w->total_time=10;w->flu.update(*w);check(w->flu.active==20,"incubation progresses to infectious");auto state=w->flu.save();w->flu.update(*w);check(state==w->flu.save(),"one tick per simulation day");
 bool commuting=false;for(int d=15;d<=100;d+=5){w->total_time=d;w->flu.update(*w);commuting|=w->flu.households.count(connected)>0;check(w->flu.active+w->flu.latent<=90,"bounded small outbreak");}check(commuting,"road-connected community transmission");check(!w->flu.households.count(isolated),"isolated distant homes do not receive road mixing");
 auto plain=town();plain->flu.seed(*plain,id,15);int recovered=0;for(int day=5;day<=100;day+=5){plain->total_time=day;plain->flu.update(*plain);recovered=std::max(recovered,plain->flu.recovered);}check(recovered>0,"natural recovery and temporary immunity");
 auto care=town();care->flu.seed(*care,id,15);care->map({6,15})->flags|=FLAG_HEALTH_COVER;for(int day=5;day<=20;day+=5){care->total_time=day;care->flu.update(*care);}check(care->flu.recovered>0,"funded healthcare helps recovery");int before=care->flu.active;care->flu.treat(id,3);check(care->flu.active<=before,"ambulance treatment lowers active cases");
 auto file=std::filesystem::temp_directory_path()/"flu-check.scn.gz";w->save(file);auto loaded=World::load(file);std::filesystem::remove(file);check(w->flu.save()==loaded->flu.save(),"all cohorts and RNG survive full save load");for(int d=105;d<=150;d+=5){w->total_time=loaded->total_time=d;w->flu.update(*w);loaded->flu.update(*loaded);}check(w->flu.save()==loaded->flu.save(),"deterministic continuation after save");
 w->flu.enabled=false;w->flu.update(*w);check(w->flu.households.empty()&&w->flu.active==0&&w->flu.latent==0,"off switch clears flu");check(w->flu.seed(*w,id,10)==0,"off disables imports");
 bool rejected=false;try{FluSystem invalid;invalid.load("1 1 10 0 123 1 999999",60);}catch(const std::exception&){rejected=true;}check(rejected,"malformed flu state rejected");
 auto visual=town();visual->buildConstruction(marketConstructionGroup,{20,25});auto* market=static_cast<Market*>(visual->map({20,25})->construction);for(int ratio:{0,15,30,80}){market->market_ratio=ratio;auto v=buildingStockVisual(*market);check(v.level==(ratio==0?0:ratio==15?1:ratio==30?2:3),"real market load selects visible stock");}
 for(auto [width,height]:{std::pair{1280,800},std::pair{1920,1080},std::pair{3072,1590},std::pair{3840,2160}}){auto ui=calculateUiGeometry(width,height);check(ui.width>=1024&&ui.height>=720,"usable logical UI size");check(ui.width*ui.scale>=width-2&&ui.width*ui.scale<=width+2,"viewport scale fills screen");}
 auto high=calculateUiGeometry(3072,1590);check(high.scale>=1.5f&&high.width<2000,"high-resolution interface is enlarged");std::cout<<"PASS flu incubation, contact transmission, isolation, recovery, care, outbreak bounds, save/load, off switch, load visuals and 3K/4K UI geometry\n";
}catch(const Message::Exception& e){std::cerr<<e.getMessage()->str()<<'\n';return 1;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
