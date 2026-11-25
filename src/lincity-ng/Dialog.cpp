/* ---------------------------------------------------------------------- *
 * src/lincity-ng/Dialog.cpp
 * This file is part of Lincity-NG.
 *
 * Copyright (C) 2005      Wolfgang Becker <uafr@gmx.de>
 * Copyright (C) 2024-2025 David Bears <dbear4q@gmail.com>
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

#include "Dialog.hpp"
#include "gui/DialogBuilder.hpp"

#include <assert.h>                         // for assert
#include <config.h>                         // for PACKAGE_VERSION
#include <stdio.h>                          // for snprintf
#include <stdlib.h>                         // for free, malloc
#include <algorithm>
#include <deque>
#include <libxml++/parsers/textreader.h>
#include <array>                            // for array
#include <exception>                        // for exception
#include <filesystem>                       // for path, operator/
#include <fstream>                          // for basic_ostream, operator<<
#include <functional>                       // for bind, _1, function
#include <iostream>                         // for cerr
#include <sstream>                          // for basic_stringstream
#include <stdexcept>                        // for runtime_error
#include <utility>                          // for move
#include <vector>                           // for vector

#include "Config.hpp"                       // for getConfig, Config
#include "Game.hpp"                         // for Game
#include "GameView.hpp"                     // for GameView
#include "Mps.hpp"                          // for MpsMap
#include "Util.hpp"                         // for getCheckButton, getButton
#include "gui/Button.hpp"                   // for Button
#include "gui/CheckButton.hpp"              // for CheckButton
#include "gui/Component.hpp"                // for Component
#include "gui/ComponentFactory.hpp"
#include "gui/Painter.hpp"
#include "gui/ComponentLoader.hpp"          // for loadGUIFile
#include "gui/Paragraph.hpp"                // for Paragraph
#include "gui/Signal.hpp"                   // for Signal
#include "gui/SwitchComponent.hpp"          // for SwitchComponent
#include "gui/Window.hpp"                   // for Window
#include "gui/WindowManager.hpp"            // for WindowManager
#include "lincity/MapPoint.hpp"             // for MapPoint, operator<<
#include "lincity/commodities.hpp"          // for CommodityRule, Commodity
#include "lincity/groups.hpp"               // for GROUP_BLACKSMITH, GROUP_C...
#include "lincity/lin-city.hpp"             // for MAX_TECH_LEVEL, RESULTS_F...
#include "lincity/lintypes.hpp"             // for Construction, NUMOF_DAYS_...
#include "lincity/messages.hpp"             // for OutOfMoneyMessage
#include "lincity/modules/all_modules.hpp"  // for Market, Port, RocketPad
#include "lincity/stats.hpp"                // for Stats, Stat
#include "lincity/util.hpp"                 // for current_year, current_month
#include "lincity/world.hpp"                // for World, Map, MapTile
#include "util/gettextutil.hpp"             // for _
#include "util/ptrutil.hpp"                 // for dynamic_unique_cast

using namespace std::placeholders;

namespace {

struct TradeBar {
  long long importCents = 0, exportCents = 0;
  bool recorded = false;
};

const Color importColor(99, 177, 231);
const Color exportColor(246, 183, 107);

class PortTradeChart : public Component {
public:
  void parse(xmlpp::TextReader& reader) {
    while(reader.move_to_next_attribute())
      parseAttribute(reader);
    reader.move_to_element();
    setFlags(FLAG_RESIZABLE);
  }

  void resize(float w, float h) override {
    width = w;
    height = h;
  }

  void setSeries(std::vector<TradeBar> series, long long maxCents) {
    bars = std::move(series);
    ceiling = std::max(1LL, maxCents);
    setDirty();
  }

  void draw(Painter& painter) override {
    painter.setFillColor(Color(16, 31, 38));
    painter.fillRectangle(Rect2D(0, 0, width, height));
    const float baseline = height - 8;
    const float plotHeight = height - 16;
    painter.setLineColor(Color(51, 73, 82));
    for(int line = 0; line <= 4; ++line) {
      const float y = baseline - plotHeight * line / 4;
      painter.drawLine(Vector2(8, y), Vector2(width-8, y));
    }
    if(bars.empty()) return;
    const float slotWidth = (width-16) / bars.size();
    const float barWidth = std::min(42.f, slotWidth*.42f);
    auto drawBar = [&](float x, long long cents, Color color) {
      if(cents <= 0) return;
      const float h = std::max(1.f, float(plotHeight * double(cents) / ceiling));
      painter.setFillColor(color);
      painter.fillRectangle(Rect2D(x, baseline-h, x+barWidth, baseline));
    };
    for(size_t i = 0; i < bars.size(); ++i) {
      const float center = 8 + (i+.5f)*slotWidth;
      const auto& bar = bars[i];
      if(!bar.recorded) continue;
      painter.setFillColor(Color(84, 116, 125));
      painter.fillRectangle(Rect2D(center-barWidth, baseline, center+barWidth, baseline+2));
      drawBar(center-barWidth, bar.importCents, importColor);
      drawBar(center, bar.exportCents, exportColor);
    }
  }

private:
  std::vector<TradeBar> bars;
  long long ceiling = 1;
};

IMPLEMENT_COMPONENT_FACTORY(PortTradeChart)

std::string tradeMoney(long long cents) {
  const int remainder = cents % 100;
  return "$" + std::to_string(cents / 100) +
    (remainder ? (remainder < 10 ? ".0" : ".") + std::to_string(remainder) : "");
}

void showPortTradeHistory(WindowManager& manager, const Port& port, int today) {
  auto panel = dynamic_unique_cast<Window>(loadGUIFile("gui/porttradehistory.xml"));
  auto* window = panel.get();
  const auto records = std::make_shared<const std::deque<Port::TradeMonth>>(port.tradeHistory);
  const int currentMonth = (today + 1) / NUMOF_DAYS_IN_MONTH;
  getParagraph(*window, "TradeCurrent")->setText(
    "Month in progress  |  " + tradeMoney(port.monthly_ic * 100LL) +
    " imported  |  " + tradeMoney(port.monthly_et) + " exported");

  const auto showRange = [window, records, currentMonth](int months) {
    const int start = std::max(0, currentMonth - months);
    const int bucketMonths = months > 120 ? 12 : 1;
    std::vector<TradeBar> bars((currentMonth - start + bucketMonths - 1) / bucketMonths);
    std::array<long long, STUFF_COUNT> importUnits{}, exportUnits{};
    long long importCents = 0, exportCents = 0;
    int recorded = 0;
    for(const auto& entry : *records) {
      if(entry.month < start || entry.month >= currentMonth) continue;
      auto& bar = bars[(entry.month - start) / bucketMonths];
      bar.recorded = true;
      bar.importCents += entry.imports * 100LL;
      bar.exportCents += entry.exportCents;
      importCents += entry.imports * 100LL;
      exportCents += entry.exportCents;
      ++recorded;
      for(Commodity item = STUFF_INIT; item < STUFF_COUNT; ++item) {
        importUnits[item] += entry.imported[item];
        exportUnits[item] += entry.exported[item];
      }
    }
    long long ceiling = 0;
    for(const auto& bar : bars)
      ceiling = std::max({ceiling, bar.importCents, bar.exportCents});
    auto* chart = dynamic_cast<PortTradeChart*>(window->findComponent("TradeChart"));
    chart->setSeries(std::move(bars), ceiling);

    auto monthLabel = [](int month) {
      const int day = month * NUMOF_DAYS_IN_MONTH;
      return std::string(current_month(day)) + " " + std::to_string(current_year(day));
    };
    getParagraph(*window, "TradePeriod")->setText(start == currentMonth ? "No completed months yet" :
      monthLabel(start) + " to " + monthLabel(currentMonth-1) + "  |  " +
      std::to_string(recorded) + " of " + std::to_string(currentMonth-start) + " months recorded");
    getParagraph(*window, "TradeScale")->setText(recorded ?
      "Chart peak " + tradeMoney(ceiling) + (bucketMonths == 1 ? " / month" : " / 12 months") :
      "No trade history for this period");
    getParagraph(*window, "TradeImports")->setText("Imports  " + tradeMoney(importCents));
    getParagraph(*window, "TradeExports")->setText("Exports  " + tradeMoney(exportCents));

    const Commodity items[] = {STUFF_FOOD, STUFF_COAL, STUFF_GOODS, STUFF_ORE, STUFF_STEEL, STUFF_HIVOLT};
    for(size_t i = 0; i < std::size(items); ++i) {
      const auto item = items[i];
      const auto row = std::to_string(i);
      getParagraph(*window, "TradeItem" + row)->setText(
        item == STUFF_HIVOLT ? "Grid power" : commodityName(item));
      getParagraph(*window, "TradeIn" + row)->setText(std::to_string(importUnits[item]));
      getParagraph(*window, "TradeOut" + row)->setText(std::to_string(exportUnits[item]));
    }
  };
  for(int months : {1, 12, 60, 120, 600, 1200})
    getButton(*window, "TradeRange" + std::to_string(months))->clicked.connect(
      [showRange, months](Button*) { showRange(months); });
  manager.addWindow(std::move(panel));
  showRange(1);
}

} // namespace


bool blockingDialogIsOpen = false;

std::vector<Dialog*> dialogVector;

void closeAllDialogs(){
    std::vector<Dialog*>::iterator iter;
    while( !dialogVector.empty() ){
        iter = dialogVector.begin();
        (*iter)->closeDialog();
    }
}

Dialog::Dialog(Game& game, int type)
  : game(game), point(MapPoint(-1,-1))
{
    initDialog();
    switch( type ){
        case ASK_COAL_SURVEY:
            coalSurvey();
            break;
        case GAME_STATS:
            gameStats();
            break;
        default:
            std::stringstream msg;
            msg <<"Can't open Dialog type " << type << " without additional parameters.";
            throw std::runtime_error(msg.str());
    }
}

Dialog::Dialog(Game& game, int type, MapPoint point)
  : game(game), point(point)
{
    initDialog();
    switch( type ) {
        case EDIT_MARKET:
            editMarket();
            break;
        case EDIT_PORT:
            editPort();
            break;
        case ASK_LAUNCH_ROCKET:
            askRocket();
            break;
        default:
            std::stringstream msg;
            msg <<"Can't open Dialog type " << type << " with coordinates.";
            throw std::runtime_error(msg.str());
    }
}

void Dialog::initDialog() {
  windowManager = nullptr;
  myDialogComponent = nullptr;
  iAmBlocking = false;
  windowManager = dynamic_cast<WindowManager*>(
    game.getGui().findComponent("windowManager"));
  assert(windowManager);
}

Dialog::~Dialog(){
}

void Dialog::registerDialog(std::unique_ptr<Window>&& window) {
  myDialogComponent = window.get();
  dialogVector.push_back(this);
  windowManager->addWindow(std::move(window));
}

void Dialog::unRegisterDialog(){
    std::vector<Dialog*>::iterator iter = dialogVector.begin();
    while( iter != dialogVector.end() ){
        if ( *iter == this ){
            iter = dialogVector.erase( iter );
        } else {
            iter++;
        }
    }

    delete( this );
}

void Dialog::askRocket(){
    if( !windowManager ) {
        std::cerr << "No window manager found.\n";
        return;
    }
    try {
        registerDialog(dynamic_unique_cast<Window>(
          loadGUIFile("gui/dialogs/launch_rocket_yn.xml")));
        blockingDialogIsOpen = true;
        iAmBlocking = true;
    } catch(std::exception& e) {
        std::cerr << "Couldn't display message 'launch_rocket_yn': "
            << e.what() << "\n";
        return;
    }
    Paragraph* p = getParagraph( *myDialogComponent, "DialogTitle" );
    std::stringstream title;
    title << _("Launchsite") << point;
    p->setText( title.str() );
    // connect signals
    Button* yesButton = getButton( *myDialogComponent, "Yes" );
    yesButton->clicked.connect(
      std::bind(&Dialog::okayLaunchRocketButtonClicked, this, _1));
    Button* noButton = getButton( *myDialogComponent, "No" );
    noButton->clicked.connect(
      std::bind(&Dialog::closeDialogButtonClicked, this, _1));
    Button* gotoButton = getButton( *myDialogComponent, "goto" );
    gotoButton->clicked.connect(
      std::bind(&Dialog::gotoButtonClicked, this, _1));
}

void Dialog::coalSurvey(){
    if( !windowManager ) {
        std::cerr << "No window manager found.\n";
        return;
    }
    try {
        registerDialog(dynamic_unique_cast<Window>(
          loadGUIFile("gui/dialogs/coal_survey_yn.xml")));
        blockingDialogIsOpen = true;
        iAmBlocking = true;
    } catch(std::exception& e) {
        std::cerr << "Couldn't display message 'coal_survey_yn': "
            << e.what() << "\n";
        return;
    }
    // connect signals
    Button* yesButton = getButton( *myDialogComponent, "Yes" );
    yesButton->clicked.connect(
      std::bind(&Dialog::okayCoalSurveyButtonClicked, this, _1));
    Button* noButton = getButton( *myDialogComponent, "No" );
    noButton->clicked.connect(
      std::bind(&Dialog::closeDialogButtonClicked, this, _1));
}

void Dialog::setParagraphN( const std::string basename, const int number, const std::string text ){
    std::stringstream paragraphName;
    paragraphName << basename << number;
    setParagraph( paragraphName.str(), text );
}

template<typename T> void Dialog::setTableRC( const std::string basename, const int row, const int column, const std::string text, T value ){
    std::stringstream paragraphName;
    paragraphName << basename << "_text_" << row << "_" << column;
    setParagraph( paragraphName.str(), text );
    paragraphName.str("");
    paragraphName << basename << "_value_" << row << "_" << column;
    std::stringstream stringvalue;
    stringvalue << value;
    setParagraph( paragraphName.str(), stringvalue.str() );
}

void Dialog::setParagraph( const std::string paragraphName, const std::string text ){
    Paragraph* p;
    try{
        p = getParagraph( *myDialogComponent, paragraphName );
        p->setText( text );
    } catch(std::exception& e) {
        std::cerr << "Couldn't set " << paragraphName << " to '" << text << "': "
            << e.what() << "\n";
    }
}

/*
 * Show game statistics in dialog and write them to RESULTS_FILENAME.
 *
 * gamestats.xml has 11 lines
 * statistic_text1
 * [...]
 * statistic_text11
 * and 15 lines in a Table with 6 columns
 * statistic_text_1_1  statistic_number_1_1  statistic_text_1_2  statistic_number_1_2  statistic_text_1_3  statistic_number_1_3
 * [...]
//  * statistic_text_15_1 statistic_number_15_1 statistic_text_15_2 statistic_number_15_2 statistic_text_15_3 statistic_number_15_3
 */
