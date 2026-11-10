/* ---------------------------------------------------------------------- *
 * src/lincity-ng/Game.cpp
 * This file is part of Lincity-NG.
 *
 * Copyright (C) 2005      Matthias Braun <matze@braunis.de>
 * Copyright (C) 2025      David Bears <dbear4q@gmail.com>
 * Copyright (C) 2026      Marc Young <myoung008@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
** ---------------------------------------------------------------------- */

#include <numeric>
#include "Game.hpp"
#include "lincity/modules/civic_buildings.hpp"

#include <SDL3/SDL.h>                       // for SDL_KeyCode, Uint32, SDL_...
#include <assert.h>                         // for assert
#include <fmt/format.h>                     // for format, native_formatter:...
#include <stddef.h>                         // for NULL
#include <algorithm>                        // for min
#include <filesystem>                       // for path, operator/, director...
#include <functional>                       // for function, bind, _1
#include <initializer_list>                 // for initializer_list
#include <iostream>                         // for basic_ostream, operator<<
#include <list>                             // for list
#include <optional>                         // for optional
#include <stdexcept>                        // for runtime_error
#include <typeinfo>                         // for type_info
#include <utility>                          // for move

#include "ButtonPanel.hpp"                  // for ButtonPanel
#include "Config.hpp"                       // for getConfig, Config
#include "Dialog.hpp"                       // for Dialog, closeAllDialogs
#include "EconomyGraph.hpp"                 // for EconomyGraph
#include "GameView.hpp"                     // for GameView
#include "HelpWindow.hpp"                   // for HelpWindow
#include "MainLincity.hpp"                  // for saveCityNG, loadCityNG
#include "MiniMap.hpp"                      // for MiniMap
#include "Mps.hpp"                          // for MpsMap, MpsFinance
#include "PBar.hpp"                         // for LCPBar
#include "Sound.hpp"                        // for getSound, Sound
#include "TimerInterface.hpp"               // for get_real_time_with
#include "Util.hpp"                         // for getButton
#include "gui/Button.hpp"                   // for Button
#include "gui/Component.hpp"                // for Component
#include "gui/ComponentLoader.hpp"          // for loadGUIFile
#include "gui/Desktop.hpp"                  // for Desktop
#include "gui/DialogBuilder.hpp"            // for DialogBuilder
#include "gui/Event.hpp"                    // for Event
#include "gui/Painter.hpp"                  // for Painter
#include "gui/Signal.hpp"                   // for Signal
#include "gui/SwitchComponent.hpp"
#include "gui/Window.hpp"
#include "gui/Paragraph.hpp"
#include "gui/WindowManager.hpp"            // for WindowManager
#include "lincity/MapPoint.hpp"             // for MapPoint, operator<<
#include "lincity/groups.hpp"               // for GROUP_MARKET, GROUP_MONUMENT
#include "lincity/lin-city.hpp"             // for MAX_TECH_LEVEL, ANIMATE_D...
#include "lincity/lintypes.hpp"             // for ConstructionGroup, Constr...
#include "lincity/messages.hpp"             // for dynamic_message_cast, Mes...
#include "lincity/modules/all_modules.hpp"  // for RocketPad, ParklandConstr...
#include "lincity/world.hpp"                // for World, Map, MapTile
#include "main.hpp"                         // for painter, videoSizeChanged
#include "util/gettextutil.hpp"             // for _
#include "util/ptrutil.hpp"                 // for dynamic_unique_cast

using namespace std::placeholders;

Game::Game(SDL_Window *window) : window(window) {
  loadGui();
}

Game::~Game() = default;

void Game::showHelpWindow( std::string topic ){
    helpWindow->showTopic( topic );
}

void Game::backToMainMenu(){
    closeAllDialogs();
    saveCityNG(*world,
      getConfig()->userDataDir.get() / "9_currentGameNG.scn.gz");
    running = false;
}

void Game::quickLoad(){
  closeAllDialogs();

  //load file
  getGameView().printStatusMessage("quick load...");
  std::string filename("quicksave.scn.gz");
  if(std::unique_ptr<World> world =
    loadCityNG(getConfig()->userDataDir.get() / filename)
  ) {
    setWorld(std::move(world));
    getGameView().printStatusMessage("quick load successful.");
  } else {
    getGameView().printStatusMessage("quick load failed!");
  }
}

void Game::quickSave(){
  //save file
  getGameView().printStatusMessage("quick save...");
  saveCityNG(*world, getConfig()->userDataDir.get() / "quicksave.scn.gz");
}

void Game::testAllHelpFiles(){
  getGameView().printStatusMessage("Testing Help Files...");

  std::filesystem::path dir = getConfig()->appDataDir.get() / "help" / "en";
  for(auto& dirEntry : std::filesystem::directory_iterator(dir)) {
    if(!dirEntry.is_regular_file()) continue;
    std::cerr << "--- Examining " << dirEntry.path().stem() << "\n";
    helpWindow->showTopic(dirEntry.path().stem().string());
  }
}

void
Game::loadGui() {
  gui = dynamic_unique_cast<Desktop>(loadGUIFile("gui/app.xml"));

  gameview = dynamic_cast<GameView *>(gui->findComponent("GameView"));
  minimap = dynamic_cast<MiniMap *>(gui->findComponent("MiniMap"));
  buttonpanel =
    dynamic_cast<ButtonPanel *>(gui->findComponent("ButtonPanel"));
  economygraph =
    dynamic_cast<EconomyGraph *>(gui->findComponent("EconomyGraph"));
  windowmanager = dynamic_cast<WindowManager *>(
    gui->findComponent("windowManager"));
  mpsmap = dynamic_cast<MpsMap *>(gui->findComponent("MapMPS"));
  mpsfinance = dynamic_cast<MpsFinance *>(gui->findComponent("GlobalMPS"));
  pbar1 = dynamic_cast<LCPBar *>(gui->findComponent("PBar"));
  pbar2 = dynamic_cast<LCPBar *>(gui->findComponent("PBar2nd"));
  gameview->setGame(this);
  minimap->setGame(this);
  buttonpanel->setGame(this);
  economygraph->setGame(this);
  mpsfinance->setGame(this);
  pbar1->setGame(this);
  pbar2->setGame(this);

  buttonpanel->selected.connect(std::bind(&Game::setUserOperation, this, _1));

  Button* gameMenu = getButton(*gui, "GameMenuButton");
  gameMenu->clicked.connect(std::bind(&Game::backToMainMenu, this));
  getButton(*gui, "CityAdvisorsButton")->clicked.connect(
    [this](Button*) { showCityHall(); });

  getButton(*gui, "MayorButton")->clicked.connect(
    [this](Button*) { showCityHall(true); });

  getButton(*gui,"TransportButton")->clicked.connect([this](Button*) {showTransport();});
  Button* helpButton = getButton(*gui, "HelpButton");
  helpButton->clicked.connect([this](Button *) {
    helpWindow->showTopic("help");
  });

  Button* statButton = getButton(*gui, "StatButton");
  statButton->clicked.connect([this](Button *) {
    new Dialog(*this, GAME_STATS);
  });

  helpWindow.reset(new HelpWindow(windowmanager));

  int width = 0, height = 0;
  SDL_GetWindowSize(window, &width, &height);
  {auto ui=resizeUserInterface();gui->resize(ui.width,ui.height);}
}

World&
Game::getWorld() const {
  assert(world);
  return *world;
}



void Game::showTransport() {
  if(gui->findComponent("TransportWindow")) return;
  auto panel=dynamic_unique_cast<Window>(loadGUIFile("gui/transport.xml"));auto* w=panel.get();
  auto refresh=[this,w]() {
    world->transit.update(*world,false);auto& t=world->transit;
    const char* labels[]={"Bus","Rail","Underground","Elevated"};
    for(int i=0;i<4;++i)getParagraph(*w,("TransitMode"+std::to_string(i)).c_str())->setText(std::string(labels[i])+": "+std::to_string(t.passengers[i])+" trips/month | "+std::to_string(t.connected[i])+" connected | "+std::to_string(t.disconnected[i])+" disconnected | "+std::to_string(t.unstaffed[i])+" unstaffed");
    getParagraph(*w,"TransitSummary")->setText(std::to_string(t.servedHomes.size())+" homes with job access | Shopping trips "+std::to_string(std::accumulate(t.shoppingTrips.begin(),t.shoppingTrips.end(),0))+"/month | Fares about $"+std::to_string(t.faresMonth)+"/month");
    const char* views[]={"Normal city view","Show all transit","Underground track view","Elevated track view"};
    getButton(*w,"TransitView")->setCaptionText(views[std::clamp(t.view,0,3)]);
    getGameView().setMapDirty();getGameView().setDirty();
  };
  getButton(*w,"TransitView")->clicked.connect([this,refresh](Button*) {world->transit.view=(world->transit.view+1)%4;refresh();});
  getButton(*w,"TransitRefresh")->clicked.connect([refresh](Button*) {refresh();});
  getButton(*w,"TransitClose")->clicked.connect([this,w](Button*) {windowmanager->removeWindow(w);});
  windowmanager->addWindow(std::move(panel));refresh();
}

