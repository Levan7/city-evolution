// SPDX-License-Identifier: GPL-2.0-or-later
// Regression test: no building's info panel may ask for more lines than the
// panel holds (Mps::PARAGRAPH_COUNT). A report that is one line too long once
// crashed release builds, where the old assert() was compiled out.
//
// Every construction group is built and its report is generated in many states,
// including the conditional ones: occupied housing that is not fed yet (extra
// supply-advice line), empty or stocked inventories, warehouses with and
// without a road link or a buyer, and both the inventory and production pages.
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/modules/city_business.hpp"
#include "lincity/modules/residence.hpp"
#include "lincity/world.hpp"
#include "lincity-ng/Mps.hpp"
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

extern int mpsLinesAdded;  // counted by the headless Mps stubs

namespace {
struct Panel { std::string failures; int checkedStates = 0; };

// The report functions only call Mps::add_*; the headless stubs count the calls.
Mps& dummyMps() { alignas(16) static char storage[sizeof(Mps)]; return *reinterpret_cast<Mps*>(storage); }

int reportLines(const Construction& c, bool production) {
  mpsLinesAdded = 0;
  c.report(dummyMps(), production);
  return mpsLinesAdded;
}

void checkStates(Panel& panel, Construction& c, const std::string& label) {
  const int limit = Mps::PARAGRAPH_COUNT;
  auto* residence = dynamic_cast<Residence*>(&c);
  auto* business = dynamic_cast<CityBusiness*>(&c);
  for(int production = 0; production < 2; ++production)
  for(int fed = 0; fed < 2; ++fed)
  for(int stock = 0; stock < 3; ++stock)
  for(int people = 0; people < (residence ? 2 : 1); ++people)
  for(int freight = 0; freight < (business ? 3 : 1); ++freight) {
    if(fed) c.flags |= FLAG_FED; else c.flags &= ~FLAG_FED;
    for(int i = 0; i < STUFF_COUNT; ++i) {
      const auto s = static_cast<Commodity>(i);
      const int max = c.constructionGroup->commodityRuleCount[s].maxload;
      c.commodityCount[s] = stock == 0 ? 0 : stock == 1 ? max / 2 : max;
    }
    if(residence) residence->local_population = people ? 10 : 0;
    if(business) { business->freightSurveyed = freight != 0; business->freightHasBuyer = freight == 2; }
    const int lines = reportLines(c, production);
    ++panel.checkedStates;
    if(lines > limit) {
      std::ostringstream out;
      out << "  " << label << ": " << lines << " lines > " << limit << " (production=" << production
          << " fed=" << fed << " stock=" << stock << " people=" << people << " freight=" << freight << ")\n";
      panel.failures += out.str();
    }
  }
}
}  // namespace

int main() {
  try {
    initializeModules();
    auto w = new_barren_city(160);
    w->total_money = 1000000000;
    w->tech_level = MAX_TECH_LEVEL;

    Panel panel;
    std::set<std::string> skipped;
    int built = 0, unfedOccupiedHousing = 0, n = 0;
    for(unsigned id = 0; id < 400; ++id) {
      ConstructionGroup* group = ConstructionGroup::getConstructionGroup(static_cast<unsigned short>(id));
      if(!group) continue;
      const int size = group->size;
      const int x = 3 + (n % 10) * 14, y = 3 + (n / 10) * 14;
      ++n;
      if(y + 12 >= 158) { skipped.insert(group->name + " (no room in test map)"); continue; }
      // Road/track link on the west side so freight and transport states exist.
      try { w->buildConstruction(trackConstructionGroup, {x - 1, y}); } catch(...) {}
      for(int i = 0; i < size; ++i) {   // river on the east side for ports
        w->map({x + size, y + i})->setTerrain(GROUP_WATER);
        w->map({x + size, y + i})->flags |= FLAG_IS_RIVER;
      }
      try {
        Message::ptr message;
        if(!group->can_build(*w, message) || !group->can_build_here(*w, {x, y}, message)) {
          if(id != GROUP_UNIVERSITY && id != GROUP_TECH_CAMPUS && id != GROUP_FOODHALL) {
            skipped.insert(group->name); continue;
          }
          group->placeItem(*w, {x, y});
        } else {
          w->buildConstruction(*group, {x, y});
        }
      } catch(...) { skipped.insert(group->name); continue; }
      Construction* c = w->map({x, y})->reportingConstruction;
      if(!c) { skipped.insert(group->name); continue; }
      ++built;
      const std::string label = group->name + " (group " + std::to_string(group->group) + ")";
      checkStates(panel, *c, label);
      if(auto* residence = dynamic_cast<Residence*>(c)) {
        residence->local_population = 10;
        c->flags &= ~FLAG_FED;
        for(int i = 0; i < STUFF_COUNT; ++i) c->commodityCount[static_cast<Commodity>(i)] = 0;
        const bool advice = !residence->foodSupplyAdvice().empty();
        std::cout << "  housing " << label << ": supply advice " << (advice ? "shown" : "not shown")
                  << ", " << reportLines(*c, false) << " report lines\n";
        if(advice) ++unfedOccupiedHousing;
      }
    }

    std::cout << "panel capacity: " << built << " building types built, " << panel.checkedStates
              << " report states checked, limit " << Mps::PARAGRAPH_COUNT << " lines\n";
    if(!skipped.empty()) {
      std::cout << "  not built here:";
      for(const auto& name : skipped) std::cout << " [" << name << "]";
      std::cout << "\n";
    }
    if(!panel.failures.empty())
      throw std::runtime_error("info panel overflow:\n" + panel.failures);
    // Guard the guard: the conditional housing line must really have been exercised,
    // and a mass skip must not turn this test into a no-op.
    if(unfedOccupiedHousing != 9)
      throw std::runtime_error("test did not reach occupied, unfed housing (supply advice line)");
    if(!skipped.empty() || built < 70)
      throw std::runtime_error("too few building types were built to trust this scan: " + std::to_string(built));
    std::cout << "PASS info panel capacity\n";
  } catch(const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  return 0;
}