void Dialog::gameStats(){
     saveGameStats();
     if( !windowManager ) {
        std::cerr << "No window manager found.\n";
        return;
    }
    bool useExisting = false;
    myDialogComponent = dynamic_cast<Window *>(
      windowManager->findComponent("GameStats"));
    if( myDialogComponent == 0){
        try {
            registerDialog(dynamic_unique_cast<Window>(
              loadGUIFile("gui/dialogs/gamestats.xml")));
        } catch(std::exception& e) {
            std::cerr << "Couldn't display message 'gamestats': "
                << e.what() << "\n";
            return;
        }
    } else {
        useExisting = true;
    }

    // Fill in Fields.
    World& world = game.getWorld();
    int line = 1;
    int maxlength = 567;
    char* outf = (char *) malloc ( maxlength );
    snprintf (outf, maxlength,"%s %s", _("Game statistics from LinCity-NG Version"), PACKAGE_VERSION);
    setParagraphN( "statistic_text", line++, outf );
    if(world.given_scene.length() > 3) {
        snprintf (outf, maxlength,"%s - %s", _("Initial loaded scene"), world.given_scene.c_str());
        setParagraphN( "statistic_text", line++, outf );
    }
    if(world.stats.sustainability.sustainable) {
        snprintf (outf, maxlength, "%s", _("Economy is sustainable"));
        setParagraphN( "statistic_text", line++, outf );
    }
    snprintf (outf, maxlength, "%s %d %s %d %s.",
        _("Population"), world.stats.population.population_m / NUMOF_DAYS_IN_MONTH,
        _("of which"),  world.people_pool, _("are not housed"));
    setParagraphN( "statistic_text", line++, outf );
    snprintf(outf, maxlength, "%s %d %s %d %s %d",
      _("Max population"), world.stats.population.max_pop_ever,
      _("Number evacuated"), world.stats.population.evacuated_t,
      _("Total births"), world.stats.population.births_t);
    setParagraphN( "statistic_text", line++, outf );
    snprintf (outf, maxlength, "%s %s %04d %s %8d %s %5.1f (%5.1f)",
        _("Date"),  current_month(world.total_time), current_year(world.total_time),
        _("Money"), world.total_money,
        _("Tech-level"), (float) world.tech_level * 100.0 / MAX_TECH_LEVEL,
        (float) world.stats.highest_tech_level * 100.0 / MAX_TECH_LEVEL);
    setParagraphN( "statistic_text", line++, outf );
    snprintf (outf, maxlength, "%s %7d %s %8.3f",
        _("Deaths by starvation"), world.stats.population.starve_deaths_t,
        _("History"), world.stats.population.starve_deaths_history);
    setParagraphN( "statistic_text", line++, outf );
    snprintf (outf, maxlength, "%s %7d   %s %8.3f",
        _("Deaths from pollution"), world.stats.population.pollution_deaths_t,
        _("History"), world.stats.population.pollution_deaths_history);
    setParagraphN( "statistic_text", line++, outf );
    snprintf (outf, maxlength, "%s %7d   %s %8.3f",
        _("Years of unemployment"), world.stats.population.unemployed_days_t / NUMOF_DAYS_IN_YEAR,
        _("History"), world.stats.population.unemployed_history);
    setParagraphN( "statistic_text", line++, outf );
    snprintf (outf, maxlength, "%s %2d  %s %2d",
        _("Rockets launched"), world.rockets_launched,
        _("Successful launches"), world.rockets_launched_success);
    setParagraphN( "statistic_text", line++, outf );

    while( line <= 11 ){ //clear remaining lines
        setParagraphN( "statistic_text", line++, "" );
    }
    free( outf );

    setTableRC("statistic", 1, 1, _("Residences"),
      world.stats.groupCount[GROUP_RESIDENCE_LL] +
      world.stats.groupCount[GROUP_RESIDENCE_ML] +
      world.stats.groupCount[GROUP_RESIDENCE_HL] +
      world.stats.groupCount[GROUP_RESIDENCE_LH] +
      world.stats.groupCount[GROUP_RESIDENCE_MH] +
      world.stats.groupCount[GROUP_RESIDENCE_HH]);
    setTableRC("statistic", 1, 2, _("Markets"), world.stats.groupCount[GROUP_MARKET]);
    setTableRC("statistic", 1, 3, _("Farms"), world.stats.groupCount[GROUP_ORGANIC_FARM]);

    setTableRC("statistic", 2, 1, _("Water wells"), world.stats.groupCount[GROUP_WATERWELL]);
    setTableRC("statistic", 2, 2, _("Windmills"), world.stats.groupCount[GROUP_WINDMILL]);
    setTableRC("statistic", 2, 3, "", "");

    setTableRC("statistic", 3, 1, "", "");
    setTableRC("statistic", 3, 2, "", "");
    setTableRC("statistic", 3, 3, "", "");

    setTableRC("statistic", 4, 1, _("Monuments"), world.stats.groupCount[GROUP_MONUMENT]);
    setTableRC("statistic", 4, 2, _("Schools"), world.stats.groupCount[GROUP_SCHOOL]);
    setTableRC("statistic", 4, 3, _("Universities"), world.stats.groupCount[GROUP_UNIVERSITY]);

    setTableRC("statistic", 5, 1, _("Fire stations"), world.stats.groupCount[GROUP_FIRESTATION]);
    setTableRC("statistic", 5, 2, _("Parks"),
      world.stats.groupCount[GROUP_PARKLAND] +
      world.stats.groupCount[GROUP_PARKPOND]);
    setTableRC("statistic", 5, 3, _("Sports fields"), world.stats.groupCount[GROUP_CRICKET]);

    setTableRC("statistic", 6, 1, _("Health centres"), world.stats.groupCount[GROUP_HEALTH]);
    setTableRC("statistic", 6, 2, _("Tips"), world.stats.groupCount[GROUP_TIP]);
    setTableRC("statistic", 6, 3, _("Shanties"), world.stats.groupCount[GROUP_SHANTY]);

    setTableRC("statistic", 7, 1, "", "");
    setTableRC("statistic", 7, 2, "", "");
    setTableRC("statistic", 7, 3, "", "");

    setTableRC("statistic", 8, 1, _("Wind powers"), world.stats.groupCount[GROUP_WIND_POWER]);
    setTableRC("statistic", 8, 2, _("Coal powers"), world.stats.groupCount[GROUP_COAL_POWER]);
    setTableRC("statistic", 8, 3, _("Solar powers"), world.stats.groupCount[GROUP_SOLAR_POWER]);

    setTableRC("statistic", 9, 1, _("Substations"), world.stats.groupCount[GROUP_SUBSTATION]);
    setTableRC("statistic", 9, 2, _("Power lines"), world.stats.groupCount[GROUP_POWER_LINE]);
    setTableRC("statistic", 9, 3, _("Ports"), world.stats.groupCount[GROUP_PORT]);

    setTableRC("statistic", 10, 1, _("Tracks"),
      world.stats.groupCount[GROUP_TRACK] +
      world.stats.groupCount[GROUP_TRACK_BRIDGE]);
    setTableRC("statistic", 10, 2, _("Roads"),
      world.stats.groupCount[GROUP_ROAD] +
      world.stats.groupCount[GROUP_ROAD_BRIDGE]);
    setTableRC("statistic", 10, 3, _("Rail"),
      world.stats.groupCount[GROUP_RAIL] +
      world.stats.groupCount[GROUP_RAIL_BRIDGE]);

    setTableRC("statistic", 11, 1, "", "");
    setTableRC("statistic", 11, 2, "", "");
    setTableRC("statistic", 11, 3, "", "");

    setTableRC("statistic", 12, 1, _("Potteries"), world.stats.groupCount[GROUP_POTTERY]);
    setTableRC("statistic", 12, 2, _("Blacksmiths"), world.stats.groupCount[GROUP_BLACKSMITH]);
    setTableRC("statistic", 12, 3, _("Mills"), world.stats.groupCount[GROUP_MILL]);

    setTableRC("statistic", 13, 1, _("Light inds"), world.stats.groupCount[GROUP_INDUSTRY_L]);
    setTableRC("statistic", 13, 2, _("Heavy inds"), world.stats.groupCount[GROUP_INDUSTRY_H]);
    setTableRC("statistic", 13, 3, _("Recyclers"), world.stats.groupCount[GROUP_RECYCLE]);

    setTableRC("statistic", 14, 1, _("Coal mines"), world.stats.groupCount[GROUP_COALMINE]);
    setTableRC("statistic", 14, 2, _("Ore mines"), world.stats.groupCount[GROUP_OREMINE]);
    setTableRC("statistic", 14, 3, _("Forests"), world.stats.groupCount[GROUP_COMMUNE]);

    setTableRC("statistic", 15, 1, "", "");
    setTableRC("statistic", 15, 2, "", "");
    setTableRC("statistic", 15, 3, "", "");

    if( !useExisting ){
        // connect signals
        Button* noButton = getButton( *myDialogComponent, "Okay" );
        noButton->clicked.connect(
          std::bind(&Dialog::closeDialogButtonClicked, this, _1));
    }
}

