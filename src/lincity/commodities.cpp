/* ---------------------------------------------------------------------- *
 * src/lincity/commodities.cpp
 * This file is part of Lincity-NG.
 *
 * Copyright (C) 2022-2024 David Bears <dbear4q@gmail.com>
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

#include "commodities.hpp"

#include <cstring>

#define N_(MSG) MSG

Commodity& operator++(Commodity& stuff) {
  return stuff = (Commodity)((int)stuff + 1);
}
Commodity operator++(Commodity& stuff, int) {
  Commodity tmp(stuff);
  ++stuff;
  return tmp;
}

const char *commodityNames[] = {
  N_("Food"),
  N_("Labor"),
  N_("Coal"),
  N_("Goods"),
  N_("Ore"),
  N_("Steel"),
  N_("Waste"),
  N_("Lo-Volt"),
  N_("Hi-Volt"),
  N_("Water"),
  N_("Timber"),
  N_("Grapes"),
  N_("Wine"),
  N_("Sparkling"),
  N_("Spirits"),
  N_("Computers"),
};
const char *commodityStandardNames[] = {
  "food",
  "labor",
  "coal",
  "goods",
  "ore",
  "steel",
  "waste",
  "lovolt",
  "hivolt",
  "water",
  "timber",
  "grapes",
  "wine",
  "sparkling",
  "spirits",
  "computers"
};

const char *commodityName(Commodity stuff) {
  if(stuff >= STUFF_INIT && stuff < STUFF_COUNT)
    return commodityNames[stuff];
  else
    return "Unknown";
}

const char *commodityStandardName(Commodity stuff){
  if(stuff >= STUFF_INIT && stuff < STUFF_COUNT)
    return commodityStandardNames[stuff];
  else
    return "unknown";
}

Commodity commodityFromStandardName(const char *name) {
  for(Commodity commodity = STUFF_INIT; commodity < STUFF_COUNT; ++commodity)
    if(std::strcmp(name, commodityStandardNames[commodity]) == 0) return commodity;
  return STUFF_COUNT;
}