void Game::showCityHall(bool mayor, const char* page) {
  if(auto* existing = gui->findComponent("CityHall")) {
    auto* pages = dynamic_cast<SwitchComponent*>(existing->findComponent("HallPages"));
    if(pages && mayor) pages->switchComponent("MayorPage");
    if(pages && page) pages->switchComponent(page);
    return;
  }
  auto hall = dynamic_unique_cast<Window>(loadGUIFile("gui/cityhall.xml"));
  Window* w = hall.get();
  auto refresh = [this, w]() {
    const auto& e = world->evolution;
    const char* focus=e.planFocus==1?"Livable city":e.planFocus==2?"Skills / education":e.planFocus==3?"Technology hub":"None";
    getButton(*w,"PlanFocus")->setCaptionText(e.planState==CityEvolution::PLAN_PROPOSED ? std::string("Focus: ")+focus : std::string("Focus locked: ")+focus);
    getButton(*w,"PedestrianControl")->setCaptionText(e.pedestrians?"People visible - hide":"People hidden - show");
    getParagraph(*w,"CivicSummary")->setText("Happy people: " + std::to_string(e.approval)
      + "% | Homes with electricity: " + std::to_string(e.poweredHomes) + "/" + std::to_string(e.homes));
    getParagraph(*w,"ElectionSummary")->setText("Past elections: " + std::to_string(e.electionCount)
      + " | Last result: " + std::to_string(e.lastVote) + "% | Next in " + std::to_string(std::max(0,e.nextElection-world->total_time)) + " days");
    getParagraph(*w,"EconomySummary")->setText("Businesses: " + std::to_string(e.operatingBusinesses)
      + " working | " + std::to_string(e.newJobs) + " jobs | " + std::to_string(e.amenities) + " amenities");
    getParagraph(*w,"AtmosphereSummary")->setText("Spiritual wellbeing: "+std::to_string(e.spiritualCoverage)+"% homes | "+std::to_string(e.churchesWorking)+" churches open. Solar: "+std::to_string(e.solarPercent())+"%.");
    {
      const bool on=e.mayorMode!=0;
      getParagraph(*w,"MayorOn")->setText(on?"THE MAYOR IS ON: he builds inside his area and budget":"");
      getParagraph(*w,"MayorOff")->setText(on?"":"THE MAYOR IS OFF: he only gives advice and builds nothing");
      getButton(*w,"MayorToggle")->setCaptionText(on?"Turn the mayor OFF":"Turn the mayor ON");
      getParagraph(*w,"MayorScopeText")->setText("Works in: "+e.mayorScopeText()+(e.mayorScope==0?"":". Outlined in pink on the map."));
      static const char* paceNames[]={"careful","normal","fast"};
      getParagraph(*w,"MayorBudgetText")->setText(e.fullAutonomy
        ? "Full control (annual cap inactive): cash $"+std::to_string(world->total_money)
          +"; adaptive reserve; up to 6 buildings and 30 street tiles per month."
        : "Budget: $"+std::to_string(e.mayorBudget)+" a year (spent $"+std::to_string(e.mayorSpent)
          +"). Pace "+paceNames[std::clamp(e.mayorPace,1,3)-1]+": up to "+std::to_string(e.mayorBuildingsPerMonth())+" buildings and "
          +std::to_string(e.mayorRoadTilesPerMonth())+" road tiles a month. Keeps $25,000 safe.");
      getButton(*w,"MayorBudgetDown")->setCaptionText(e.fullAutonomy?"Annual cap inactive (full control)":"Lower annual cap: $"+std::to_string(e.mayorBudget));
      getButton(*w,"MayorBudgetUp")->setCaptionText(e.fullAutonomy?"Uses cash minus reserve":"Raise annual cap: $"+std::to_string(e.mayorBudget));
      getButton(*w,"MayorFullAutonomy")->setCaptionText(e.fullAutonomy&&e.mayorMode?"Full autonomy: ON":"Full autonomy: OFF");
      getParagraph(*w,"MayorReport")->setText(e.mayorReport);
      getButton(*w,"MayorPace")->setCaptionText(std::string("Pace: ")+paceNames[std::clamp(e.mayorPace,1,3)-1]);
      getButton(*w,"MayorScopeDistrict")->setCaptionText(e.mayorScope==1 && e.mayorDistrict>=0 && e.mayorDistrict<int(e.districts.size()) ? "Area: "+e.districts[e.mayorDistrict].name.substr(0,22)
        : e.mayorScope==2 ? "Area: drawn zone" : "Area: whole city");
      getButton(*w,"MayorServices")->setCaptionText(std::string("Basic services: ")+(e.mayorServices?"allowed":"blocked"));
      getButton(*w,"MayorParks")->setCaptionText(std::string("Parks: ")+(e.mayorParks?"allowed":"blocked"));
      getButton(*w,"MayorRoads")->setCaptionText(std::string("Roads: ")+(e.mayorRoads?"allowed":"blocked"));
      getParagraph(*w,"CityLine")->setText(namingDistrict==-2 ? "City name: "+namingBuffer+"_   (type, Enter = save, Esc = cancel)"
        : "City: "+(e.cityName.empty()?std::string("(no name yet)"):e.cityName)+"   |   Happy people: "+std::to_string(e.approval)+"%   |   Homes with power: "+std::to_string(e.poweredHomes)+"/"+std::to_string(e.homes));
    }
    getParagraph(*w,"WorkHead")->setText("WORKFORCE");
    getParagraph(*w,"AdviceHead")->setText("ADVISORS");
    auto pct=[](int per1000){ return std::to_string(per1000/10)+"%"; };
    getParagraph(*w,"WorkTech")->setText("Technicians (trained by schools): "+std::to_string(e.technicians)+" live here, "
      +std::to_string(e.techNeed)+" needed"+(e.techNeed?" ("+pct(e.techCoverage)+" staffed)":""));
    getParagraph(*w,"WorkEng")->setText("Engineers (trained by universities): "+std::to_string(e.engineers)+" live here, "
      +std::to_string(e.engNeed)+" needed"+(e.engNeed?" ("+pct(e.engCoverage)+" staffed)":""));
    getParagraph(*w,"WorkCampus")->setText(e.campuses ? std::to_string(e.campuses)+" technology campus(es) running at "+pct(e.campusSpeed())+" speed"
      : "No technology campus yet. It needs "+std::to_string(CityEvolution::CAMPUS_ENGINEERS)+" engineers and "+std::to_string(CityEvolution::CAMPUS_TECHS)+" technicians each.");
    getParagraph(*w,"WorkTrain")->setText("Last month: +"+std::to_string(e.techTrainedMonth)+" technicians, +"+std::to_string(e.engTrainedMonth)
      +" engineers trained; "+std::to_string(e.skilledLeftMonth)+" skilled people left.");
    {
      auto lines=e.planLines(*world);
      const char* ids[]={"PlanL1","PlanL2","PlanL3","PlanL4","PlanL5"};
      for(int i=0;i<5;++i) getParagraph(*w,ids[i])->setText(i<int(lines.size())?lines[i]:"");
      const bool proposed=e.planState==CityEvolution::PLAN_PROPOSED, active=e.planState==CityEvolution::PLAN_ACTIVE;
      getButton(*w,"PlanApprove")->setCaptionText(proposed?"Approve plan":active?"Plan is running":"Approve plan (none yet)");
      getButton(*w,"PlanReject")->setCaptionText(proposed?"Reject plan":active?"Reject plan (already running)":"Reject plan (none yet)");
      getButton(*w,"PlanAuto")->setCaptionText(std::string("Auto-approve plans: ")+(e.planAuto?(e.mayorMode?"on":"paused"):"off"));
    }
    {
      const auto& dl = e.districts;
      distSel = dl.empty() ? 0 : std::clamp(distSel, 0, int(dl.size()) - 1);
      getParagraph(*w,"DistHead")->setText("DISTRICTS: " + std::to_string(dl.size()) + " of " + std::to_string(CityEvolution::MAX_DISTRICTS));
      if(dl.empty()) {
        getParagraph(*w,"DistSel")->setText("No districts yet. Choose a type, press Draw zone, then drag a rectangle on the map.");
        getParagraph(*w,"DistDetail")->setText("A zone may cover buildings you already built. Matching buildings form a cluster with real bonuses.");
        getParagraph(*w,"DistList")->setText("");
      } else {
        getParagraph(*w,"DistSel")->setText("Selected " + std::to_string(distSel + 1) + " of " + std::to_string(dl.size()) + ": " + dl[distSel].name
          + " [" + CityEvolution::districtTypeName(dl[distSel].type) + "]");
        getParagraph(*w,"DistDetail")->setText(e.describeDistrict(distSel));
        std::string all="Use Previous and Next to inspect each district.";
        getParagraph(*w,"DistList")->setText(all);
      }
      getParagraph(*w,"DistNaming")->setText((namingDistrict >= 0) ? "Name: " + namingBuffer + "_    (type, Enter = save, Esc = cancel)"
        : "Map outlines: orange industrial, blue technology, green farming, yellow trade, purple university.");
      getButton(*w,"DistNewType")->setCaptionText(std::string("New zone type: ") + CityEvolution::districtTypeName(newDistType));
    }
    auto advice=e.advise(*world);
    const char* tags[]={"[Note] ","[Warning] ","[URGENT] "};
    for(int i=0;i<2;++i) {
      auto* para=getParagraph(*w,("Advice"+std::to_string(i+1)).c_str());
      if(i<int(advice.size())) para->setText(std::string(tags[std::clamp(advice[i].severity,0,2)])+advice[i].title+": "+advice[i].text);
      else para->setText("");
    }
    if(!e.news.empty()) getParagraph(*w,"News1")->setText("News: "+e.news[0]);
  };
  refresh();
  hallRefresh = refresh; hallPtr = w;
  auto close = [this,w]() { if(namingDistrict != -1) finishNaming(true); windowmanager->removeWindow(w); };
  getButton(*w,"CityServices")->clicked.connect([this,close](Button*) { close();showPublicServices(); });
  getButton(*w,"CityClose")->clicked.connect([close](Button*) { close(); });
  getButton(*w,"PedestrianControl")->clicked.connect([this,refresh](Button*) {
    world->evolution.pedestrians = !world->evolution.pedestrians;
    getGameView().printStatusMessage(world->evolution.pedestrians ? "Pedestrians on" : "Pedestrians off");
    refresh(); getGameView().setDirty();
  });
  auto* pages=dynamic_cast<SwitchComponent*>(w->findComponent("HallPages"));
  if(mayor && pages) pages->switchComponent("MayorPage");
  if(page && pages) pages->switchComponent(page);
  for(auto entry : {std::pair<const char*,const char*>{"CouncilTab","CouncilPage"},{"DistrictsTab","DistrictsPage"},{"PlanTab","PlanPage"},{"WeatherTab","WeatherPage"},{"MayorTab","MayorPage"}})
    getButton(*w,entry.first)->clicked.connect([pages,entry](Button*) {pages->switchComponent(entry.second);});
  getButton(*w,"DistNewType")->clicked.connect([this,refresh](Button*) {newDistType=newDistType%5+1;refresh();});
  getButton(*w,"DistNew")->clicked.connect([this,w](Button*) {
    if(namingDistrict != -1) finishNaming(true);
    windowmanager->removeWindow(w); hallPtr = nullptr;
    getGameView().beginDistrictZone(newDistType);
  });
  getButton(*w,"DistPrev")->clicked.connect([this,refresh](Button*) {int n=int(world->evolution.districts.size());if(n){distSel=(distSel+n-1)%n;focusDistrict(distSel);}refresh();});
  getButton(*w,"DistNext")->clicked.connect([this,refresh](Button*) {int n=int(world->evolution.districts.size());if(n){distSel=(distSel+1)%n;focusDistrict(distSel);}refresh();});
  getButton(*w,"DistRename")->clicked.connect([this](Button*) {if(!world->evolution.districts.empty()) startNaming(distSel);});
  getButton(*w,"DistRetype")->clicked.connect([this,refresh](Button*) {world->evolution.retypeDistrict(*world,distSel,newDistType);refresh();getGameView().setDirty();});
  getButton(*w,"DistDelete")->clicked.connect([this,refresh](Button*) {
    if(namingDistrict != -1) finishNaming(false);
    world->evolution.removeDistrict(distSel); distSel=std::max(0,distSel-1); refresh(); getGameView().setDirty();});
  getButton(*w,"PlanApprove")->clicked.connect([this,refresh](Button*) {world->evolution.approvePlan(*world);refresh();});
  getButton(*w,"PlanReject")->clicked.connect([this,refresh](Button*) {world->evolution.rejectPlan(*world);refresh();});
  getButton(*w,"PlanFocus")->clicked.connect([this,refresh](Button*) {auto& e=world->evolution;
    if(e.planState!=CityEvolution::PLAN_PROPOSED)
      getGameView().printStatusMessage("Focus changes apply to a proposed plan. The active plan keeps its agreed targets.");
    else { e.changePlanFocus(*world); getGameView().printStatusMessage("Plan focus changed; review the new targets before approval."); }
    refresh();});
  getButton(*w,"PlanAuto")->clicked.connect([this,refresh](Button*) {auto& e=world->evolution;e.planAuto=!e.planAuto;if(e.planAuto&&e.planState==CityEvolution::PLAN_PROPOSED)e.approvePlan(*world);refresh();});
  getButton(*w,"PlanBudgetDown")->clicked.connect([this,refresh](Button*) {auto& e=world->evolution;e.adjustPlanBudget(*world,-std::max(5000,e.planBudgetYear/10));refresh();});
  getButton(*w,"PlanBudgetUp")->clicked.connect([this,refresh](Button*) {auto& e=world->evolution;e.adjustPlanBudget(*world,std::max(5000,e.planBudgetYear/10));refresh();});
  getButton(*w,"MayorFullAutonomy")->clicked.connect([this,refresh](Button*){auto& e=world->evolution;e.setFullAutonomy(*world,!(e.fullAutonomy&&e.mayorMode));refresh();});
  getButton(*w,"MayorToggle")->clicked.connect([this,refresh](Button*) {
    auto& e=world->evolution; e.mayorMode = e.mayorMode==0 ? 2 : 0; if(!e.mayorMode)e.fullAutonomy=false;
    getGameView().printStatusMessage(e.mayorMode ? "The mayor is ON." : "The mayor is OFF: advice only.");
    refresh(); getGameView().setDirty();
  });
  getButton(*w,"MayorScopeDistrict")->clicked.connect([this,refresh](Button*) {
    auto& e=world->evolution; const int n=int(e.districts.size());
    // whole city -> district 1 -> ... -> district n -> whole city
    if(e.mayorScope==1 && e.mayorDistrict+1<n) { e.setMayorScopeDistrict(e.mayorDistrict+1); focusDistrict(e.mayorDistrict); }
    else if(e.mayorScope!=1 && n>0) { e.setMayorScopeDistrict(0); focusDistrict(0); }
    else e.setMayorScopeCity();
    refresh(); getGameView().setDirty();
  });
  getButton(*w,"MayorScopeZone")->clicked.connect([this,w](Button*) {
    windowmanager->removeWindow(w); hallPtr = nullptr;
    getGameView().beginDistrictZone(0); // 0 = zone for the mayor
  });
  getButton(*w,"MayorPace")->clicked.connect([this,refresh](Button*) {auto& e=world->evolution;e.mayorPace=e.mayorPace%3+1;refresh();});
  getButton(*w,"MayorBudgetDown")->clicked.connect([this,refresh](Button*) {
    auto& e=world->evolution; if(e.fullAutonomy){getGameView().printStatusMessage("Annual cap applies in limited control. Full autonomy uses available cash minus its reserve.");return;} static const int steps[]={5000,10000,25000,50000,100000,250000,500000,1000000};
    int i=0; for(int k=0;k<8;++k) if(steps[k]<e.mayorBudget) i=k; e.mayorBudget=steps[std::max(0,i)]; if(e.mayorBudget>=steps[0]&&steps[0]>=e.mayorBudget) e.mayorBudget=steps[0]; refresh(); });
  getButton(*w,"MayorBudgetUp")->clicked.connect([this,refresh](Button*) {
    auto& e=world->evolution; if(e.fullAutonomy){getGameView().printStatusMessage("Annual cap applies in limited control. Full autonomy uses available cash minus its reserve.");return;} static const int steps[]={5000,10000,25000,50000,100000,250000,500000,1000000};
    for(int k=0;k<8;++k) if(steps[k]>e.mayorBudget) {e.mayorBudget=steps[k];break;} refresh(); });
  getButton(*w,"MayorServices")->clicked.connect([this,refresh](Button*) {world->evolution.mayorServices=!world->evolution.mayorServices;refresh();});
  getButton(*w,"MayorParks")->clicked.connect([this,refresh](Button*) {world->evolution.mayorParks=!world->evolution.mayorParks;refresh();});
  getButton(*w,"MayorRoads")->clicked.connect([this,refresh](Button*) {world->evolution.mayorRoads=!world->evolution.mayorRoads;refresh();});
  getButton(*w,"RenameCity")->clicked.connect([this](Button*) {startNaming(-2);});
  getButton(*w,"AllAdvice")->clicked.connect([this](Button*) {showCityAdvisors();});
  windowmanager->addWindow(std::move(hall));
}

