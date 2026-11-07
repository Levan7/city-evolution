from pathlib import Path
import xml.etree.ElementTree as E
s=Path('review16/city-evolution')
def edit(f,a,b):
 p=s/f;t=p.read_text();assert a in t,(f,a[:60]);p.write_text(t.replace(a,b))
# No simulation tick may escape on the frame a management window is opened.
edit('src/gui/WindowManager.hpp','  void addWindow(std::unique_ptr<Window>&& window);','  bool hasOpenWindows() const { return !childs.empty() || !addQueue.empty(); }\n  void addWindow(std::unique_ptr<Window>&& window);')
edit('src/lincity-ng/Game.cpp','if(tick >= next_execute) { // execute','if(tick >= next_execute && simDelay != SIM_DELAY_PAUSE && !blockingDialogIsOpen && !windowmanager->hasOpenWindows()) { // execute')
edit('src/lincity-ng/Game.cpp','simDelay == SIM_DELAY_PAUSE || blockingDialogIsOpen','simDelay == SIM_DELAY_PAUSE || blockingDialogIsOpen || windowmanager->hasOpenWindows()')
# Keep rendered information inside the HUD page so controls stay uncovered.
edit('data/gui/mps.xml','width="260" height="272"','width="264" height="204"')
edit('data/gui/mps.xml','font-size="14"','font-size="13"')
# Constrain inactive/oversized switch children for both hit testing and drawing.
edit('src/gui/SwitchComponent.cpp','        if(! (child.getComponent()->getFlags() & FLAG_RESIZABLE))','        child.setClipRect(Rect2D(0,0,width,height));\n        if(! (child.getComponent()->getFlags() & FLAG_RESIZABLE))')
edit('src/gui/SwitchComponent.cpp','#include "SwitchComponent.hpp"','#include "SwitchComponent.hpp"\n#include "Rect2D.hpp"')
edit('src/gui/SwitchComponent.cpp','    for(Childs::const_iterator i = childs.begin(); i != childs.end(); ++i) {','    if(pos.x<0 || pos.y<0 || pos.x>=width || pos.y>=height) return false;\n    for(Childs::const_iterator i = childs.begin(); i != childs.end(); ++i) {')
# A running plan has committed targets; explain why a focus switch is unavailable.
edit('src/lincity-ng/Game.cpp','world->evolution.changePlanFocus(*world);refresh();','''auto& e=world->evolution;
    if(e.planState!=CityEvolution::PLAN_PROPOSED)
      getGameView().printStatusMessage("Focus changes apply to a proposed plan. The active plan keeps its agreed targets.");
    else { e.changePlanFocus(*world); getGameView().printStatusMessage("Plan focus changed; review the new targets before approval."); }
    refresh();''')
edit('src/lincity-ng/Game.cpp','  refresh();\n  hallRefresh = refresh;', '''  refresh();
  hallRefresh = refresh;''')
# Add an explanation to the native focus caption on every refresh.
edit('src/lincity-ng/Game.cpp','    const auto& e = world->evolution;','''    const auto& e = world->evolution;
    const char* focus=e.planFocus==1?"Livable city":e.planFocus==2?"Skills / education":e.planFocus==3?"Technology hub":"None";
    getButton(*w,"PlanFocus")->setCaptionText(e.planState==CityEvolution::PLAN_PROPOSED ? std::string("Change focus: ")+focus : std::string("Focus locked: ")+focus);
    getButton(*w,"PedestrianControl")->setCaptionText(e.pedestrians?"People visible - hide":"People hidden - show");''')
