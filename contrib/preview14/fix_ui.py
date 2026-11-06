from pathlib import Path
s=Path('preview14-work/CityEvolution-Preview-14-Source')
p=s/'src/lincity-ng/main.hpp';t=p.read_text().replace('#include <string>','#include <string>\n#include <SDL3/SDL.h>\n#include "UiScale.hpp"');t=t.replace('void videoSizeChanged(int width, int height);','UiGeometry resizeUserInterface();\nvoid mapUserInterfaceEvent(SDL_Event& event);\nvoid videoSizeChanged(int width, int height);');p.write_text(t)
p=s/'src/lincity-ng/main.cpp';t=p.read_text();idx=t.index('void videoSizeChanged(');t=t[:idx]+'''UiGeometry resizeUserInterface() {
 int width=0,height=0;
 if(window_renderer)SDL_GetRenderOutputSize(window_renderer,&width,&height);
 else SDL_GetWindowSize(window,&width,&height);
 auto ui=calculateUiGeometry(width,height);
 if(window_renderer)SDL_SetRenderLogicalPresentation(window_renderer,ui.width,ui.height,SDL_LOGICAL_PRESENTATION_STRETCH);
 return ui;
}
void mapUserInterfaceEvent(SDL_Event& event) {
 if(window_renderer)SDL_ConvertEventToRenderCoordinates(window_renderer,&event);
}

'''+t[idx:];t=t.replace('    fontManager = new FontManager();','    auto ui=resizeUserInterface();\n    std::cerr<<"[video] logical UI "<<ui.width<<"x"<<ui.height<<"; scale "<<ui.scale<<"\\n";\n    fontManager = new FontManager();');p.write_text(t)
for name in ['Game.cpp','MainMenu.cpp']:
 p=s/'src/lincity-ng'/name;t=p.read_text();t=t.replace('            switch(event.type) {','            mapUserInterfaceEvent(event);\n            switch(event.type) {');t=t.replace('gui->resize(width, height);','{auto ui=resizeUserInterface();gui->resize(ui.width,ui.height);}') if name=='Game.cpp' else t.replace('menu->resize(width, height);','{auto ui=resizeUserInterface();menu->resize(ui.width,ui.height);}')
 t=t.replace('gui->resize(getConfig()->videoX.get(), getConfig()->videoY.get());','{auto ui=resizeUserInterface();gui->resize(ui.width,ui.height);}')
 for obj in ['gui','menu']:t=t.replace(obj+'->resize(event.window.data1, event.window.data2);','{auto ui=resizeUserInterface();'+obj+'->resize(ui.width,ui.height);}')
 p.write_text(t)
p=s/'src/lincity-ng/GameView.cpp';t=p.read_text().replace('b->setCaptionText(hideHigh?"Models hidden - click to show":"Building models visible")','b->setCaptionText(hideHigh?"Buildings hidden - show":"Buildings visible - hide")');t=t.replace('if(name=="ShowBuildingsButton"){hideHigh=false;mapOverlay=overlayNone;','if(name=="ShowBuildingsButton"){hideHigh=!hideHigh;mapOverlay=overlayNone;');p.write_text(t)