void Game::focusDistrict(int index) {
  if(index < 0 || index >= int(world->evolution.districts.size())) return;
  const auto& d = world->evolution.districts[index];
  getGameView().show(MapPoint((d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2));
}

void Game::refreshHall() {
  if(hallRefresh && hallPtr && gui->findComponent("CityHall") == hallPtr) hallRefresh();
}

void Game::startNaming(int index) {
  if(index != -2 && (index < 0 || index >= int(world->evolution.districts.size()))) return;
  namingDistrict = index; namingBuffer.clear();
  SDL_StartTextInput(window);
  refreshHall();
}

void Game::finishNaming(bool save) {
  if(namingDistrict == -1) return;
  if(save && !namingBuffer.empty()) {
    if(namingDistrict == -2) {
      world->evolution.renameCity(namingBuffer);
      SDL_SetWindowTitle(window, ("City Evolution - " + world->evolution.cityName).c_str());
    } else world->evolution.renameDistrict(namingDistrict, namingBuffer);
  }
  namingDistrict = -1; namingBuffer.clear();
  SDL_StopTextInput(window);
  refreshHall(); getGameView().setDirty();
}

void Game::mayorZoneDrawn(int x0, int y0, int x1, int y1) {
  world->evolution.setMayorScopeZone(*world, x0, y0, x1, y1);
  showCityHall(true);
  refreshHall();
}

void Game::districtDrawn(int index) {
  distSel = index;
  showCityHall(false, "DistrictsPage");
  startNaming(index);
}

void Game::showCityAdvisors() {
  DialogBuilder dialog;
  dialog.titleText("City advisors").buttonSet(DialogBuilder::ButtonSet::OK);
  dialog.messageAddText("City Steward: elections report public approval and never remove your building control.");
  const auto& e = world->evolution;
  dialog.messageAddText("Approval " + std::to_string(e.approval) + "% | Solar output " + std::to_string(e.solarPercent()) + "%. Keep coal or wind generation and substations available for dark or wet periods.");
  {
    const char* tags[] = {"[Note] ", "[Warning] ", "[URGENT] "};
    for(const auto& a : world->evolution.advise(*world))
      dialog.messageAddText(std::string(tags[std::clamp(a.severity, 0, 2)]) + a.title + ": " + a.text);
  }
  const auto debtAdvice = world->civic.debtAdvice(*world);
  if(!debtAdvice.empty()) dialog.messageAddText(debtAdvice);
  int supplyWarnings = 0, transitWarnings = 0;
  for(const auto* building : world->map.constructions) {
    if(building->isDead()) continue;
    const std::string location = " (" + std::to_string(building->point.x)
      + ", " + std::to_string(building->point.y) + ")";
    if(const auto* home = dynamic_cast<const Residence*>(building); home && supplyWarnings < 3) {
      const auto advice = home->foodSupplyAdvice();
      if(!advice.empty()) {
        dialog.messageAddText("Housing" + location + ". " + advice + ".");
        ++supplyWarnings;
      }
    }
    if(const auto* stop = dynamic_cast<const TransitFacility*>(building);
       stop && stop->mode() == TransitSystem::BUS && stop->overloaded() && transitWarnings < 3) {
      const bool terminal = building->constructionGroup->group == GROUP_BUS_TERMINAL;
      dialog.messageAddAction("Go to " + building->constructionGroup->name + location,
        [this, point=building->point]() {
          if(hallPtr && gui->findComponent("CityHall") == hallPtr) {
            if(namingDistrict != -1) finishNaming(true);
            windowmanager->removeWindow(static_cast<Window*>(hallPtr));
            hallPtr = nullptr;
          }
          getButtonPanel().selectQueryTool();
          getGameView().show(point);
          getMpsMap().query(world->map(point));
          getMiniMap().switchView("MapMPS");
        });
      dialog.messageAddText("Estimated demand: "
        + std::to_string(stop->requestedTrips) + " monthly trips, but handles up to "
        + std::to_string(stop->monthlyCapacity()) + ". "
        + (terminal ? "Add a nearby bus stop to share the load."
                    : "Consider a bus terminal or a second stop closer to part of the neighbourhood."));
      ++transitWarnings;
    }
  }
  int homes = 0, schools = 0, universities = 0, issues = 0, darkHomes = 0;
  for(const Construction* building : world->map.constructions) {
    const auto group = building->constructionGroup->group;
    if(GROUP_IS_RESIDENCE(group)) { ++homes; if(!static_cast<const Residence*>(building)->hasElectricity()) ++darkHomes; }
    if(group == GROUP_SCHOOL) ++schools;
    if(group == GROUP_UNIVERSITY) ++universities;
    const std::string coordinates = " (" + std::to_string(building->point.x) + ", " + std::to_string(building->point.y) + ")";
    if(group >= GROUP_TECH_CAMPUS && group <= GROUP_FOODHALL && issues < 4) {
      std::string missing;
      for(Commodity stuff = STUFF_INIT; stuff < STUFF_COUNT; stuff++) {
        int daily = building->commodityMaxCons[stuff] / NUMOF_DAYS_IN_MONTH;
        if(daily > 0 && building->commodityCount[stuff] < daily)
          missing += (missing.empty() ? "" : ", ") + std::string(commodityStandardName(stuff));
      }
      if(!missing.empty()) {
        dialog.messageAddText(building->constructionGroup->name + coordinates + " needs " + missing
          + ". Check neighboring transport and input producers.");
        ++issues;
      }
    }
    if(group != GROUP_PORT && group != GROUP_INDUSTRY_L && group != GROUP_INDUSTRY_H && group != GROUP_WAREHOUSE) continue;
    bool road = false, rail = false;
    for(const Construction* neighbor : building->neighbors) {
      const auto ng = neighbor->constructionGroup->group;
      road |= ng == GROUP_TRACK || ng == GROUP_ROAD || ng == GROUP_TRACK_BRIDGE || ng == GROUP_ROAD_BRIDGE;
      rail |= ng == GROUP_RAIL || ng == GROUP_RAIL_BRIDGE;
    }
    const std::string where = " (" + std::to_string(building->point.x) + ", " + std::to_string(building->point.y) + ")";
    if(!road && !rail && issues < 4) {
      dialog.messageAddText(building->constructionGroup->name + where
        + " has no direct transport neighbor. Connect a road, track or railway to its edge.");
      ++issues;
    }
    if(group == GROUP_WAREHOUSE && rail && !road && issues < 4) {
      dialog.messageAddText("Freight warehouse" + where + ": add a road or track at its edge to distribute rail freight locally.");
      ++issues;
    }
    if(group == GROUP_PORT && issues < 4) {
      bool stockpiled = false;
      for(Commodity stuff : {STUFF_COAL, STUFF_ORE, STUFF_STEEL, STUFF_GOODS}) {
        const auto& rule = static_cast<const Port*>(building)->commodityRuleCount[stuff];
        if(rule.take && rule.maxload > 0 && building->commodityCount[stuff] > rule.maxload * 4 / 5)
          stockpiled = true;
      }
      if(stockpiled) {
        dialog.messageAddText("Imported stock is piling up at port" + where
          + (rail ? ". Check that its transport network reaches factories that consume those materials."
                  : ". Consider a railway toward your industrial district and check factory input demand."));
        ++issues;
      }
    }
  }
  if(schools && !universities)
    dialog.messageAddText("Education: schools are established. As technology rises, add a university and supply it with labor and goods.");
  if(!issues && homes && schools)
    dialog.messageAddText("No direct factory/port access problem found. This is an initial diagnostic; inspect building inventories for production shortages.");
  dialog.messageAddText("Map: " + std::to_string(world->map.len()) + " x " + std::to_string(world->map.len())
    + ". Larger boundaries are selected when creating a new world.");
  dialog.build();
}

void
Game::setWorld(std::unique_ptr<World>&& world) {
  this->world = std::move(world);
  this->world->transit.update(*this->world,false);
  // Old forced sky/weather modes reset whenever a city is entered.
  this->world->evolution.skyMode = 0;
  this->world->evolution.weatherMode = 0;

  for(auto w : {&warnBullWater, &warnBullShanty, &warnBullMonument})
    w->onAccept.clear();
  getButtonPanel().selectQueryTool();
  getButtonPanel().updateTech();
  getMpsMap().query(nullptr);
  gameview->show(this->world->map.recentPoint);
  getSound()->setTechLevel(this->world->tech_level);

  inventions.assign({
    &potteryConstructionGroup,       //  0.0
    &monumentConstructionGroup,      //  0.0
    &waterwellConstructionGroup,     //  0.0
    &communeConstructionGroup,       //  0.0
    &organic_farmConstructionGroup,  //  0.0
    &trackConstructionGroup,         //  0.0
    // &residenceLLConstructionGroup,   //  0.0
    // &residenceMLConstructionGroup,   //  0.0
    &residenceHLConstructionGroup,   //  0.0
    &oremineConstructionGroup,       //  0.0
    &tipConstructionGroup,           //  0.0
    &marketConstructionGroup,        //  0.0
    &schoolConstructionGroup,        //  0.1
    &parklandConstructionGroup,      //  0.2
    // &parkpondConstructionGroup       //  0.2
    &blacksmithConstructionGroup,    //  0.3
    &cricketConstructionGroup,       //  1.2
    &fireStationConstructionGroup,   //  2.2
    &millConstructionGroup,          //  2.5
    &windmillConstructionGroup,      //  3.0
    &portConstructionGroup,          //  3.5
    &roadConstructionGroup,          //  5.0
    &coalmineConstructionGroup,      //  8.5
    &healthCentreConstructionGroup,  // 11.0
    &universityConstructionGroup,    // 15.0
    &industryLightConstructionGroup, // 16.0
    &industryHeavyConstructionGroup, // 17.0
    &railConstructionGroup,          // 18.0
    &coal_powerConstructionGroup,    // 20.0
    // &substationConstructionGroup,    // 20.0
    // &powerlineConstructionGroup,     // 20.0
    &recycleConstructionGroup,       // 23.2
    // &residenceLHConstructionGroup,   // 30.0
    // &residenceMHConstructionGroup,   // 30.0
    &residenceHHConstructionGroup,   // 30.0
    &windpowerConstructionGroup,     // 45.0
    &solarPowerConstructionGroup,    // 50.0
    &rocketPadConstructionGroup,     // 75.0
  });
  inventions.sort(
    [](const ConstructionGroup *a, const ConstructionGroup *b)
    { return a->tech < b->tech; }
  );
  while(!inventions.empty()
    && inventions.front()->tech <= this->world->tech_level
  )
    inventions.pop_front();
}

void
Game::updateTech() {
  while(!inventions.empty()
    && inventions.front()->tech <= world->tech_level
  ) {
    const ConstructionGroup& cstGrp = *inventions.front();
    DialogBuilder()
      .titleText(_("New Invention!"))
      .messageAddTextBold(fmt::format(_("{} Available"), _(cstGrp.name_plural)))
      .messageAddText(fmt::format(_("You have reached the tech level which"
          " enables you to build {}. See the help screen for more information"
          " about this type of building."),
        _(cstGrp.name_plural)))
      .imageFile("images/gui/dialogs/invention.png")
      .buttonSet(DialogBuilder::ButtonSet::OK)
      .build();
    inventions.pop_front();
  }
}

UserOperation&
Game::getUserOperation() {
  return userOperation;
}

const UserOperation&
Game::getUserOperation() const {
  return userOperation;
}

void
Game::setUserOperation(const UserOperation& op) {
  userOperation = op;
  if(op.action==UserOperation::ACTION_BUILD&&op.constructionGroup){int g=op.constructionGroup->group;if(g==GROUP_METRO_TRACK)world->transit.view=2;else if(g==GROUP_ELEVATED_TRACK)world->transit.view=3;else world->transit.view=0;getGameView().setMapDirty();}

  for(auto w : {&warnBullWater, &warnBullShanty, &warnBullMonument})
    w->accepted = false;;
  getGameView().setCursorSize(userOperation.cursorSize());
  getGameView().showToolInfo();
}

void
Game::executeUserOperation(MapPoint point) {
  try {
    if(!world->map.is_visible(point))
      OutsideMapMessage::create(point)->throwEx();

    switch(userOperation.action) {
    case UserOperation::ACTION_QUERY: {
      // TODO: do dialogs on double-click, not 'second-click'
      if(getMpsMap().tile == world->map(point)) {
        Construction *cst = world->map(point)->reportingConstruction;
        switch(world->map(point)->getGroup()) {
        case GROUP_MARKET:
          new Dialog(*this, EDIT_MARKET, cst->point);
          break;
        case GROUP_PORT:
          new Dialog(*this, EDIT_PORT, cst->point);
          break;
        case GROUP_ROCKET:
          RocketPad *rocket = dynamic_cast<RocketPad *>(cst);
          assert(rocket);
          if(rocket->stage == RocketPad::AWAITING) {
            new Dialog(*this, ASK_LAUNCH_ROCKET, cst->point);
          }
          break;
        }
      }

      getMpsMap().query(world->map(point));
      getMiniMap().switchView("MapMPS");

      break;
    }

    case UserOperation::ACTION_BUILD: {
      // TODO: move dependence on SDL_GetKeyboardState elsewhere (GameView?)
      if(userOperation.constructionGroup == &parklandConstructionGroup
        && SDL_GetKeyboardState(NULL)[SDL_SCANCODE_K]
      )
        userOperation.constructionGroup = &parkpondConstructionGroup;

      break;
    }

    case UserOperation::ACTION_BULLDOZE: {
      if(Message::ptr msg; !userOperation.isAllowedHere(*world, point, msg)) {
        // don't issue a warning if we can't bulldoze anyway
        break;
      }

      unsigned short grp = world->map(point)->getGroup();
      decltype(warnBullWater) *warnStatus = nullptr;
      switch(grp) {
      case GROUP_MONUMENT:
        warnStatus = &warnBullMonument;
        break;
      case GROUP_RIVER:
        warnStatus = &warnBullWater;
        break;
      case GROUP_SHANTY:
        warnStatus = &warnBullShanty;
        break;
      }
      if(!warnStatus || warnStatus->accepted)
        break;

      Construction *cst = world->map(point)->reportingConstruction;
      warnStatus->onAccept.connect([this, point, cst, grp]() {
        // make sure things haven't changed
        // there must be a better way to do this
        if(world->map(point)->reportingConstruction == cst
          && world->map(point)->getGroup() == grp
        ) {
          try{
            userOperation.execute(*world, point);
            getMpsMap().setTile(world->map(point));
            getSound()->playSound("Raze");
          } catch(const Message::Exception& ex) {
            handleMessage(ex.getMessage());
          }
        }
      });

      if(warnStatus->pending)
        return;

      DialogBuilder()
        .titleText("Warning")
        .messageAddTextBold("Warning:")
        .messageAddText(fmt::format(_("Bulldozing a {} costs a lot of money."),
          _(world->map(point)->getConstructionGroup()->name)))
        .messageAddText("Want to bulldoze?")
        .imageFile("images/gui/dialogs/warning.png")
        // TODO: use "Bulldoze"/"Leave It" buttons
        .buttonSet(DialogBuilder::ButtonSet::YESNO)
        .onYes([warnStatus]() {
          warnStatus->pending = false;
          warnStatus->accepted = true;
          warnStatus->onAccept();
        })
        .onNo([warnStatus]() {
          warnStatus->pending = false;
          warnStatus->onAccept.clear();
        })
        .build();
      warnStatus->pending = true;

      return;
    }

    case UserOperation::ACTION_EVACUATE:
    case UserOperation::ACTION_FLOOD:
      break;

    default: {
      assert(false);
    }
    }

    userOperation.execute(*world, point);

    getMpsMap().setTile(world->map(point));
    if(userOperation.action != UserOperation::ACTION_BULLDOZE)
      getSound()->playSound(*world->map(point));
    else
      getSound()->playSound("Raze");
  } catch(const Message::Exception& ex) {
    handleMessage(ex.getMessage());
  }
}

Component&
Game::getGui() const {
  return *gui;
}

WindowManager&
Game::getWindowManager() const {
  return *windowmanager;
}

GameView&
Game::getGameView() const {
  assert(gameview);
  return *gameview;
}

MiniMap&
Game::getMiniMap() const {
  assert(minimap);
  return *minimap;
}

EconomyGraph&
Game::getEconomyGraph() const {
  assert(economygraph);
  return *economygraph;
}

MpsMap&
Game::getMpsMap() const {
  return *mpsmap;
}

MpsFinance&
Game::getMpsFinance() const {
  return *mpsfinance;
}

LCPBar&
Game::getPBar1() const {
  return *pbar1;
}

LCPBar&
Game::getPBar2() const {
  return *pbar2;
}

ButtonPanel&
Game::getButtonPanel() const {
  return *buttonpanel;
}


void
Game::run() {
    struct StopRainOnExit {
      ~StopRainOnExit() { getSound()->stopRainAmbience(); }
    } stopRainOnExit;
    assert(world);
    if(!world) return;

    // init GUI
    Desktop* desktop = dynamic_cast<Desktop*> (gui.get());
    if(!desktop)
      throw std::runtime_error("Toplevel component is not a Desktop");
    {auto ui=resizeUserInterface();gui->resize(ui.width,ui.height);}
    DialogBuilder::setDefaultWindowManager(dynamic_cast<WindowManager *>(
      desktop->findComponent("windowManager")));
    world->setUpdated(World::Updatable::MONEY);

    getButtonPanel().selectQueryTool();

    int frame = 0;
    bool new_day = true, new_month = true, new_year = true;
    Uint32 next_execute = ~0, next_animate = ~0, next_gui = 0, next_fps = 0;
    [[maybe_unused]]
    Uint32 prev_execute = 0, prev_animate = 0, prev_gui = 0, prev_fps = 0;
    Uint32 next_task;
    Uint32 next_building_animation = 0;

    running = true;
    while(true) {
        getSound()->update();
        next_task = std::min({next_execute, next_animate, next_gui, next_fps});
        while(true) {
            if(!running) return;
            SDL_Event event;
            int event_timeout = next_task - SDL_GetTicks();
            if(event_timeout < 0) event_timeout = 0;
            int status = SDL_WaitEventTimeout(&event, event_timeout);
            if(!status) break; // timed out

            mapUserInterfaceEvent(event);
            switch(event.type) {
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                    videoSizeChanged(event.window.data1, event.window.data2);
                    {auto ui=resizeUserInterface();gui->resize(ui.width,ui.height);}
                    int windowWidth=0,windowHeight=0;SDL_GetWindowSize(window,&windowWidth,&windowHeight);
                    getConfig()->videoX.session = windowWidth;
                    getConfig()->videoY.session = windowHeight;
                    getConfig()->videoX.sessionToConfig();
                    getConfig()->videoY.sessionToConfig();
                    break;
                }
                case SDL_EVENT_WINDOW_MOUSE_ENTER:
                case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
                    Event gui_event(event);
                    gui->event(gui_event);
                    break;
                    }
                case SDL_EVENT_TEXT_INPUT: {
                  if(namingDistrict != -1 && namingBuffer.size() < 32) { namingBuffer += event.text.text; refreshHall(); }
                  break;
                }
                case SDL_EVENT_KEY_UP: {
                  if(namingDistrict != -1) break; // typing a name: no shortcuts
                  Event gui_event(event);
                  if(gui_event.key == SDLK_ESCAPE) {
                    getButtonPanel().selectQueryTool();
                    break;
                  }
                  if(gui_event.key == SDLK_B) {
                    getButtonPanel().toggleBulldozeTool();
                    break;
                  }
                  if(gui_event.key == SDLK_F6) showCityHall();
                  if(gui_event.key == SDLK_F7) showPublicServices();
                  if(gui_event.key == SDLK_F1) {
                    helpWindow->showTopic("help");
                      break;
                  }
                  if(gui_event.key == SDLK_F12) {
                    quickSave();
                    break;
                  }
                  if(gui_event.key == SDLK_F9) {
                    quickLoad();
                    break;
                  }
#ifdef DEBUG
                  if(gui_event.key == SDLK_F5) {
                    testAllHelpFiles();
                    break;
                  }
#endif
                  int need_break=true;
                  switch(gui_event.key) {
                    case SDLK_GRAVE: getMiniMap().mapViewChangeDisplayMode(MiniMap::NORMAL); break;
                    case SDLK_1: getMiniMap().mapViewChangeDisplayMode(MiniMap::STARVE); break;
                    case SDLK_2: getMiniMap().mapViewChangeDisplayMode(MiniMap::UB40); break;
                    case SDLK_3: getMiniMap().mapViewChangeDisplayMode(MiniMap::POWER); break;
                    case SDLK_4: getMiniMap().mapViewChangeDisplayMode(MiniMap::FIRE); break;
                    case SDLK_5: getMiniMap().mapViewChangeDisplayMode(MiniMap::CRICKET); break;
                    case SDLK_6: getMiniMap().mapViewChangeDisplayMode(MiniMap::HEALTH); break;
                    case SDLK_7: getMiniMap().mapViewChangeDisplayMode(MiniMap::TRAFFIC); break;
                    case SDLK_8: getMiniMap().mapViewChangeDisplayMode(MiniMap::POLLUTION); break;
                    case SDLK_9: getMiniMap().mapViewChangeDisplayMode(MiniMap::COAL); break;
                    case SDLK_0: getMiniMap().mapViewChangeDisplayMode(MiniMap::COMMODITIES); break;
                    default:  need_break=false;
                  }
                  if (need_break) break;

                  gui->event(gui_event);
                  break;
                }
                case SDL_EVENT_MOUSE_MOTION:
                case SDL_EVENT_MOUSE_BUTTON_UP:
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_WHEEL:
                case SDL_EVENT_KEY_DOWN: {
                    if(namingDistrict != -1) {
                      const SDL_Keycode k = event.key.key;
                      if(k == SDLK_RETURN || k == SDLK_KP_ENTER) finishNaming(true);
                      else if(k == SDLK_ESCAPE) finishNaming(false);
                      else if(k == SDLK_BACKSPACE && !namingBuffer.empty()) {
                        do { namingBuffer.pop_back(); } while(!namingBuffer.empty() && (static_cast<unsigned char>(namingBuffer.back()) & 0xC0) == 0x80);
                        refreshHall();
                      }
                      break;
                    }
                    Event gui_event(event);
                    gui->event(gui_event);
                    break;
                }
                case SDL_EVENT_QUIT: {
                    saveCityNG(*world, getConfig()->userDataDir.get()
                      / "9_currentGameNG.scn.gz");
                    // push the QUIT event back for main menu to handle
                    int s = SDL_PushEvent(&event);
                    assert(s == 1);
                    return;
                }
                default:
                    break;
            }

            if(desktop->needsRedraw())
              next_task = 0;
        }

        Uint32 tick = SDL_GetTicks();
        get_real_time_with(tick);
        frame++;

        if(tick >= next_gui) { // gui update
            const int weather = world->evolution.currentWeather();
            getSound()->setRainAmbience(weather == 3 ? .42f : weather == 2 ? .28f : 0.f);
            // fire update event
            gui->event(Event((tick - prev_gui) / 1000.0f));

            // show any pending dialogs
            if(world->civic.zombieNoticePending && !blockingDialogIsOpen) { world->civic.zombieNoticePending=false;DialogBuilder().titleText("Pollution outbreak").messageAddText("Your city was so polluted that zombies appeared. S.T.A.R.S. report for duty! Police crews are responding; reduce pollution to prevent another outbreak.").buttonSet(DialogBuilder::ButtonSet::OK).build(); }
            while(Message::ptr message = world->popMessage())
              handleMessage(message);

            if(world->isUpdated(World::Updatable::MAP)) {
              getGameView().setDirty();
              getGameView().setMapDirty();
              getMiniMap().setDirty();
              getMiniMap().setMapDirty();
              world->clearUpdated(World::Updatable::MAP);
            }

            // update the help window
            // TODO: Why is this not triggered by the gui update?
            helpWindow->update();

            if(new_day) {
              new_day = false;
              updateDate();
            }
            if(new_month) {
              new_month = false;
              getPBar1().refresh();
              getPBar2().refresh();
              getMpsMap().refresh();
              getEconomyGraph().setDirty();
              getGameView().setMapDirty();
              getMiniMap().setMapDirty();
            }
            if(new_year) {
              new_year = false;
              getButtonPanel().updateTech();
              getSound()->setTechLevel(this->world->tech_level);
              getMpsFinance().refresh();
              updateTech();
            }
            if(world->isUpdated(World::Updatable::MONEY)) {
              updateMoney();
              world->clearUpdated(World::Updatable::MONEY);
            }

            // reschedule
            next_gui = tick + 1000/30; // 30 FPS
            prev_gui = tick;
        }
        if(tick >= next_execute && simDelay != SIM_DELAY_PAUSE && !blockingDialogIsOpen && !windowmanager->hasOpenWindows()) { // execute
            // simulation timestep
            int previousElection = world->evolution.electionCount;
            world->do_time_step();
            if(world->evolution.electionCount != previousElection)
              getGameView().printStatusMessage("Election results are ready in City hall. You remain City Steward.");

            new_day = true;
            if(world->total_time % NUMOF_DAYS_IN_MONTH == 0)
              new_month = true;
            if(world->total_time % NUMOF_DAYS_IN_YEAR == 0)
              new_year = true;

            // reschedule
            next_execute = tick + simDelay;
            prev_execute = tick;
        }
        if(tick >= next_animate) { // game animation
          // animate
          world->evolution.advance(std::min<Uint32>(250, tick - prev_animate));
          if(tick >= next_building_animation) {
            world->do_animate(tick);
            next_building_animation = tick + ANIMATE_DELAY;
          }
          // TODO: see if texture fetching can be done in initialization
          //fetch remaining textures in order loader thread can exit
          if(getGameView().textures_ready && getGameView().remaining_images)
            getGameView().fetchTextures();
          getGameView().setDirty();
          getGameView().setMapDirty();

          // reschedule
          const bool movingStreetActors = world->evolution.pedestrians
            || getConfig()->carsEnabled.get() || !world->transit.routes.empty();
          next_animate = tick + (movingStreetActors ? 1000 / 30 : ANIMATE_DELAY);
          prev_animate = tick;
        }
        if(tick >= next_fps) { // fps
#ifdef DEBUG_FPS
          printf("FPS: %d\n", (frame*1000) / (tick - prev_fps));
#endif
          getEconomyGraph().newFPS(frame);
          frame = 0;

          // reschedule
          next_fps = tick + 1000;
          prev_fps = tick;
        }

        if(desktop->needsRedraw()) { // redraw
          desktop->draw(*painter);
          painter->updateScreen();
        }

        // this is kind of janky, but it works for now
        if( simDelay == SIM_DELAY_PAUSE || blockingDialogIsOpen || windowmanager->hasOpenWindows() ) {
          // deschedule execute and animate
          next_execute = ~0;
          next_animate = ~0;
        }
        else if(next_execute == (Uint32)~0 || next_animate == (Uint32)~0) {
            // reschedule execute and animate
            next_execute = tick;
            next_animate = tick;
            prev_animate = tick;
            next_building_animation = tick;
        }
    }
}