# The replacement above is only in City Hall (single occurrence).
edit('src/lincity/civic_services.hpp',' void update(World&); void monthly(World&);',' void adjustTaxes(World&); std::string financialAdvice(const World&) const;\n void update(World&); void monthly(World&);')
p=s/'src/lincity/civic_services.cpp';t=p.read_text();a=t.index(' if(w.evolution.mayorMode){\n  const auto& in=');b=t.index('\n\n}',a)
t=t[:a]+''' if(autoTax)adjustTaxes(w);
 if(autoBudget)for(int s=0;s<4;++s){int desired=w.total_money<reserve?75:100;if(s==HEALTH&&pop&&sickTotal+w.flu.active>pop/20&&w.total_money>=reserve)desired=125;budget[s]+=std::clamp(desired-budget[s],-10,10);}
'''+t[b:]
t+='''
void CivicServices::adjustTaxes(World& w) {
 const auto& in=w.stats.income; const auto& ex=w.stats.expenses;
 long long revenue=1LL*in.income_tax.stat+in.goods_tax.stat+in.coal_tax.stat+in.export_tax.stat+in.transit_fares.stat;
 long long operating=1LL*ex.unemployment.stat+ex.transport.stat+ex.windmill.stat+ex.university.stat+ex.recycle.stat+ex.deaths.stat+ex.health.stat+ex.school.stat+ex.police.stat+ex.church.stat+ex.firestation.stat+ex.cricket.stat+ex.interest.stat;
 const bool deficit=revenue>0&&operating>revenue;
 int population=0; for(auto*c:w.map.constructions)if(auto*r=dynamic_cast<Residence*>(c))population+=r->local_population;
 int& rate=w.money_rates.income_tax; int ceiling=std::clamp(taxCeiling,1,30);
 if(rate>ceiling) --rate;
 else if(population>0&&(w.total_money<reserve||deficit)&&rate<ceiling)++rate;
 else if(w.total_money>4LL*reserve&&!deficit&&rate>8)--rate;
 // Goods taxes and service funding have their own explicit controls.
}
std::string CivicServices::financialAdvice(const World& w) const {
 const auto& in=w.stats.income;const auto& ex=w.stats.expenses;
 long long revenue=1LL*in.income_tax.stat+in.goods_tax.stat+in.coal_tax.stat+in.export_tax.stat+in.transit_fares.stat;
 long long costs=1LL*ex.unemployment.stat+ex.transport.stat+ex.windmill.stat+ex.university.stat+ex.recycle.stat+ex.deaths.stat+ex.health.stat+ex.school.stat+ex.police.stat+ex.church.stat+ex.firestation.stat+ex.cricket.stat+ex.interest.stat;
 std::string result="Last completed year: revenue $"+std::to_string(revenue)+", operating costs $"+std::to_string(costs)+". Construction is excluded. ";
 result+=revenue==0?"No established tax base yet: connect occupied housing to food, water, markets and working jobs before raising taxes. ":costs>revenue?"Operating deficit: check idle services and missing workers/supplies before adding new buildings. ":"Operating income covers costs: preserve essential services and grow only where demand exists. ";
 if(ex.unemployment.stat>in.income_tax.stat/2)result+="Social support is a large expense: supply existing workplaces and connect homes to jobs. ";
 if(w.total_money<reserve)result+="Cash is below your reserve: defer optional projects. ";
 result+="Auto tax works independently of building autonomy, changes income tax by at most one percentage point per monthly review, and respects the ceiling. It does not change goods tax or service budgets. Manual tax changes turn auto tax off.";
 return result;
}
''';p.write_text(t)
edit('src/lincity-ng/Game.cpp','c.autoTax?"Mayor taxes: ON":"Mayor taxes: OFF"','c.autoTax?"[ON] Auto-adjust taxes":"Auto-adjust taxes: OFF"')
edit('src/lincity-ng/Game.cpp','bind("AutoTax",[this](){world->civic.autoTax=!world->civic.autoTax;});','''bind("AutoTax",[this](){auto& c=world->civic;c.autoTax=!c.autoTax;if(c.autoTax)c.adjustTaxes(*world);});
  bind("FinancialAdvice",[this](){DialogBuilder().titleText("Financial advisor").messageAddText(world->civic.financialAdvice(*world)).buttonSet(DialogBuilder::ButtonSet::OK).build();});''')
for name in ['TaxDown','TaxUp','BusinessDown','BusinessUp']:
 edit('src/lincity-ng/Game.cpp',f'bind("{name}",[this](){{',f'bind("{name}",[this](){{world->civic.autoTax=false;')
edit('src/lincity-ng/Game.cpp','    getGameView().setMapDirty();\n  };\n  auto bind=', '''    getParagraph(*w,"ServiceNote")->setText(c.autoTax?"AUTO TAX ON: monthly income-tax review, max 1 point. Other taxes and funding stay under their own controls.":"AUTO TAX OFF: income tax is manual. Financial advisor explains costs and recommendations.");
    getGameView().setMapDirty();
  };
  auto bind=''')
