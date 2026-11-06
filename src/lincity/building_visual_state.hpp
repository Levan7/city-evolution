// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "lintypes.hpp"
#include "groups.hpp"
#include "modules/market.hpp"
#include <algorithm>
struct StockVisual {int kind=-1,level=0;};
inline StockVisual buildingStockVisual(const Construction& c){
 int group=c.constructionGroup->group,kind=-1,ratio=0;Commodity item=STUFF_GOODS;
 if(group==GROUP_MARKET){ratio=static_cast<const Market&>(c).market_ratio;kind=0;return {kind,ratio<10?0:ratio<20?1:ratio<50?2:3};}
 switch(group){case GROUP_WAREHOUSE:kind=1;break;case GROUP_OREMINE:kind=2;item=STUFF_ORE;break;case GROUP_COALMINE:kind=3;item=STUFF_COAL;break;case GROUP_POTTERY:case GROUP_BLACKSMITH:case GROUP_MILL:case GROUP_INDUSTRY_L:kind=1;break;case GROUP_INDUSTRY_H:kind=1;item=STUFF_STEEL;break;default:return {};}
 int capacity=c.constructionGroup->commodityRuleCount[item].maxload;ratio=capacity>0?int(100LL*std::max(0,c.commodityCount[item])/capacity):0;
 return {kind,ratio<=0?0:ratio<25?1:ratio<60?2:3};
}