/*
 * Save game statistics to RESULTS_FILENAME. Do NOT translate text here!
 * That way the files can be used for highscores.
 */
void Dialog::saveGameStats(){
    std::ofstream results(getConfig()->userDataDir.get() / RESULTS_FILENAME);

    // Fill in Fields.
    World& world = game.getWorld();
    int maxlength = 567;
    char* outf = (char *) malloc ( maxlength );
    //int group_count[NUM_OF_GROUPS];
    //count_all_groups (group_count);
    snprintf (outf, maxlength, "Game statistics from LinCity-NG Version %s", PACKAGE_VERSION);
    results << outf << std::endl;
    if (world.given_scene.length() > 3){
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wformat-truncation"
        snprintf (outf, maxlength, "Initial loaded scene - %s", world.given_scene.c_str());
        #pragma GCC diagnostic pop
        results << outf << std::endl;
    }
    if(world.stats.sustainability.sustainable){
        snprintf (outf, maxlength, "%s", "Economy is sustainable");
        results << outf << std::endl;
    }
    snprintf (outf, maxlength, "Population  %d  of which  %d  are not housed.",
      world.stats.population.population_m / NUMOF_DAYS_IN_MONTH,
      world.people_pool);
    results << outf << std::endl;
    snprintf (outf, maxlength,
         "Max population %d  Number evacuated %d Total births %d"
         , world.stats.population.max_pop_ever, world.stats.population.evacuated_t, world.stats.population.births_t);
    results << outf << std::endl;
    snprintf (outf, maxlength,
         "Date %02d/%04d  Money %8d   Tech-level %5.1f (%5.1f)",
         1 + ((world.total_time % NUMOF_DAYS_IN_YEAR) / NUMOF_DAYS_IN_MONTH), current_year(world.total_time), world.total_money,
         (float) world.tech_level * 100.0 / MAX_TECH_LEVEL,
         (float) world.stats.highest_tech_level * 100.0 / MAX_TECH_LEVEL);
    results << outf << std::endl;
    snprintf (outf, maxlength,
         " Deaths by starvation %7d   History %8.3f",
         world.stats.population.starve_deaths_t, world.stats.population.starve_deaths_history);
    results << outf << std::endl;
    snprintf (outf, maxlength,
         "Deaths from pollution %7d   History %8.3f",
         world.stats.population.pollution_deaths_t, world.stats.population.pollution_deaths_history);
    results << outf << std::endl;
    snprintf (outf, maxlength, "Years of unemployment %7d   History %8.3f",
         world.stats.population.unemployed_days_t / NUMOF_DAYS_IN_YEAR, world.stats.population.unemployed_history);
    results << outf << std::endl;
    snprintf (outf, maxlength, "Rockets launched %2d  Successful launches %2d",
         world.rockets_launched, world.rockets_launched_success);
    results << outf << std::endl;
    results << "" << std::endl;

    snprintf(outf, maxlength, "    Residences %4d         Markets %4d            Farms %4d",
      world.stats.groupCount[GROUP_RESIDENCE_LL]
        + world.stats.groupCount[GROUP_RESIDENCE_ML]
        + world.stats.groupCount[GROUP_RESIDENCE_HL]
        + world.stats.groupCount[GROUP_RESIDENCE_LH]
        + world.stats.groupCount[GROUP_RESIDENCE_MH]
        + world.stats.groupCount[GROUP_RESIDENCE_HH],
      world.stats.groupCount[GROUP_MARKET],
      world.stats.groupCount[GROUP_ORGANIC_FARM]);
    results << outf << std::endl;
    snprintf(outf, maxlength, "   Water wells %4d     Wind powers %4d",
      world.stats.groupCount[GROUP_WATERWELL],
      world.stats.groupCount[GROUP_WIND_POWER]);
    results << outf << std::endl;
    snprintf(outf, maxlength, "     Monuments %4d         Schools %4d     Universities %4d",
      world.stats.groupCount[GROUP_MONUMENT],
      world.stats.groupCount[GROUP_SCHOOL],
      world.stats.groupCount[GROUP_UNIVERSITY]);
    results << outf << std::endl;
    snprintf(outf, maxlength, " Fire stations %4d           Parks %4d    Sports fields %4d",
      world.stats.groupCount[GROUP_FIRESTATION],
      world.stats.groupCount[GROUP_PARKLAND] + world.stats.groupCount[GROUP_PARKPOND],
      world.stats.groupCount[GROUP_CRICKET]);
    results << outf << std::endl;
    snprintf(outf, maxlength, "Health centres %4d            Tips %4d         Shanties %4d",
      world.stats.groupCount[GROUP_HEALTH],
      world.stats.groupCount[GROUP_TIP],
      world.stats.groupCount[GROUP_SHANTY]);
    results << outf << std::endl;
    results << "" << std::endl;

    snprintf(outf, maxlength, "     Windmills %4d     Coal powers %4d     Solar powers %4d",
      world.stats.groupCount[GROUP_WINDMILL],
      world.stats.groupCount[GROUP_COAL_POWER],
      world.stats.groupCount[GROUP_SOLAR_POWER]);
    results << outf << std::endl;
    snprintf(outf, maxlength, "   Substations %4d     Power lines %4d            Ports %4d",
      world.stats.groupCount[GROUP_SUBSTATION],
      world.stats.groupCount[GROUP_POWER_LINE],
      world.stats.groupCount[GROUP_PORT]);
    results << outf << std::endl;
    snprintf(outf, maxlength, "        Tracks %4d           Roads %4d             Rail %4d",
      world.stats.groupCount[GROUP_TRACK] + world.stats.groupCount[GROUP_TRACK_BRIDGE],
      world.stats.groupCount[GROUP_ROAD] + world.stats.groupCount[GROUP_ROAD_BRIDGE],
      world.stats.groupCount[GROUP_RAIL] + world.stats.groupCount[GROUP_RAIL_BRIDGE]);
    results << outf << std::endl;
    results << "" << std::endl;

    snprintf(outf, maxlength, "     Potteries %4d     Blacksmiths %4d            Mills %4d",
      world.stats.groupCount[GROUP_POTTERY],
      world.stats.groupCount[GROUP_BLACKSMITH],
      world.stats.groupCount[GROUP_MILL]);
    results << outf << std::endl;
    snprintf(outf, maxlength, "    Light inds %4d      Heavy inds %4d        Recyclers %4d",
      world.stats.groupCount[GROUP_INDUSTRY_L],
      world.stats.groupCount[GROUP_INDUSTRY_H],
      world.stats.groupCount[GROUP_RECYCLE]);
    results << outf << std::endl;
    snprintf(outf, maxlength, "    Coal mines %4d       Ore mines %4d         Communes %4d",
      world.stats.groupCount[GROUP_COALMINE],
      world.stats.groupCount[GROUP_OREMINE],
      world.stats.groupCount[GROUP_COMMUNE]);
    results << outf << std::endl;
    results << "" << std::endl;

    free( outf );
}

