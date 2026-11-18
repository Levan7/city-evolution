// SPDX-License-Identifier: GPL-2.0-or-later
// Port import pricing uses 64-bit arithmetic from the first multiplication, keeps
// the original rounding, and never lets an unpayable bill wrap around an int.
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/modules/port.hpp"
#include "lincity/world.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
void need(bool ok, const std::string& what) { if(!ok) throw std::runtime_error(what); }

long long importedAmount(const Port& port, Commodity stuff) {
  const int maxload = portConstructionGroup.commodityRuleCount[stuff].maxload;
  return (static_cast<long long>(maxload - port.commodityCount[stuff]) * PORT_IMPORT_RATE) / 1000;
}
}  // namespace

int main() {
  try {
    initializeModules();
    auto w = new_barren_city(60);
    w->total_money = 1000000000;
    w->tech_level = MAX_TECH_LEVEL;
    for(int y = 20; y < 24; ++y) {
      w->map({24, y})->setTerrain(GROUP_WATER);
      w->map({24, y})->flags |= FLAG_IS_RIVER;
    }
    w->buildConstruction(portConstructionGroup, {20, 20});
    auto* port = static_cast<Port*>(w->map({20, 20})->construction);
    const Commodity stuff = STUFF_ORE;
    w->tradeRule[stuff].take = true;
    w->money_rates.import_cost = 100;
    const int originalRate = portConstructionGroup.commodityRates[stuff];

    const long long i = importedAmount(*port, stuff);
    need(i > 1000, "test setup: expected a sizeable import order, got " + std::to_string(i));
    const int tariff = std::clamp(w->money_rates.import_cost, 1, 100);

    // Ordinary price: same result as the documented formula.
    {
      port->commodityCount[stuff] = 0; port->importPence = 0;
      const long long total = 11LL * i * originalRate * tariff / 10;
      const int before = w->total_money;
      const int charged = port->buy_stuff(stuff);
      need(charged == total / 100 && before - w->total_money == charged && port->importPence == total % 100,
        "ordinary import price changed");
    }

    // Large order at the top tariff: 11 * base * tariff exceeds 32 bits but the
    // bill itself fits, so it must be charged exactly (it used to wrap).
    {
      const int rate = static_cast<int>(3000000 / i);
      portConstructionGroup.commodityRates[stuff] = rate;
      port->commodityCount[stuff] = 0; port->importPence = 17;
      const long long base = i * rate;
      const long long total = 11LL * base * tariff / 10 + 17;
      need(11LL * base * tariff > std::numeric_limits<int>::max(), "test setup: product should exceed 32 bits");
      const int before = w->total_money;
      const int charged = port->buy_stuff(stuff);
      need(charged == total / 100, "large import was mischarged (32-bit overflow?)");
      need(before - w->total_money == charged, "treasury did not pay the large import");
      need(port->importPence == total % 100, "rounding remainder was not carried");
      need(port->commodityCount[stuff] > 0, "large affordable import was not delivered");
    }

    // A bill beyond the treasury's int range can never be paid: skip it cleanly.
    {
      portConstructionGroup.commodityRates[stuff] = 2000000000;
      port->commodityCount[stuff] = 0; port->importPence = 5;
      const int before = w->total_money;
      const int charged = port->buy_stuff(stuff);
      need(charged == 0 && w->total_money == before && port->commodityCount[stuff] == 0
           && port->importPence == 5, "an impossible bill was not skipped cleanly");
    }

    portConstructionGroup.commodityRates[stuff] = originalRate;
    std::cout << "PASS import pricing: ordinary, 64-bit large order, impossible bill\n";
  } catch(const std::exception& e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
