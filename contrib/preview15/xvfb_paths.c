#include <dlfcn.h>
#include <stdlib.h>
__attribute__((constructor)) static void set_path(void){char**p=dlsym(RTLD_DEFAULT,"XkbBinDirectory");const char*v=getenv("CITY_TEST_XKB_BIN");if(p&&v)*p=(char*)v;}