void Dialog::editMarket(){
    if( !windowManager ) {
        std::cerr << "No window manager found.\n";
        return;
    }
    try {
        registerDialog(dynamic_unique_cast<Window>(
          loadGUIFile("gui/dialogs/tradedialog.xml")));
        blockingDialogIsOpen = true;
        iAmBlocking = true;
    } catch(std::exception& e) {
        std::cerr << "Couldn't display dialog 'tradedialog.xml': "
            << e.what() << "\n";
        return;
    }
    // set Dialog to Market-Data
    Paragraph* p = getParagraph( *myDialogComponent, "DialogTitle" );
    std::stringstream title;
    title << _("Market") << point;
    p->setText( title.str() );
    Market *market = dynamic_cast<Market *>(
      game.getWorld().map(point)->reportingConstruction);
    assert(market);
    CheckButton* cb;
    cb = getCheckButton( *myDialogComponent, "BuyLabor" );
    if( market->commodityRuleCount[STUFF_LABOR].take ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "SellLabor" );
    if( market->commodityRuleCount[STUFF_LABOR].give ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "BuyFood" );
    if( market->commodityRuleCount[STUFF_FOOD].take ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "SellFood" );
    if( market->commodityRuleCount[STUFF_FOOD].give ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "BuyCoal" );
    if( market->commodityRuleCount[STUFF_COAL].take ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "SellCoal" );
    if( market->commodityRuleCount[STUFF_COAL].give ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "BuyOre" );
    if( market->commodityRuleCount[STUFF_ORE].take ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "SellOre" );
    if( market->commodityRuleCount[STUFF_ORE].give ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "BuyGoods" );
    if( market->commodityRuleCount[STUFF_GOODS].take ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "SellGoods" );
    if( market->commodityRuleCount[STUFF_GOODS].give ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "BuySteel" );
    if( market->commodityRuleCount[STUFF_STEEL].take ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "SellSteel" );
    if( market->commodityRuleCount[STUFF_STEEL].give ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "BuyWaste" );
    if( market->commodityRuleCount[STUFF_WASTE].take ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "SellWaste" );
    if( market->commodityRuleCount[STUFF_WASTE].give ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "BuyWater" );
    if( market->commodityRuleCount[STUFF_WATER].take ) cb->check(); else cb->uncheck();
    cb = getCheckButton( *myDialogComponent, "SellWater" );
    if( market->commodityRuleCount[STUFF_WATER].give ) cb->check(); else cb->uncheck();
    // connect signals
    Button* applyButton = getButton( *myDialogComponent, "Apply" );
    applyButton->clicked.connect(
      std::bind(&Dialog::applyMarketButtonClicked, this, _1));
    Button* gotoButton = getButton( *myDialogComponent, "goto" );
    gotoButton->clicked.connect(
      std::bind(&Dialog::gotoButtonClicked, this, _1));
}

