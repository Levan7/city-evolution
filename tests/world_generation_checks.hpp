// SPDX-License-Identifier: GPL-2.0-or-later
// Optional Windows-side integration checks: --verify-worlds <scratch directory>.
#ifndef LINCITY_WORLD_GENERATION_CHECKS_HPP
#define LINCITY_WORLD_GENERATION_CHECKS_HPP
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/world_setup.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

inline int verifyWorldGeneration(const std::filesystem::path& scratch, bool initialize = true) {
  try {
    std::filesystem::create_directories(scratch);
    if(initialize) initializeModules();
    std::srand(20261101);
    int worlds = 0;
    auto require = [](bool ok, const char* message) {
      if(!ok) throw std::runtime_error(message);
    };
    for(const auto& preset : WorldSetup::sizes) {
      auto world = new_barren_city(preset.side);
      require(world->map.len() == preset.side, "barren world has wrong dimensions");
      require(world->map.constructions.empty(), "barren world contains buildings");
      require(world->total_money == WorldSetup::emptyStartMoney, "empty-start budget missing");
      for(const auto& tile : world->map) {
        require(tile.group == GROUP_BARE, "barren world contains non-bare terrain");
        require(!tile.construction && !tile.reportingConstruction, "barren tile contains a construction");
        require(tile.ground.altitude == 0, "barren world is not flat");
        require(tile.flags & FLAG_HAS_UNDERGROUND_WATER, "barren world has no groundwater");
      }
      const auto file = scratch / ("barren-" + std::to_string(preset.side) + ".scn.gz");
      world->save(file);
      auto copy = World::load(file);
      require(copy->map.len() == preset.side && copy->map.constructions.empty(), "barren save/load failed");
      require(copy->map.alt_step >= 1, "flat terrain altitude scale is zero after load");
      require(copy->total_money == WorldSetup::emptyStartMoney, "save/load changed starting budget");
      for(int day = 0; day < 5; ++day) copy->do_time_step();
      std::filesystem::remove(file);
      ++worlds;
      std::cout << "PASS barren generation/save/load/simulation " << preset.side << " x " << preset.side << '\n';
    }
    city_settings empty;
    empty.with_village = false;
    empty.without_trees = true;
    for(int size : {100, 200, 400}) {
      for(int biome = 0; biome < 4; ++biome) {
        auto world = create_new_city(&empty, size, biome == 0, biome);
        require(world->map.len() == size && world->map.constructions.empty(), "empty natural world contains a settlement");
        require(world->total_money == WorldSetup::emptyStartMoney, "natural empty-start budget missing");
        for(const auto& tile : world->map)
          require(tile.group != GROUP_TREE && tile.group != GROUP_TREE2 && tile.group != GROUP_TREE3, "remove-trees mode contains trees");
        ++worlds;
        std::cout << "PASS empty natural world " << size << " biome " << biome << '\n';
      }
    }
    city_settings village;
    village.with_village = true;
    auto town = new_city(&village, 100);
    require(!town->map.constructions.empty(), "village option did not create a village");
    std::cout << "PASS starter-village regression\nPASS all " << worlds + 1 << " generated-world checks\n";
    return 0;
  } catch(const std::exception& error) {
    std::cerr << "FAIL world verification: " << error.what() << '\n';
    return 1;
  }
}
#endif
