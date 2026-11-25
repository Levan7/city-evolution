#include "lincity/init_game.hpp"
#include "lincity/world.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/modules/city_business.hpp"
#include "lincity/lin-city.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
static void require(bool ok, const char* message) {
  if(!ok) throw std::runtime_error(message);
}
int main() {
  try {
    initializeModules();
    auto world = new_barren_city(90);
    world->total_money = 10000000;
    world->tech_level = MAX_TECH_LEVEL;
    for(auto* group : {&airportConstructionGroup, &racingCircuitConstructionGroup, &amusementParkConstructionGroup}) {
      const MapPoint position(group->group == GROUP_AIRPORT ? 10 : group->group == GROUP_RACING_CIRCUIT ? 30 : 50, 10);
      world->buildConstruction(*group, position);
      auto* service = static_cast<CityBusiness*>(world->map(position)->construction);
      for(Commodity stuff = STUFF_INIT; stuff < STUFF_COUNT; ++stuff)
        if(group->commodityRuleCount[stuff].take)
          service->commodityCount[stuff] = group->commodityRuleCount[stuff].maxload;
      service->update();
      require(!service->operating, "unconnected service earned revenue");
      world->buildConstruction(roadConstructionGroup, position.w());
      const int before = world->total_money;
      service->update();
      require(service->operating && world->total_money > before,
              "supplied service did not operate or earn net revenue");
      service->commodityCount[STUFF_LOVOLT] = 0;
      service->update();
      require(!service->operating, "service operated without electricity");
    }
    require(world->stats.income.service_revenue.acc == -125 && world->stats.income.goods_tax.acc == 0,
            "airport and circuit receipts posted to incorrect account");
    const auto file = std::filesystem::temp_directory_path()/"ce-airport-check.scn.gz";
    world->save(file);
    auto loaded = World::load(file);
    std::filesystem::remove(file);
    require(loaded->stats.income.service_revenue.acc == -125, "lost service receipts on reload");
    require(loaded->map({10,10})->construction->constructionGroup == &airportConstructionGroup,
            "airport did not survive save/load");
    require(loaded->map({30,10})->construction->constructionGroup == &racingCircuitConstructionGroup,
            "circuit did not survive save/load");
    require(loaded->map({50,10})->construction->constructionGroup == &amusementParkConstructionGroup,
            "amusement park did not survive save/load");
    std::cout << "PASS airport, circuit and amusement park: access, inputs, revenue, power failure, save/load\n";
  } catch(const std::exception& ex) {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