void Dialog::editPort(){
    if( !windowManager ) {
        std::cerr << "No window manager found.\n";
        return;
    }
    try {
        registerDialog(dynamic_unique_cast<Window>(
          loadGUIFile("gui/dialogs/portdialog.xml")));
        blockingDialogIsOpen = true;
        iAmBlocking = true;
    } catch(std::exception& e) {
        std::cerr << "Couldn't display dialog 'portdialog.xml': "
            << e.what() << "\n";
        return;
    }
    // set Dialog to Port-Data
    //int port_flags = world(point)->reportingConstruction->flags;
    Port *port = dynamic_cast<Port *>(
      game.getWorld().map(point)->reportingConstruction);
    assert(port);
    Paragraph* p = getParagraph( *myDialogComponent, "DialogTitle" );
    std::stringstream title;
    title << _("Foreign Trade — Port ") << point;
    p->setText( title.str() );

    for(Commodity c = STUFF_INIT; c != STUFF_COUNT; c++) {
      if(!port->commodityRuleCount[c].maxload) continue;

      std::array<CheckButton*,2> direction{};
      for(int isGive = 0; isGive < 2; isGive++) {
        SwitchComponent *sc = dynamic_cast<SwitchComponent *>(
          myDialogComponent->findComponent(
            std::string(isGive?"Sell":"Buy")+commodityNames[c]));
        if(!sc) {
          std::cerr << "error: could not get "
            << (std::string(isGive?"Sell":"Buy")+commodityNames[c])
            << " SwitchComponent" << std::endl;
          continue;
        }
        const CommodityRule &rule = game.getWorld().tradeRule[c];
        sc->switchComponent((isGive ? rule.give : rule.take) ? "green" : "red");
        CheckButton *cb = dynamic_cast<CheckButton *>(sc->getActiveComponent());
        if(!cb) {
          std::cerr << "error: could not get "
            << ((isGive ? rule.give : rule.take) ? "green" : "red")
            << " CheckButton for "
            << (std::string(isGive?"Sell":"Buy")+commodityNames[c])
            << std::endl;
          continue;
        }

        direction[isGive] = cb;
        CommodityRule& crc = port->commodityRuleCount[c];
        if(isGive ? crc.give : crc.take) cb->check(); else cb->uncheck();
      }
      if(direction[0] && direction[1]) {
        direction[0]->checked.connect([other=direction[1]](CheckButton*) { other->uncheck(); });
        direction[1]->checked.connect([other=direction[0]](CheckButton*) { other->uncheck(); });
      }
    }

    // connect signals
    Button* applyButton = getButton( *myDialogComponent, "Apply" );
    applyButton->clicked.connect(
      std::bind(&Dialog::applyPortButtonClicked, this, _1));
    getButton(*myDialogComponent, "TradeHistory")->clicked.connect([this, port](Button*) {
      showPortTradeHistory(*windowManager, *port, game.getWorld().total_time);
    });
    Button* gotoButton = getButton( *myDialogComponent, "goto" );
    gotoButton->clicked.connect(
      std::bind(&Dialog::gotoButtonClicked, this, _1));
}

