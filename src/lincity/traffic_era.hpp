// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <cstdint>
namespace TrafficEra {
inline int motorShare(int tech,int maximum) {
 if(maximum<=0 || tech*100LL<maximum*10)return 0;
 return std::clamp(1+int((tech*100LL-maximum*10)*99/(maximum*20LL)),1,100);
}
inline bool motorVehicle(int tech,int maximum,std::uint32_t seed){return int(seed%100)<motorShare(tech,maximum);}
}
