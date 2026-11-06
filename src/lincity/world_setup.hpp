// SPDX-License-Identifier: GPL-2.0-or-later
// Shared map-creation policy for the Expanded Worlds community build.
#ifndef LINCITY_WORLD_SETUP_HPP
#define LINCITY_WORLD_SETUP_HPP

#include <algorithm>
#include <array>
#include <string>

namespace WorldSetup {
struct SizePreset { int side; const char* name; };
inline constexpr std::array<SizePreset, 6> sizes{{
  {50, "Small"}, {100, "Medium-small"}, {150, "Medium"},
  {200, "Large"}, {300, "Huge"}, {400, "Vast"}
}};
inline constexpr int emptyStartMoney = 200000;
inline int nextSize(int current, bool larger) {
  if(larger) {
    for(const auto& size : sizes) if(size.side > current) return size.side;
    return sizes.back().side;
  }
  for(auto it = sizes.rbegin(); it != sizes.rend(); ++it)
    if(it->side < current) return it->side;
  return sizes.front().side;
}
inline std::string label(int side) {
  std::string name = "Custom";
  for(const auto& size : sizes) if(size.side == side) name = size.name;
  return name + "  " + std::to_string(side) + " x " + std::to_string(side);
}
inline int boundedSize(int side) { return std::clamp(side, 50, 400); }
inline int altitudeStep(int low, int high) { return std::max(1, (high - low) / 10); }
}
#endif