void Dialog::applyMarketButtonClicked( Button* ){
    CheckButton* cb;
    Market * market = dynamic_cast<Market *>(
      game.getWorld().map(point)->construction);
    cb = getCheckButton( *myDialogComponent, "BuyLabor" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_LABOR].take = true;
    } else {
        market->commodityRuleCount[STUFF_LABOR].take = false;
    }
    cb = getCheckButton( *myDialogComponent, "SellLabor" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_LABOR].give = true;
    } else {
        market->commodityRuleCount[STUFF_LABOR].give = false;
    }
    cb = getCheckButton( *myDialogComponent, "BuyFood" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_FOOD].take = true;
    } else {
        market->commodityRuleCount[STUFF_FOOD].take = false;
    }
    cb = getCheckButton( *myDialogComponent, "SellFood" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_FOOD].give = true;
    } else {
        market->commodityRuleCount[STUFF_FOOD].give = false;
    }
    cb = getCheckButton( *myDialogComponent, "BuyCoal" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_COAL].take = true;
    } else {
        market->commodityRuleCount[STUFF_COAL].take = false;
    }
    cb = getCheckButton( *myDialogComponent, "SellCoal" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_COAL].give = true;
    } else {
        market->commodityRuleCount[STUFF_COAL].give = false;
    }
    cb = getCheckButton( *myDialogComponent, "BuyOre" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_ORE].take = true;
    } else {
        market->commodityRuleCount[STUFF_ORE].take = false;
    }
    cb = getCheckButton( *myDialogComponent, "SellOre" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_ORE].give = true;
    } else {
        market->commodityRuleCount[STUFF_ORE].give = false;
    }
    cb = getCheckButton( *myDialogComponent, "BuyGoods" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_GOODS].take = true;
    } else {
        market->commodityRuleCount[STUFF_GOODS].take = false;
    }
    cb = getCheckButton( *myDialogComponent, "SellGoods" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_GOODS].give = true;
    } else {
        market->commodityRuleCount[STUFF_GOODS].give = false;
    }
    cb = getCheckButton( *myDialogComponent, "BuySteel" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_STEEL].take = true;
    } else {
        market->commodityRuleCount[STUFF_STEEL].take = false;
    }
    cb = getCheckButton( *myDialogComponent, "SellSteel" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_STEEL].give = true;
    } else {
        market->commodityRuleCount[STUFF_STEEL].give = false;
    }
    cb = getCheckButton( *myDialogComponent, "BuyWaste" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_WASTE].take = true;
    } else {
        market->commodityRuleCount[STUFF_WASTE].take = false;
    }
    cb = getCheckButton( *myDialogComponent, "SellWaste" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_WASTE].give = true;
    } else {
        market->commodityRuleCount[STUFF_WASTE].give = false;
    }
    cb = getCheckButton( *myDialogComponent, "BuyWater" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_WATER].take = true;
    } else {
        market->commodityRuleCount[STUFF_WATER].take = false;
    }
    cb = getCheckButton( *myDialogComponent, "SellWater" );
    if(cb->isChecked()) {
        market->commodityRuleCount[STUFF_WATER].give = true;
    } else {
        market->commodityRuleCount[STUFF_WATER].give = false;
    }
    game.getMpsMap().refresh();
    windowManager->removeWindow( myDialogComponent );
    blockingDialogIsOpen = false;
    unRegisterDialog();
}

