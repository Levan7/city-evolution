/* ---------------------------------------------------------------------- *
 * src/lincity/modules/port.cpp
 * This file is part of Lincity-NG.
 *
 * Copyright (C) 1995-1997 I J Peters
 * Copyright (C) 1997-2005 Greg Sharp
 * Copyright (C) 2000-2004 Corey Keasling
 * Copyright (C) 2022-2025 David Bears <dbear4q@gmail.com>
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

#include "port.hpp"
#include <sstream>
#include <algorithm>
#include <limits>

#include <libxml++/parsers/textreader.h>  // for TextReader
#include <libxml/xmlwriter.h>             // for xmlTextWriterWriteElement
#include <string>                         // for basic_string, string, opera...

#include "lincity-ng/Mps.hpp"             // for Mps
#include "lincity/MapPoint.hpp"           // for MapPoint
#include "lincity/groups.hpp"             // for GROUP_PORT
#include "lincity/lin-city.hpp"           // for FALSE, MAX_TECH_LEVEL
#include "lincity/stats.hpp"              // for Stats
#include "lincity/world.hpp"              // for World, Map, MapTile
#include "util/xmlutil.hpp"               // for xmlFormat, xmlStr, xmlParse
#include "util/gettextutil.hpp"

PortConstructionGroup portConstructionGroup(
    N_("Port"),
    N_("Ports"),
     FALSE,                     /* need credit? */
     GROUP_PORT,
     GROUP_PORT_SIZE,
     GROUP_PORT_COLOUR,
     GROUP_PORT_COST_MUL,
     GROUP_PORT_BUL_COST,
     GROUP_PORT_FIREC,
     GROUP_PORT_COST,
     GROUP_PORT_TECH,
     GROUP_PORT_RANGE
);

Construction *PortConstructionGroup::createConstruction(World& world) {
  return new Port(world, this);
}

bool
PortConstructionGroup::can_build_here(const World& world, const MapPoint point,
  Message::ptr& message
) const {
  if(!ConstructionGroup::can_build_here(world, point, message)) return false;

  for(int side=0; side<4; ++side) {
    bool waterfront = true;
    for(int i=0; i<size; ++i) {
      const auto tile = side==0 ? point.e(size).s(i) : side==1 ? point.w().s(i) :
                        side==2 ? point.s(size).e(i) : point.n().e(i);
      waterfront &= world.map(tile)->is_river();
    }
    if(waterfront) return true;
  }
  message = PortRequiresRiverMessage::create();
  return false;
}

Port::Port(World& world, ConstructionGroup *cstgrp) :
  Construction(world)
{
    this->constructionGroup = cstgrp;
    this->daily_ic = 0; this->daily_et = 0;
    this->monthly_ic = 0; this->monthly_et = 0;
    this->lastm_ic = 0; this->lastm_et = 0;
    this->pence = 0;
    this->working_days = 0;
    this->busy = 0;
    this->tech_made = 0;
    initialize_commodities();
    //local copy of commodityRuleCount
    commodityRuleCount = constructionGroup->commodityRuleCount;
    //do not trade labor
    // commodityRuleCount.erase (STUFF_LABOR);
    commodityRuleCount[STUFF_LABOR] = (CommodityRule){
      .maxload = 0,
      .take = false,
      .give = false
    };
    commodityRuleCount[STUFF_FOOD].take = false;
    commodityRuleCount[STUFF_FOOD].give = false;
    commodityRuleCount[STUFF_COAL].take = false;
    commodityRuleCount[STUFF_COAL].give = false;
    commodityRuleCount[STUFF_GOODS].take = false;
    commodityRuleCount[STUFF_GOODS].give = false;
    commodityRuleCount[STUFF_ORE].take = false;
    commodityRuleCount[STUFF_ORE].give = false;
    commodityRuleCount[STUFF_STEEL].take = false;
    commodityRuleCount[STUFF_STEEL].give = false;

    commodityMaxCons[STUFF_LABOR] = 100 * PORT_LABOR;
    for(Commodity stuff = STUFF_INIT ; stuff < STUFF_COUNT ; stuff++) {
        if(!commodityRuleCount[stuff].maxload) continue;
        commodityMaxCons[stuff] = 100 * ((
          portConstructionGroup.commodityRuleCount[stuff].maxload *
          PORT_EXPORT_RATE) / 1000);
        commodityMaxProd[stuff] = 100 * ((
          portConstructionGroup.commodityRuleCount[stuff].maxload *
          PORT_IMPORT_RATE) / 1000);
    }
}

