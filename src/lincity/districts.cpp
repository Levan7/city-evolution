// SPDX-License-Identifier: GPL-2.0-or-later
// Districts: player-drawn, named, typed zones. Deterministic, integers only.
#include "city_evolution.hpp"
#include "world.hpp"
#include "lintypes.hpp"
#include "modules/all_modules.hpp"

const char* CityEvolution::districtTypeName(int type) {
  switch(type) {
    case DIST_INDUSTRIAL: return "Industrial";
    case DIST_TECH: return "Technology";
    case DIST_FARMING: return "Farming";
    case DIST_TRADE: return "Trade";
    case DIST_UNIVERSITY: return "University";
  }
  return "Unknown";
}

int CityEvolution::districtAt(int x, int y) const {
  for(int i = int(districts.size()) - 1; i >= 0; --i) { // newest district wins where zones overlap
    const District& d = districts[i];
    if(x >= d.x0 && x <= d.x1 && y >= d.y0 && y <= d.y1) return i;
  }
  return -1;
}

static std::string cleanName(std::string name) {
  std::string out;
  for(unsigned char c : name) if(c >= 32 && c != '|' && c != 127) out += char(c);
  while(!out.empty() && out.back() == ' ') out.pop_back();
  size_t start = 0; while(start < out.size() && out[start] == ' ') ++start;
  out = out.substr(start);
  // Keep names short (by bytes, without cutting a UTF-8 character in half).
  if(out.size() > 28) { out.resize(28); while(!out.empty() && (static_cast<unsigned char>(out.back()) & 0xC0) == 0x80) out.pop_back();
                        if(!out.empty() && static_cast<unsigned char>(out.back()) >= 0xC0) out.pop_back(); }
  return out;
}

bool CityEvolution::addDistrict(World& w, int type, int x0, int y0, int x1, int y1, std::string name) {
  if(type < DIST_INDUSTRIAL || type > DIST_UNIVERSITY || int(districts.size()) >= MAX_DISTRICTS) return false;
  const int len = w.map.len();
  if(x0 > x1) std::swap(x0, x1);
  if(y0 > y1) std::swap(y0, y1);
  x0 = std::clamp(x0, 0, len - 1); x1 = std::clamp(x1, 0, len - 1);
  y0 = std::clamp(y0, 0, len - 1); y1 = std::clamp(y1, 0, len - 1);
  if(x1 - x0 < 1 || y1 - y0 < 1) return false; // at least 2 x 2 tiles
  name = cleanName(name);
  if(name.empty()) name = std::string(districtTypeName(type)) + " district " + std::to_string(districts.size() + 1);
  districts.push_back({type, x0, y0, x1, y1, name});
  updateDistricts(w);
  headline("New " + std::string(districtTypeName(type)) + " district: " + name + ".");
  return true;
}

void CityEvolution::removeDistrict(int i) {
  if(i < 0 || i >= int(districts.size())) return;
  headline("District removed: " + districts[i].name + ". Its buildings stay.");
  districts.erase(districts.begin() + i);
  if(mayorScope == 1) { if(mayorDistrict == i) { mayorScope = 0; mayorDistrict = -1; } else if(mayorDistrict > i) --mayorDistrict; }
  if(i < int(districtStats.size())) districtStats.erase(districtStats.begin() + i);
}

void CityEvolution::renameDistrict(int i, std::string name) {
  if(i < 0 || i >= int(districts.size())) return;
  name = cleanName(name);
  if(!name.empty()) districts[i].name = name;
}

void CityEvolution::retypeDistrict(World& w, int i, int type) {
  if(i < 0 || i >= int(districts.size()) || type < DIST_INDUSTRIAL || type > DIST_UNIVERSITY) return;
  districts[i].type = type; updateDistricts(w);
}