void Dialog::applyPortButtonClicked( Button* ){
    Port *port = dynamic_cast<Port *>(
      game.getWorld().map(point)->reportingConstruction);
    assert(port);

    for(Commodity c = STUFF_INIT; c != STUFF_COUNT; c++) {
      if(!port->commodityRuleCount[c].maxload) continue;

      for(int isGive = 0; isGive < 2; isGive++) {
        SwitchComponent *sc = dynamic_cast<SwitchComponent *>(
          myDialogComponent->findComponent(
            std::string(isGive?"Sell":"Buy")+commodityNames[c]));
        if(!sc) {
          std::cerr << "error: could not find "
            << (std::string(isGive?"Sell":"Buy")+commodityNames[c])
            << " SwitchComponent" << std::endl;
          continue;
        }
        CheckButton *cb = dynamic_cast<CheckButton *>(sc->getActiveComponent());
        if(!cb) {
          std::cerr << "error: could not find CheckButton for "
            << (std::string(isGive?"Sell":"Buy")+commodityNames[c])
            << std::endl;
          continue;
        }

        CommodityRule& crc = port->commodityRuleCount[c];
        (isGive ? crc.give : crc.take) = cb->isChecked();
      }

      if(port->commodityRuleCount[c].take && port->commodityRuleCount[c].give) {
        port->commodityRuleCount[c].take = false;
        port->commodityRuleCount[c].give = false;
      }
    }

    windowManager->removeWindow( myDialogComponent );
    blockingDialogIsOpen = false;
    unRegisterDialog();
}

void Dialog::okayLaunchRocketButtonClicked(Button *) {
  dynamic_cast<RocketPad*>(
    game.getWorld().map(point)->reportingConstruction
  )->launch_rocket();
  windowManager->removeWindow(myDialogComponent);
  blockingDialogIsOpen = false;
  unRegisterDialog();
}


void Dialog::okayCoalSurveyButtonClicked( Button* ){
    try {
        game.getWorld().do_coal_survey();
    } catch(const OutOfMoneyMessage::Exception&) {
        game.getGameView().printStatusMessage(_("Not enough credit for a coal survey."));
        closeDialog();
        return;
    }
    windowManager->removeWindow( myDialogComponent );
    blockingDialogIsOpen = false;
    unRegisterDialog();
}

void Dialog::gotoButtonClicked( Button* ){
  game.getGameView().show(point);
}

void Dialog::closeDialogButtonClicked( Button* ){
    closeDialog();
}

void Dialog::closeDialog(){
    windowManager->removeWindow( myDialogComponent );
    if( iAmBlocking ){
        blockingDialogIsOpen = false;
    }
    unRegisterDialog();
}

/** @file lincity-ng/Dialog.cpp */