p=s/'data/gui/publicservices.xml';t=p.read_text().replace('Mayor taxes: OFF','Auto-adjust taxes: OFF');t=t.replace('<Paragraph color="#edf0df" font-size="14">Full autonomy controls building and funding.</Paragraph>','''<Button name="FinancialAdvice" width="320" height="34"><image src="images/readable/plate-320-normal.png" resizable="yes"/><image-hover src="images/readable/plate-320-hover.png" resizable="yes"/><text-caption color="#ffe39a" halign="center" font-size="16">Financial advisor</text-caption></Button>''');t=t.replace('<text-caption color="#ffffff" halign="center" font-size="16">Auto-adjust taxes: OFF','<text-caption color="#ffe39a" halign="center" font-size="16">Auto-adjust taxes: OFF');p.write_text(t)
# Brighter, consistently warm window overlays; modest ambient detail remains visible.
edit('src/lincity-ng/GameView.cpp','const int strength = powered ? 110 : 148;','const int strength = powered ? 80 : 110;')
edit('src/lincity-ng/GameView.cpp','float alpha=amount*(layer.evening?1-late:1);','float alpha=std::min(1.f,amount*1.25f)*(layer.evening?1-late*.35f:1);')
# Crop art follows the original render footprint, and housing variants inherit lighting.
edit('src/lincity-ng/GameView.cpp','auto it=nightCrops.find(key); if(it==nightCrops.end()) return;','''auto it=nightCrops.find(key);
  if(it==nightCrops.end()) {auto fallback=key;auto variant=fallback.find("Variant");auto colon=fallback.find(":");if(variant!=std::string::npos&&colon!=std::string::npos)fallback.erase(variant,colon-variant);it=nightCrops.find(fallback);}
  if(it==nightCrops.end())return;''')
# 0.8 artwork scale introduced earlier also applies to crop coordinates and sizes.
edit('src/lincity-ng/GameView.cpp','Vector2 p=rect.p1+Vector2(layer.x*zoom,layer.y*zoom);','''const float sx=rect.getWidth()/384.f;
    const float sy=sx;
    Vector2 p=rect.p1+Vector2(layer.x*sx,layer.y*sy);''')
# Do not impose a fixed 384 width on traffic/nonbuilding crops; use existing art scale.
edit('src/lincity-ng/GameView.cpp','const float sx=rect.getWidth()/384.f;','const float sx=key.rfind("Traffic:",0)==0?zoom:zoom*.8f;')
edit('src/lincity-ng/GameView.cpp','p.x+tx->getWidth()*zoom,p.y+tx->getHeight()*zoom','p.x+tx->getWidth()*sx,p.y+tx->getHeight()*sy')
# No additional bitmap assets: street lamps are drawn procedurally beside town roads.
edit('src/lincity-ng/GameView.hpp','  void drawStreetActors(', '  void drawStreetLamps(Painter&, MapPoint);\n  void drawStreetActors(')
edit('src/lincity-ng/GameView.cpp','          drawStreetActors(painter, tile);','''          { drawStreetActors(painter, tile); drawStreetLamps(painter,tile); }''')
p=s/'src/lincity-ng/GameView.cpp';t=p.read_text();t+='''
void GameView::drawStreetLamps(Painter& painter,MapPoint tile) {
 auto& w=getWorld();float night=w.evolution.darkness()/1000.f;
 if(night<=.05f||(tile.x+tile.y)%4)return;
 bool settled=false;for(int dy=-3;dy<=3&&!settled;++dy)for(int dx=-3;dx<=3;++dx){MapPoint q(tile.x+dx,tile.y+dy);if(!w.map.is_visible(q))continue;auto*c=w.map(q)->reportingConstruction;if(c&&!(c->flags&FLAG_IS_TRANSPORT)){settled=true;break;}}
 if(!settled)return;
 auto base=getScreenPoint(tile)+Vector2(tileWidth*.25f,-tileHeight*.55f);
 auto lamp=base-Vector2(0,19*zoom);
 for(int radius=16;radius>=4;radius-=4){Vector2 points[12];for(int k=0;k<12;++k){float angle=k*6.2831853f/12;points[k]=base+Vector2(std::cos(angle)*radius*zoom,std::sin(angle)*radius*zoom*.45f);}painter.setFillColor(Color(255,190,85,int(night*12)));painter.fillPolygon(12,points);}
 painter.setLineColor(Color(76,64,47,255));painter.drawLine(base,lamp);
 painter.setFillColor(Color(255,205,115,int(230*night)));painter.fillRectangle(Rect2D(lamp.x-2*zoom,lamp.y-2*zoom,lamp.x+2*zoom,lamp.y+3*zoom));
}
''';t=t.replace('#include "NightCrops.hpp"','#include "NightCrops.hpp"\n#include <cmath>');p.write_text(t)
print('Applied management pause, HUD bounds, focus feedback, tax advisor and brighter night/street lamps')
