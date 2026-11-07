#include <SDL3/SDL.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
static SDL_Renderer* render;static Uint64 start;static int phase,captured=-1;
static void click(float x,float y,int button){float wx,wy;SDL_RenderCoordinatesToWindow(render,x,y,&wx,&wy);SDL_Window*w=SDL_GetRenderWindow(render);SDL_Event e={0};e.type=SDL_EVENT_MOUSE_MOTION;e.motion.windowID=SDL_GetWindowID(w);e.motion.x=wx;e.motion.y=wy;SDL_PushEvent(&e);e.type=SDL_EVENT_MOUSE_BUTTON_DOWN;e.button.windowID=SDL_GetWindowID(w);e.button.button=button;e.button.x=wx;e.button.y=wy;e.button.down=true;SDL_PushEvent(&e);e.type=SDL_EVENT_MOUSE_BUTTON_UP;e.button.down=false;SDL_PushEvent(&e);}
static void key(SDL_Keycode code){SDL_Event e={0};e.type=SDL_EVENT_KEY_DOWN;e.key.windowID=SDL_GetWindowID(SDL_GetRenderWindow(render));e.key.key=code;e.key.down=true;SDL_PushEvent(&e);e.type=SDL_EVENT_KEY_UP;e.key.down=false;SDL_PushEvent(&e);}
static void shot(const char*name){const char*d=getenv("CITY_SMOKE_SCREEN_DIR");if(!d)return;char p[1024];snprintf(p,sizeof(p),"%s/%s.bmp",d,name);SDL_Surface*s=SDL_RenderReadPixels(render,NULL);if(s){SDL_SaveBMP(s,p);SDL_DestroySurface(s);fprintf(stderr,"[smoke] captured %s\n",name);}}
bool SDL_RenderPresent(SDL_Renderer*r){static bool(*real)(SDL_Renderer*);if(!real)real=dlsym(RTLD_NEXT,"SDL_RenderPresent");render=r;if(!start)start=SDL_GetTicks();if(captured!=phase){const char*names[]={"main","city-hud","budget-before","budget-after","budget-held","advisor-opening","advisor","finished","unused","finished"};shot(names[phase<10?phase:9]);captured=phase;}return real(r);}
bool SDL_WaitEventTimeout(SDL_Event*event,Sint32 timeout){static bool(*real)(SDL_Event*,Sint32);if(!real)real=dlsym(RTLD_NEXT,"SDL_WaitEventTimeout");if(render){Uint64 elapsed=SDL_GetTicks()-start;int w,h;SDL_RendererLogicalPresentation mode;SDL_GetRenderLogicalPresentation(render,&w,&h,&mode);if(elapsed>(Uint64)(phase+1)*900){switch(phase){case 0:click(204,h*.5f-124,1);break;case 1:key(SDLK_F7);break;case 2:click(w*.5f-180,h*.5f-85,1);break;
case 3:break;
case 4:click(w*.5f+180,h*.5f+181,1);break;
case 5:break;
default:{SDL_Event e={0};e.type=SDL_EVENT_QUIT;SDL_PushEvent(&e);break;}}phase++;}}
return real(event,timeout>50?50:timeout);}