void
Game::handleMessage(Message::ptr message_) {
  if(auto requirement = dynamic_message_cast<CityRequirementMessage>(message_)) {
    DialogBuilder().titleText("City requirements").messageAddText(requirement->text)
      .buttonSet(DialogBuilder::ButtonSet::OK).build();
    return;
  }
  if(FireStartedMessage::ptr message =
    dynamic_message_cast<FireStartedMessage>(message_)
  ) {
    DialogBuilder()
      .titleText(_("Fire!"))
      .messageAddTextBold(_("A Fire has Started"))
      .messageAddText(fmt::format(_("A fire has broken out in a {}! You should"
          " address this promptly before the whole city burns down."),
        _(message->getGroup().name)))
      .imageFile("images/gui/dialogs/warning.png") // TODO: fire icon
      .buttonSet(DialogBuilder::ButtonSet::OK)
      .build();
  }
  else if(OutOfMoneyMessage::ptr message =
    dynamic_message_cast<OutOfMoneyMessage>(message_)
  ) {
    if(message->isOutOfCredit()) {
        DialogBuilder()
          .titleText(_("Warning!"))
          .messageAddTextBold(_("Out of Credit"))
          .messageAddText(_("You are deep in debt and at the end of your"
            "credit line. You are about to find out what happens when "
            "government checks bounce."))
          .imageFile("images/gui/dialogs/warning.png") // TODO: money icon
          .buttonSet(DialogBuilder::ButtonSet::OK)
          .build();
    }
    else {
      DialogBuilder()
        .titleText(_("Warning!"))
        .messageAddTextBold(_("Out of Money"))
        .messageAddText(_("You just spent all your money. Further spending "
          "will require you to take out a loan which you must pay interest "
          "on. And certain projects you are not allowed to pay for with "
          "credit. The legislative council strongly advises you learn to "
          "budget."))
        .imageFile("images/gui/dialogs/warning.png") // TODO: money icon
        .buttonSet(DialogBuilder::ButtonSet::OK)
        .build();
    }
  }
  else if(RocketReadyMessage::ptr message =
    dynamic_message_cast<RocketReadyMessage>(message_)
  ) {
    DialogBuilder()
      .titleText(_("Rocket Ready"))
      .messageAddTextBold(_("A Rocket is Ready for Launch"))
      .messageAddText(fmt::format(_("The rocket at {} has finished construction"
          " and is ready for takeoff. You may choose to launch now or later. If"
          " you choose to wait, beware it costs money to keep the rocket in"
          " tip-top shape until launch day."),
        message->getPoint()))
      .messageAddText(_("Launch now?"))
      .imageFile("images/gui/dialogs/info.png") // TODO: rocket icon
      .buttonSet(DialogBuilder::ButtonSet::YESNO)
      .onYes([this, point = message->getPoint()]() {
        RocketPad *rocket = dynamic_cast<RocketPad *>(
          getWorld().map(point)->construction);
        if(!rocket) return; // it must have been deleted
        rocket->launch_rocket();
      })
      .build();
  }
  else if(RocketResultMessage::ptr message =
    dynamic_message_cast<RocketResultMessage>(message_)
  ) {
    DialogBuilder dialog;
    dialog
      .titleText(_("Rocket Launched"))
      .imageFile("images/gui/dialogs/info.png") // TODO: rocket icon
      .buttonSet(DialogBuilder::ButtonSet::OK);
    switch(message->getResult()) {
    case RocketResultMessage::LaunchResult::FAIL:
      dialog
        .messageAddTextBold(_("Rocket Crashed"))
        .messageAddText(_("Looks like your rocket technology leaves some to be"
          " desired. Thankfully this was a test flight, so no people were on"
          " board. The scientists say they have identified the cause of the"
          " crash and are confident future launches will go smoothly."))
        .imageFile("images/gui/dialogs/warning.png");
      break;
    case RocketResultMessage::LaunchResult::SUCCESS:
      dialog
        .messageAddTextBold(_("Test Launch Successful"))
        .messageAddText(_("The rocket successfully completed a test flight."));
        // TODO: Add information about # of successful test launches so far
        //       and/or # of remaining successful test launches before
        //       evacuating people.
      break;
    case RocketResultMessage::LaunchResult::EVAC:
      dialog
        .messageAddTextBold(_("Evacuation Launch Successful"))
        .messageAddText(_("You have successfully evacuated 1000 people. You "
          "may consider reducing the size of your economy appropriately."));
      break;
    case RocketResultMessage::LaunchResult::EVAC_WIN:
      dialog
        .titleText("You Won!")
        .messageAddTextBold(_("You Evacuated Everyone!"))
        .messageAddText(_("Congradulations! You have successfully evacuated "
          "everyone from the city and won the game."));
      break;
    default:
      assert(false);
    }
    dialog.build();
  }
  else if(NoPeopleLeftMessage::ptr message =
    dynamic_message_cast<NoPeopleLeftMessage>(message_)
  ) {
    DialogBuilder()
      .titleText(_("You Lost!"))
      .messageAddTextBold(_("Everyone Died!"))
      .messageAddText(_("Your leadership drove the city to ruin, and "
        "all the people died."))
      .imageFile("images/gui/dialogs/warning.png")
      .buttonSet(DialogBuilder::ButtonSet::OK)
      .build();
  }
  else if(SustainableEconomyMessage::ptr message =
    dynamic_message_cast<SustainableEconomyMessage>(message_)
  ) {
    DialogBuilder()
      .titleText(_("You Won!"))
      .messageAddTextBold(_("Economy Sustainable"))
      .messageAddText(_("Well done! Thanks to your excellent leadership, the "
        "city council has determined the economy is sustainable and "
        "(at least theoretically) may continue indefinitely in prosperity. "
        "Therefore, you may retire from your position -- of course with a "
        "substantial pension."))
      .imageFile("images/gui/dialogs/info.png")
      .buttonSet(DialogBuilder::ButtonSet::OK)
      .build();
  }
  else if(OutsideMapMessage::ptr message =
    dynamic_message_cast<OutsideMapMessage>(message_)
  ) {
    // silently ignore requests to do stuff outside the map
  }
  else if(CannotBuildMessage::ptr message =
    dynamic_message_cast<CannotBuildMessage>(message_)
  ) {
    CannotBuildHereMessage::ptr hereMessage =
      dynamic_message_cast<CannotBuildHereMessage>(message);
    Message::ptr reason_ = message->getReason();
    DialogBuilder dialog;
    dialog
      .titleText(_("Cannot Build"))
      .messageAddTextBold(fmt::format(
        hereMessage ? _("Cannot build a {} here.") : _("Cannot build a {}."),
        _(message->getGroup().name)))
      .imageFile("images/gui/dialogs/warning.png")
      .buttonSet(DialogBuilder::ButtonSet::OK);
    if(OutOfMoneyMessage::ptr reason =
      dynamic_message_cast<OutOfMoneyMessage>(reason_)
    ) {
      if(reason->isOutOfCredit()) {
        dialog.messageAddText(_("You do not have sufficient credit to build "
          "this."));
      }
      else {
        dialog.messageAddText(_("You cannot build this on credit."));
      }
    }
    else if(NotEnoughTechMessage::ptr reason =
      dynamic_message_cast<NotEnoughTechMessage>(reason_)
    ) {
      const std::string formatStr = hereMessage
        ? _("Your tech level is too low. Building a {} here requires {:.1f}"
          " tech level, but you have only {:.1f} tech level.")
        : _("Your tech level is too low. Building a {} requires {:.1f}"
          " tech level, but you have only {:.1f} tech level.");
      dialog.messageAddText(fmt::format(formatStr,
        _(message->getGroup().name),
        reason->getRequiredTech() * 100.0f / MAX_TECH_LEVEL,
        reason->getCurrentTech() * 100.0f / MAX_TECH_LEVEL));
    }
    else if(SpaceOccupiedMessage::ptr reason =
      dynamic_message_cast<SpaceOccupiedMessage>(reason_)
    ) {
      dialog.messageAddText(_("This space is occupied."));
    }
    else if(OutsideMapMessage::ptr reason =
      dynamic_message_cast<OutsideMapMessage>(reason_)
    ) {
      dialog.messageAddText(_("Silly! You cannot build outside the map."));
    }
    else if(DesertHereMessage::ptr reason =
      dynamic_message_cast<DesertHereMessage>(reason_)
    ) {
      dialog.messageAddText(fmt::format(
        _("A {} needs water, but this space is desert."),
        _(message->getGroup().name)));
    }
    else if(NoOreMessage::ptr reason =
      dynamic_message_cast<NoOreMessage>(reason_)
    ) {
      dialog.messageAddText(_("There are no ore reserves left here."));
    }
    else if(auto reason = dynamic_message_cast<CityRequirementMessage>(reason_)) {
      dialog.messageAddText(reason->text);
    }
    else if(!reason_) {
// #ifdef DEBUG
      // clang doesn't like '*' operator in typeid
      const CannotBuildMessage& message_raw = *message;
      std::cerr << "warning: no reason given in CannotBuildMessage: "
        << typeid(message_raw).name() << ": "
        << message->str() << std::endl;
// #endif
      assert(false);
    }
    else {
// #ifdef DEBUG
      // clang doesn't like '*' operator in typeid
      const Message& reason_raw = *reason_;
      std::cerr << "warning: unrecognized reason in CannotBuildMessage: "
        << typeid(reason_raw).name() << ": "
        << reason_->str() << std::endl;
// #endif
      dialog
        .messageAddText(_("unrecognized reason"))
        .imageFile("images/gui/dialogs/error.png");
      assert(false);
    }
    dialog.build();
  }
  else if(CannotBulldozeThisMessage::ptr message =
    dynamic_message_cast<CannotBulldozeThisMessage>(message_)
  ) {
    DialogBuilder dialog;
    dialog
      .titleText(_("Cannot Bulldoze"))
      .messageAddTextBold(fmt::format(_("Cannot bulldoze this {}."),
        _(message->getGroup().name)))
      .imageFile("images/gui/dialogs/warning.png")
      .buttonSet(DialogBuilder::ButtonSet::OK);
    if(CannotBulldozeNonemptyTipMessage::ptr message =
      dynamic_message_cast<CannotBulldozeNonemptyTipMessage>(message_)
    ) {
      dialog.messageAddText(_("You cannot bulldoze this land fill because it is"
        " full of waste."));
    }
    else if(CannotBulldozeIncompleteMonumentMessage::ptr message =
      dynamic_message_cast<CannotBulldozeIncompleteMonumentMessage>(message_)
    ) {
      dialog.messageAddText(_("You cannot bulldoze this monument because it is"
        " still under construction."));
    }
    else if(CannotBulldozeThisEverMessage::ptr message =
      dynamic_message_cast<CannotBulldozeThisEverMessage>(message_)
    ) {
      dialog.messageAddText(fmt::format(
        _("You are not allowed to bulldoze {}."),
        _(message->getGroup().name_plural)));
    }
    else {{
// #ifdef DEBUG
      // clang doesn't like '*' operator in typeid
      const Message& message_raw = *message_;
      std::cerr << "warning: unrecognized message derived from "
        << "CannotBulldozeThisMessage: "
        << typeid(message_raw).name() << ": "
        << message_->str() << std::endl;
// #endif
      CannotBulldozeThisMessage::ptr message =
        dynamic_message_cast<CannotBulldozeThisMessage>(message_);
      dialog.messageAddText(fmt::format(_("You are not allowed to bulldoze this"
        " {}, but we're not exactly sure why."),
        _(message->getGroup().name)));
      dialog.imageFile("images/gui/dialogs/warning.png");
      assert(false);
    }}
    dialog.build();
  }
  else if(CannotEvacuateThisMessage::ptr message =
    dynamic_message_cast<CannotEvacuateThisMessage>(message_)
  ) {
    DialogBuilder()
      .titleText(_("Cannot Evacuate"))
      .imageFile("images/gui/dialogs/warning.png")
      .buttonSet(DialogBuilder::ButtonSet::OK)
      .messageAddTextBold(fmt::format(_("Cannot evacuate {}."),
        _(message->getGroup().name)))
      .messageAddText(fmt::format(_("You are not allowed to evacuate {}."),
        _(message->getGroup().name_plural)))
      .build();
  }
  else {
// #ifdef DEBUG
    // clang doesn't like '*' operator in typeid
    const Message& message_raw = *message_;
    std::cerr << "warning: encountered unrecognized message: "
      << typeid(message_raw).name() << ": "
      << message_->str() << std::endl;
// #endif
    // DialogBuilder()
    //   .titleText(_("Error!"))
    //   .messageAddTextBold(_("The requested action failed for an unrecognized "
    //     "reason."))
    //   .imageFile("images/gui/dialogs/error.png")
    //   .buttonSet(DialogBuilder::ButtonSet::OK)
    //   .build();
    assert(false);
  }
}

