from pathlib import Path
from PIL import Image
import json,zipfile,xml.etree.ElementTree as E
s=Path('preview13-work/CityEvolution-Preview-13-Source')
def edit(f,a,b):
 p=s/f;t=p.read_text();assert a in t,(f,a);p.write_text(t.replace(a,b))
def button(name,caption,width=320):return f'<Button name="{name}" width="{width}" height="34"><image src="images/readable/plate-320-normal.png" resizable="yes"/><image-hover src="images/readable/plate-320-hover.png" resizable="yes"/><text-caption color="#ffffff" halign="center" font-size="16">{caption}</text-caption></Button>'
def cell(row,col,body,span=1):return f'<cell row="{row}" col="{col}" colspan="{span}" halign="center" valign="center">{body}</cell>'
pairs=[('TaxDown','Income tax -','TaxUp','Income tax +'),('AutoTax','Mayor taxes: OFF','AutoBudget','Mayor budgets: OFF'),('EducationBudget','Education: 100%','HealthBudget','Healthcare: 100%'),('FireBudget','Fire service: 100%','PoliceBudget','Police: 100%'),('SocialDown','Social support -','SocialUp','Social support +'),('ZombieToggle','Zombie Easter egg: ON','TaxCeiling','Tax ceiling: 15%'),('ReserveDown','Cash reserve -','ReserveUp','Cash reserve +'),('BusinessDown','Business tax -','BusinessUp','Business tax +')]
page='<Panel name="BudgetPage" resizable="yes"><TableLayout rows="9" cols="2">'+''.join('<rowsize row="'+str(i)+'" fixed="38"/>' for i in range(1,10))
for i,(a,b,c,d) in enumerate(pairs,1):page+=cell(i,1,button(a,b))+cell(i,2,button(c,d))
page+=cell(9,1,'<Paragraph color="#edf0df" font-size="15">Mayor automation requires the mayor to be ON.</Paragraph>',2)+'</TableLayout></Panel>'
assets=json.loads((s/'art/preview13/integration/skyscraper-assets.json').read_text())['assets'];ids=[('BuildPolice','Police station - $8,000','images/emergency/police-station.png')]+[(n,o['label']+' - $'+('60,000' if i<3 else '85,000'),o['file']) for i,(n,o) in enumerate(zip(['BuildIvory','BuildBrick','BuildSand','BuildGlass','BuildBronze','BuildDeco'],assets))]
(s/'data/images/catalog').mkdir(exist_ok=True)
page2='<Panel name="DevelopmentPage" resizable="yes"><TableLayout rows="4" cols="4"><colsize col="1" fixed="80"/><colsize col="3" fixed="80"/>'
for i,(name,caption,file) in enumerate(ids):
 im=Image.open(s/'data'/file).convert('RGBA');im.thumbnail((68,68),Image.Resampling.LANCZOS);canvas=Image.new('RGBA',(70,70));canvas.alpha_composite(im,((70-im.width)//2,(70-im.height)//2));canvas.save(s/'data/images/catalog'/(name+'.png'))
 row=i//2+1;col=i%2*2+1;page2+=cell(row,col,f'<Image src="images/catalog/{name}.png"/>')+cell(row,col+1,button(name,caption,245))
page2+='</TableLayout></Panel>'
text='<?xml version="1.0"?>\n<?xml-stylesheet type="text/xsl" href="identity.xsl"?>\n<Window name="PublicServices" width="760" height="600" border="12" titlesize="30" resizable="no"><title><Paragraph color="#ffffff" font-size="21">BUDGET / PUBLIC SERVICES</Paragraph></title><background><FilledRectangle color="#14252dff"/></background><title-background><FilledRectangle color="#29463fff"/></title-background><closebutton><text color="white">X</text></closebutton><contents><TableLayout rows="6" cols="2"><rowsize row="1" fixed="38"/><rowsize row="2" fixed="38"/><rowsize row="3" fixed="40"/><rowsize row="5" fixed="42"/><rowsize row="6" fixed="40"/>'
text+=cell(1,1,'<Paragraph name="ServiceSummary" color="#edf0df" font-size="16">Budget</Paragraph>',2)+cell(2,1,'<Paragraph name="ServiceHealth" color="#edf0df" font-size="16">Health</Paragraph>',2)+cell(3,1,button('BudgetTab','Taxes and funding'))+cell(3,2,button('DevelopmentTab','Police and towers'))+cell(4,1,'<SwitchComponent name="ServicePages">'+page+page2+'</SwitchComponent>',2)+cell(5,1,'<Paragraph name="ServiceNote" color="#edf0df" font-size="14">Roads speed up crews; off-road travel is slower. Water needs a crossing.</Paragraph>',2)+cell(6,1,button('ServicesClose','Back to my city'),2)+'</TableLayout></contents></Window>\n';(s/'data/gui/publicservices.xml').write_text(text)
edit('data/gui/CMakeLists.txt','  cityhall.xml','  publicservices.xml\n  cityhall.xml')
p=s/'data/gui/cityhall.xml';t=p.read_text();t=t.replace('<cell row="6" col="1" colspan="5"','<cell row="6" col="1" colspan="3"');pos=t.rfind('    </TableLayout>');t=t[:pos]+cell(6,4,button('CityServices','Budget / public services',280),2)+t[pos:];p.write_text(t)
edit('src/lincity-ng/Game.hpp','void showTransport();','void showTransport();\n    void showPublicServices();')
edit('src/lincity-ng/Game.cpp','getButton(*w,"CityClose")->clicked.connect','getButton(*w,"CityServices")->clicked.connect([this,close](Button*) { close();showPublicServices(); });\n  getButton(*w,"CityClose")->clicked.connect')
edit('src/lincity-ng/Game.cpp','if(gui_event.key == SDLK_F6) showCityHall();','if(gui_event.key == SDLK_F6) showCityHall();\n                  if(gui_event.key == SDLK_F7) showPublicServices();')
edit('src/lincity-ng/Game.cpp','#include "Game.hpp"','#include "Game.hpp"\n#include "lincity/modules/civic_buildings.hpp"')
p=s/'src/lincity-ng/Game.cpp';p.write_text(p.read_text()+'''
void Game::showPublicServices() {
  if(gui->findComponent("PublicServices"))return;
  auto panel=dynamic_unique_cast<Window>(loadGUIFile("gui/publicservices.xml"));auto* w=panel.get();
  auto refresh=[this,w](){auto& c=world->civic;auto& r=world->money_rates;
    getParagraph(*w,"ServiceSummary")->setText("Income tax "+std::to_string(r.income_tax)+"% | Business "+std::to_string(r.goods_tax)+"% | Reserve $"+std::to_string(c.reserve)+" | Crews "+std::to_string(c.units.size()));
    getParagraph(*w,"ServiceHealth")->setText("Pollution illness: "+std::to_string(c.sickTotal)+" | Education "+std::to_string(c.education/10)+"% | Incident risk "+std::to_string(c.crime/10)+"% | Social "+std::to_string(r.dole));
    const char* names[]={"EducationBudget","HealthBudget","FireBudget","PoliceBudget"};const char* captions[]={"Education","Healthcare","Fire service","Police"};for(int i=0;i<4;++i)getButton(*w,names[i])->setCaptionText(std::string(captions[i])+": "+std::to_string(c.budget[i])+"%");
    getButton(*w,"AutoTax")->setCaptionText(c.autoTax?"Mayor taxes: ON":"Mayor taxes: OFF");getButton(*w,"AutoBudget")->setCaptionText(c.autoBudget?"Mayor budgets: ON":"Mayor budgets: OFF");getButton(*w,"ZombieToggle")->setCaptionText(c.zombiesEnabled?"Zombie Easter egg: ON":"Zombie Easter egg: OFF");getButton(*w,"TaxCeiling")->setCaptionText("Tax ceiling: "+std::to_string(c.taxCeiling)+"%");
    getGameView().setMapDirty();
  };
  auto bind=[w,refresh](const char* name,std::function<void()> fn){getButton(*w,name)->clicked.connect([fn,refresh](Button*){fn();refresh();});};
  bind("TaxDown",[this](){world->money_rates.income_tax=std::max(0,world->money_rates.income_tax-1);});bind("TaxUp",[this](){world->money_rates.income_tax=std::min(30,world->money_rates.income_tax+1);});
  bind("BusinessDown",[this](){world->money_rates.goods_tax=std::max(0,world->money_rates.goods_tax-1);});bind("BusinessUp",[this](){world->money_rates.goods_tax=std::min(20,world->money_rates.goods_tax+1);});
  bind("SocialDown",[this](){world->money_rates.dole=std::max(0,world->money_rates.dole-5);});bind("SocialUp",[this](){world->money_rates.dole=std::min(100,world->money_rates.dole+5);});
  bind("AutoTax",[this](){world->civic.autoTax=!world->civic.autoTax;});bind("AutoBudget",[this](){world->civic.autoBudget=!world->civic.autoBudget;});
  bind("ZombieToggle",[this](){world->civic.zombiesEnabled=!world->civic.zombiesEnabled;if(!world->civic.zombiesEnabled){auto& calls=world->civic.incidents;calls.erase(std::remove_if(calls.begin(),calls.end(),[](auto& c){return c.kind==CivicServices::ZOMBIE_CALL;}),calls.end());world->civic.zombieNoticePending=false;}});
  bind("TaxCeiling",[this](){world->civic.taxCeiling=world->civic.taxCeiling>=30?10:world->civic.taxCeiling+5;});
  bind("ReserveDown",[this](){world->civic.reserve=std::max(25000,world->civic.reserve-25000);});bind("ReserveUp",[this](){world->civic.reserve=std::min(1000000,world->civic.reserve+25000);});
  const char* names[]={"EducationBudget","HealthBudget","FireBudget","PoliceBudget"};for(int i=0;i<4;++i)bind(names[i],[this,i](){int& b=world->civic.budget[i];b=b<50?50:b<75?75:b<100?100:b<125?125:b<150?150:0;});
  getButton(*w,"BudgetTab")->clicked.connect([w](Button*){dynamic_cast<SwitchComponent*>(w->findComponent("ServicePages"))->switchComponent("BudgetPage");});getButton(*w,"DevelopmentTab")->clicked.connect([w](Button*){dynamic_cast<SwitchComponent*>(w->findComponent("ServicePages"))->switchComponent("DevelopmentPage");});
  const std::pair<const char*,ConstructionGroup*> buildings[]={{"BuildPolice",&policeGroup},{"BuildIvory",&ivoryTowerGroup},{"BuildBrick",&brickTowerGroup},{"BuildSand",&sandTowerGroup},{"BuildGlass",&glassOfficeGroup},{"BuildBronze",&bronzeOfficeGroup},{"BuildDeco",&decoOfficeGroup}};
  for(auto [name,group]:buildings)getButton(*w,name)->clicked.connect([this,w,group](Button*){Message::ptr message;if(!group->can_build(*world,message)){getParagraph(*w,"ServiceNote")->setText("This building needs more money or technology (towers require 60%).");return;}UserOperation op;op.action=UserOperation::ACTION_BUILD;op.constructionGroup=group;setUserOperation(op);windowmanager->removeWindow(w);getGameView().setDirty();});
  getButton(*w,"ServicesClose")->clicked.connect([this,w](Button*){windowmanager->removeWindow(w);});windowmanager->addWindow(std::move(panel));refresh();
}
''')
# Original supplied developer logo, at native display dimensions.
with zipfile.ZipFile('preview13-work/input/biscuit-maker-games-firebase-site-v2.zip') as z:
 file=next(x for x in z.namelist() if x.endswith('/biscuit-maker-games-logo.png'));data=z.read(file)
 import io
 im=Image.open(io.BytesIO(data));im.resize((280,153),Image.Resampling.LANCZOS).save(s/'data/images/biscuit-maker-games-logo.png');(s/'art/preview13/biscuit-maker-games-logo-original.png').write_bytes(data)
p=s/'data/gui/creditslist.xml';t=p.read_text();idx=t.index('>',t.index('<Document'))+1;t=t[:idx]+'\n<img src="images/biscuit-maker-games-logo.png"/>\n<p>City Evolution - Biscuit Maker Games</p>\n<p>A fork of LinCity-NG. Original project credits and licenses follow.</p>\n'+t[idx:];p.write_text(t)
