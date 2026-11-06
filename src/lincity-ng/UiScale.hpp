// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
struct UiGeometry { int width,height;float scale; };
inline UiGeometry calculateUiGeometry(int width,int height) {
 width=std::max(1,width);height=std::max(1,height);
 float scale=std::clamp(std::floor(std::min(width/1440.f,height/900.f)*4)/4,1.f,3.f);
 if(width<1024||height<720)scale=std::min(width/1024.f,height/720.f);
 return {std::max(1,int(std::round(width/scale))),std::max(1,int(std::round(height/scale))),scale};
}