int Port::buy_stuff(Commodity stuff)
//fills up a PORT_IMPORT_RATE fraction of the available capacity and returns the cost
//amount to buy must exceed PORT_TRIGGER_RATE
{
    if(!world.tradeRule[stuff].take)
        return 0;
    int i = portConstructionGroup.commodityRuleCount[stuff].maxload - commodityCount[stuff];
    i = (i * PORT_IMPORT_RATE) / 1000;
    if (i < (portConstructionGroup.commodityRuleCount[stuff].maxload / PORT_TRIGGER_RATE))
    {   return 0;}
    try {
      // 64-bit from the first multiplication: a large order at a high tariff
      // overflows 32 bits. Rounding is unchanged (integer division, remainder
      // carried in importPence).
      const long long base = static_cast<long long>(i) * portConstructionGroup.commodityRates[stuff];
      // Foreign purchases carry a 10% markup over the base export value.
      const long long tariff = std::clamp(world.money_rates.import_cost, 1, 100);
      const long long total = 11LL * base * tariff / 10 + importPence;
      const long long charged64 = total / 100;
      // The treasury is an int, so a bill beyond that can never be paid.
      if(charged64 > std::numeric_limits<int>::max())
        return 0;
      const int charged = static_cast<int>(charged64);
      world.expense(charged, world.stats.expenses.import, false);
      importPence = static_cast<int>(total % 100);
      produceStuff(stuff, i);
      currentTrade.imported[stuff] += i;
      return charged;
    } catch(const OutOfMoneyMessage::Exception& ex) {
      return 0;
    }
}

int Port::sell_stuff(Commodity stuff)
//sells a PORT_IMPORT_RATE fraction of the current load and returns the revenue
//amount to sell must exceed PORT_TRIGGER_RATE
{
    if(!world.tradeRule[stuff].give)
        return 0;
    int i = commodityCount[stuff];
    i = (i * PORT_EXPORT_RATE) / 1000;
    if (i < (portConstructionGroup.commodityRuleCount[stuff].maxload / PORT_TRIGGER_RATE))
    {   return 0;}
    consumeStuff(stuff, i);
    currentTrade.exported[stuff] += i;
    return (i * portConstructionGroup.commodityRates[stuff]);
}

void Port::trade_connection()
{
    //Checks all flags and issues buy_stuff sell_stuff accordingly
    for(Commodity stuff = STUFF_INIT; stuff < STUFF_COUNT; stuff++ ) {
        CommodityRule& rule = commodityRuleCount[stuff];
        if(!rule.maxload) continue;
        if (rule.take == rule.give)
        {   continue;}
        if (rule.take)
        {   daily_ic += buy_stuff(stuff);}
        else if (rule.give)
        {   daily_et += sell_stuff(stuff);}
    }
}

void Port::update()
{
    daily_ic = 0;
    daily_et = 0;

    if (commodityCount[STUFF_LABOR] >= PORT_LABOR)//there is enough workforce
    {
        trade_connection();
        if (daily_ic || daily_et)
        {
            consumeStuff(STUFF_LABOR, PORT_LABOR);
            world.map(point)->pollution += PORT_POLLUTION;
            world.stats.sustainability.trade_flag = false;
            tech_made++;
            world.tech_level++;
            working_days++;
            if (daily_ic && daily_et)
              world.tech_level++;
        }
    }
    monthly_ic += daily_ic;
    monthly_et += daily_et;
    //monthly update
    if(world.total_time % 100 == 99) {
        reset_prod_counters();
        busy = working_days;
        working_days = 0;
        currentTrade.month = world.total_time / NUMOF_DAYS_IN_MONTH;
        currentTrade.imports = monthly_ic;
        currentTrade.exportCents = monthly_et;
        tradeHistory.push_back(currentTrade);
        if(tradeHistory.size() > historyMonths) tradeHistory.pop_front();
        currentTrade = {};
        lastm_ic = monthly_ic;
        lastm_et = monthly_et;
        monthly_ic = 0;
        monthly_et = 0;
    }

    daily_et += pence;
    world.taxable.trade_ex += daily_et / 100;
    pence = daily_et % 100;
}

void Port::report(Mps& mps, bool production) const {
  mps.add_s(constructionGroup->name);
  mps.add_sfp(N_("Last month active"), busy);
  mps.add_sd(N_("Last month exports $"),lastm_et/100);
  mps.add_sd(N_("Last month imports $"),lastm_ic);
  const auto gross = "This mo. $: " + std::to_string(monthly_et/100)
    + " out / " + std::to_string(monthly_ic) + " in";
  mps.add_s(gross);
  list_commodities(mps, production);
}