/** @file lincity-ng/Game.cpp */

// "1234567" -> "$1,234,567"
static std::string dollars(long long v) {
  std::string digits = std::to_string(v < 0 ? -v : v), out;
  for(size_t i = 0; i < digits.size(); ++i) {
    if(i && (digits.size() - i) % 3 == 0) out += ',';
    out += digits[i];
  }
  return std::string(v < 0 ? "-$" : "$") + out;
}

void Game::showPublicServices() {
  if(gui->findComponent("PublicServices")) return;
  auto panel = dynamic_unique_cast<Window>(loadGUIFile("gui/publicservices.xml"));
  auto* w = panel.get();
  auto* pages = dynamic_cast<SwitchComponent*>(w->findComponent("ServicePages"));

  // Every text and caption in the window is rebuilt here after each click.
  auto refresh = [this, w]() {
    auto& civic = world->civic;
    auto& rates = world->money_rates;
    const auto fiscal = civic.monthlyFiscal(*world);
    const long long balance = fiscal.revenue - fiscal.operating;
    getParagraph(*w, "ServiceSummary")->setText("Cash " + dollars(world->total_money) + (civic.hasMonthlyHistory ? "   |   Last month: revenue " : "   |   Monthly baseline: revenue ") + dollars(fiscal.revenue)
      + ", running costs " + dollars(fiscal.operating) + ", balance " + (balance >= 0 ? "+" : "") + dollars(balance));

    // Taxes page
    const long long estimatedTax = civic.laborTaxBase(*world) * rates.income_tax / 100;
    getParagraph(*w, "IncomeLine")->setText("Income tax: " + std::to_string(rates.income_tax) + "%" + "   (monthly estimate " + dollars(estimatedTax) + ")");
    getParagraph(*w, "GoodsLine")->setText("Goods tax: " + std::to_string(rates.goods_tax) + "%");
    getParagraph(*w, "SocialLine")->setText("Social support: " + std::to_string(rates.dole) + "   (help for the jobless)");
    getParagraph(*w, "CeilingLine")->setText("Tax ceiling for automation: " + std::to_string(civic.taxCeiling) + "%");
    std::string why;
    const int recommended = civic.recommendedTax(*world, &why);
    getParagraph(*w, "AdvisorText")->setText(civic.taxNote.empty()
      ? "Financial advisor: recommends " + std::to_string(recommended) + "% income tax (now " + std::to_string(rates.income_tax) + "%). " + why
      : "Financial advisor: " + civic.taxNote);
    getButton(*w, "AutoTax")->setCaptionText(civic.autoTax ? "Taxes are automatic (click to stop)" : "Set taxes automatically now");

    // Services page
    static const char* serviceNames[] = {"Education", "Health", "Fire", "Police"};
    static const char* lineNames[] = {"EducationLine", "HealthLine", "FireLine", "PoliceLine"};
    const auto& counts = world->stats.groupCount;
    const unsigned facilities[] = {counts[GROUP_SCHOOL] + counts[GROUP_UNIVERSITY],
      counts[GROUP_HEALTH], counts[GROUP_FIRESTATION], counts[GROUP_POLICE]};
    for(int i = 0; i < 4; ++i)
      getParagraph(*w, lineNames[i])->setText(std::string(serviceNames[i]) + " funding: "
        + std::to_string(civic.budget[i]) + "%   (" + (facilities[i] == 0
        ? "no buildings, $0" : std::to_string(facilities[i]) + (facilities[i] == 1 ? " facility" : " facilities")) + ")");
    getParagraph(*w, "ServiceHealth")->setText("Flu cases: " + std::to_string(world->flu.active) + "   |   Pollution illness: " + std::to_string(civic.sickTotal)
      + "   |   Education: " + std::to_string(civic.education / 10) + "%   |   Incident risk: " + std::to_string(civic.crime / 10)
      + "%   |   Crews: " + std::to_string(civic.units.size())
      + ". Below 100% reduces operating days. Above 100% increases capacity.");
    getButton(*w, "AutoBudget")->setCaptionText(civic.autoBudget ? "Auto funding: ON (click to stop)" : "Apply recommended funding and turn on auto funding");

    // Automation page
    const bool autonomous = world->evolution.fullAutonomy && world->evolution.mayorMode;
    getButton(*w, "FullAutonomy")->setCaptionText(autonomous ? "Full autonomy: ON (click to stop)" : "Full autonomy: OFF");
    getParagraph(*w, "ReserveLine")->setText("Cash reserve: " + dollars(civic.reserve) + "   (kept in the treasury)");
    getButton(*w, "FluToggle")->setCaptionText(world->flu.enabled ? "Flu transmission: ON" : "Flu transmission: OFF");
    getParagraph(*w, "AutomationHelp")->setText(
      "Full autonomy: the mayor builds and manages his area, and switches on automatic taxes and funding.   "
      "Auto-adjust taxes (Taxes tab): sets income tax to the advisor's rate at once, then follows it one point a month.   "
      "Auto funding applies now, then reviews monthly.   Changing a value by hand switches its automation off.");
    getParagraph(*w, "ServiceNote")->setText("Manual changes turn off that automation. Tax receipts arrive monthly. " + (civic.fundingNote.empty()
      ? std::string("Services need facilities, staff and goods.") : civic.fundingNote));
    getGameView().setMapDirty();
  };

  // bind(): run an action when a button is clicked, then redraw the window.
  auto bind = [w, refresh](const char* name, std::function<void()> action) {
    getButton(*w, name)->clicked.connect([action, refresh](Button*) { action(); refresh(); });
  };
  auto tabs = [&](const char* button, const char* page) {
    getButton(*w, button)->clicked.connect([pages, page](Button*) { if(pages) pages->switchComponent(page); });
  };
  tabs("TabTaxes", "TaxPage"); tabs("TabServices", "ServicesPage"); tabs("TabAutomation", "AutomationPage");

  // Taxes: a manual change always switches automatic taxes off.
  auto manualTax = [this](int& rate, int delta, int low, int high) {
    world->civic.autoTax = false; world->civic.taxNote.clear();
    rate = std::clamp(rate + delta, low, high);
  };
  bind("TaxDown",      [this, manualTax]() { manualTax(world->money_rates.income_tax, -1, 0, 30); });
  bind("TaxUp",        [this, manualTax]() { manualTax(world->money_rates.income_tax, +1, 0, 30); });
  bind("BusinessDown", [this, manualTax]() { manualTax(world->money_rates.goods_tax, -1, 0, 20); });
  bind("BusinessUp",   [this, manualTax]() { manualTax(world->money_rates.goods_tax, +1, 0, 20); });
  bind("SocialDown",   [this]() { world->money_rates.dole = std::max(0, world->money_rates.dole - 5); });
  bind("SocialUp",     [this]() { world->money_rates.dole = std::min(100, world->money_rates.dole + 5); });
  bind("CeilingDown",  [this]() { world->civic.taxCeiling = std::max(5, world->civic.taxCeiling - 1); });
  bind("CeilingUp",    [this]() { world->civic.taxCeiling = std::min(30, world->civic.taxCeiling + 1); });
  bind("AutoTax", [this]() {
    auto& civic = world->civic;
    if(civic.autoTax) { civic.autoTax = false; civic.taxNote = "Automatic taxes stopped. The rate stays where it is."; }
    else { civic.autoTax = true; civic.applyRecommendedTax(*world); } // takes effect now, not at the next monthly review
  });

  // Service funding: separate minus and plus buttons, 5% steps from 0 to 200%.
  static const char* downNames[] = {"EduDown", "HealthDown", "FireDown", "PoliceDown"};
  static const char* upNames[] = {"EduUp", "HealthUp", "FireUp", "PoliceUp"};
  for(int i = 0; i < 4; ++i) {
    bind(downNames[i], [this, i]() { world->civic.autoBudget = false; int& b = world->civic.budget[i]; b = std::max(0, b - 5); });
    bind(upNames[i],   [this, i]() { world->civic.autoBudget = false; int& b = world->civic.budget[i]; b = std::min(200, b + 5); });
  }
  bind("AutoBudget", [this]() {
    auto& civic = world->civic;
    civic.autoBudget = !civic.autoBudget;
    if(civic.autoBudget) civic.adjustFunding(*world, true);
  });

  // Automation
  bind("FullAutonomy", [this]() { auto& e = world->evolution; e.setFullAutonomy(*world, !(e.fullAutonomy && e.mayorMode)); });
  getButton(*w, "OpenMayorArea")->clicked.connect([this, w](Button*) { windowmanager->removeWindow(w); showCityHall(true); });
  bind("ReserveDown", [this]() { world->civic.reserve = std::max(25000, world->civic.reserve - 25000); });
  bind("ReserveUp",   [this]() { world->civic.reserve = std::min(1000000, world->civic.reserve + 25000); });
  bind("FluToggle",   [this]() { world->flu.enabled = !world->flu.enabled; if(!world->flu.enabled) world->flu.clear(); });

  getButton(*w, "ServicesClose")->clicked.connect([this, w](Button*) { windowmanager->removeWindow(w); });
  windowmanager->addWindow(std::move(panel));
  refresh();
}