void CityEvolution::updateDistricts(World& w) {
  districtStats.assign(districts.size(), DistrictStats{});
  if(districts.empty()) return;
  for(auto* c : w.map.constructions) {
    const int i = districtAt(c->point.x, c->point.y);
    if(i < 0) continue;
    DistrictStats& s = districtStats[i];
    const int g = c->constructionGroup->group;
    if(GROUP_IS_RESIDENCE(g)) { ++s.homes; continue; }
    switch(g) {
      case GROUP_SCHOOL: ++s.schools; break;
      case GROUP_UNIVERSITY: ++s.universities; break;
      case GROUP_TECH_CAMPUS: ++s.campuses; break;
      case GROUP_INDUSTRY_L: case GROUP_INDUSTRY_H: ++s.factories; break;
      case GROUP_ORGANIC_FARM: {
        ++s.farms;
        const int cap = c->constructionGroup->commodityRuleCount[STUFF_FOOD].maxload;
        if(cap > 0 && c->commodityCount[STUFF_FOOD] * 10 >= cap * 9) ++s.backedUpFarms; // storage full: harvest cannot leave
        break; }
      case GROUP_PORT: ++s.ports; break;
      case GROUP_WAREHOUSE: ++s.warehouses; break;
      case GROUP_MARKET: ++s.markets; break;
      default: continue;
    }
    ++s.buildings;
  }
  for(size_t i = 0; i < districts.size(); ++i) {
    DistrictStats& s = districtStats[i];
    const int match[6] = {0, s.factories, s.campuses, s.farms, s.ports + s.warehouses, s.schools + s.universities};
    int best = DIST_INDUSTRIAL;
    for(int t = DIST_INDUSTRIAL; t <= DIST_UNIVERSITY; ++t) if(match[t] > match[best]) best = t;
    s.bestType = match[best] > 0 ? best : 0;
    s.fit = s.buildings ? match[districts[i].type] * 100 / s.buildings : 0;
  }
}

int CityEvolution::districtBonusAt(int x, int y, int kind) const {
  const int i = districtAt(x, y);
  if(i < 0 || i >= int(districtStats.size())) return 0;
  const District& d = districts[i]; const DistrictStats& s = districtStats[i];
  if(kind == 0 && d.type == DIST_INDUSTRIAL && s.factories >= 2) return DISTRICT_CLUSTER_BONUS;
  if(kind == 1 && d.type == DIST_TECH && s.universities + s.schools >= 1) return DISTRICT_CLUSTER_BONUS;
  return 0;
}

std::string CityEvolution::describeDistrict(int i) const {
  if(i < 0 || i >= int(districts.size())) return "";
  const District& d = districts[i];
  std::string t = d.name + " (" + districtTypeName(d.type) + ", " + std::to_string(d.x1 - d.x0 + 1) + "x" + std::to_string(d.y1 - d.y0 + 1) + " tiles): ";
  if(i >= int(districtStats.size())) return t + "not yet measured";
  const DistrictStats& s = districtStats[i];
  if(!s.buildings && !s.homes) return t + "empty zone. Build " + std::string(districtTypeName(d.type)) + " buildings here.";
  auto add = [&](std::string& out, int n, const char* what) { if(n) out += (out.empty() ? "" : ", ") + std::to_string(n) + " " + what; };
  std::string c;
  add(c, s.homes, "homes"); add(c, s.factories, "factories"); add(c, s.campuses, "campuses"); add(c, s.universities, "universities");
  add(c, s.schools, "schools"); add(c, s.farms, "farms"); add(c, s.ports, "ports"); add(c, s.warehouses, "warehouses"); add(c, s.markets, "markets");
  t += c + ". ";
  if(s.buildings) t += std::to_string(s.fit) + "% of its work buildings fit its type.";
  const int bonus = districtBonusAt(d.x0, d.y0, d.type == DIST_INDUSTRIAL ? 0 : 1);
  if(d.type == DIST_INDUSTRIAL) t += s.factories >= 2 ? " Cluster bonus ON: factories +20% speed." : " Needs 2 factories for the cluster bonus.";
  else if(d.type == DIST_TECH) t += (s.universities + s.schools) ? " Research bonus ON: campuses +20% speed." : " Needs a school or university here for the research bonus.";
  else if(d.type == DIST_UNIVERSITY) t += (s.universities + s.schools) ? " Schools and universities here train 25% more." : " Needs a school or university.";
  (void)bonus;
  return t;
}