void Port::save(xmlTextWriterPtr xmlWriter) const {
  xmlTextWriterWriteElement(xmlWriter, (xmlStr)"tech_made", xmlFormat<int>(tech_made));

  xmlTextWriterWriteElement(xmlWriter, (xmlStr)"month_imports", xmlFormat<int>(monthly_ic));
  xmlTextWriterWriteElement(xmlWriter, (xmlStr)"month_exports", xmlFormat<int>(monthly_et));
  xmlTextWriterWriteElement(xmlWriter, (xmlStr)"last_imports", xmlFormat<int>(lastm_ic));
  xmlTextWriterWriteElement(xmlWriter, (xmlStr)"last_exports", xmlFormat<int>(lastm_et));
  xmlTextWriterWriteElement(xmlWriter, (xmlStr)"trade_days", xmlFormat<int>(working_days));
  xmlTextWriterWriteElement(xmlWriter, (xmlStr)"last_trade_days", xmlFormat<int>(busy));
  xmlTextWriterWriteElement(xmlWriter, (xmlStr)"import_pence", xmlFormat<int>(importPence));
  xmlTextWriterWriteElement(xmlWriter, (xmlStr)"export_pence", xmlFormat<int>(pence));
  const std::string givePfx("give_");
  const std::string takePfx("take_");
  for(Commodity stuff = STUFF_INIT; stuff < STUFF_COUNT; stuff++) {
    const CommodityRule& rule = commodityRuleCount[stuff];
    if(!rule.maxload) continue;
    const char *name = commodityStandardName(stuff);
    const std::string giveName = givePfx + name;
    const std::string takeName = takePfx + name;
    const xmlStr giveNameXml = (xmlStr)giveName.c_str();
    const xmlStr takeNameXml = (xmlStr)takeName.c_str();
    xmlTextWriterWriteElement(xmlWriter, giveNameXml, xmlFormat<int>(rule.give));
    xmlTextWriterWriteElement(xmlWriter, takeNameXml, xmlFormat<int>(rule.take));
  }

  auto writeTrade = [&](const char* tag, const TradeMonth& entry) {
    std::ostringstream fields;
    fields << entry.month << ' ' << entry.imports << ' ' << entry.exportCents;
    for(int amount : entry.imported) fields << ' ' << amount;
    for(int amount : entry.exported) fields << ' ' << amount;
    xmlTextWriterWriteElement(xmlWriter, (xmlStr)tag, (xmlStr)fields.str().c_str());
  };
  for(const auto& entry : tradeHistory) writeTrade("trade_history_month", entry);
  writeTrade("trade_history_current", currentTrade);

  Construction::save(xmlWriter);
}

bool Port::loadMember(xmlpp::TextReader& xmlReader, unsigned int ldsv_version) {
  std::string tag = xmlReader.get_name();
  if(tag == "month_imports") monthly_ic = std::max(0, xmlParse<int>(xmlReader.read_inner_xml()));
  else if(tag == "month_exports") monthly_et = std::max(0, xmlParse<int>(xmlReader.read_inner_xml()));
  else if(tag == "last_imports") lastm_ic = std::max(0, xmlParse<int>(xmlReader.read_inner_xml()));
  else if(tag == "last_exports") lastm_et = std::max(0, xmlParse<int>(xmlReader.read_inner_xml()));
  else if(tag == "trade_history_month" || tag == "trade_history_current") {
    std::istringstream fields(xmlReader.read_inner_xml());
    TradeMonth entry;
    if(!(fields >> entry.month >> entry.imports >> entry.exportCents)
        || entry.month < 0 || entry.imports < 0 || entry.exportCents < 0) return true;
    for(auto* amounts : {&entry.imported, &entry.exported})
      for(int& amount : *amounts)
        if(!(fields >> amount) || amount < 0) return true;
    if(tag == "trade_history_current") currentTrade = entry;
    else if(tradeHistory.empty() || tradeHistory.back().month < entry.month) {
      tradeHistory.push_back(entry);
      if(tradeHistory.size() > historyMonths) tradeHistory.pop_front();
    }
  }
  else if(tag == "trade_days") working_days = std::clamp(xmlParse<int>(xmlReader.read_inner_xml()),0,100);
  else if(tag == "last_trade_days") busy = std::clamp(xmlParse<int>(xmlReader.read_inner_xml()),0,100);
  else if(tag == "import_pence") importPence = std::clamp(xmlParse<int>(xmlReader.read_inner_xml()),0,99);
  else if(tag == "export_pence") pence = std::clamp(xmlParse<int>(xmlReader.read_inner_xml()),0,99);
  else {
    bool give;
    Commodity stuff;
    if(((give = tag.find("give_") == 0) || tag.find("take_") == 0) &&
      (stuff = commodityFromStandardName(tag.substr(5).c_str())) != STUFF_COUNT &&
      commodityRuleCount[stuff].maxload
    ) {
      CommodityRule& rule = commodityRuleCount[stuff];
      (give ? rule.give : rule.take) = xmlParse<int>(xmlReader.read_inner_xml());
    }
    else if(tag == "tech_made") tech_made = xmlParse<int>(xmlReader.read_inner_xml());
    else return Construction::loadMember(xmlReader, ldsv_version);
    return true;
  }
  return true;
}

/** @file lincity/modules/port.cpp */
