# XML / string-lifetime stress check

This supplemental native test repeatedly parses configuration and menu XML
using the rebuilt libxml++ wrapper and header-only fmt. On Linux, its CMake
target enables AddressSanitizer. It does not exercise Windows graphics, SDL,
the entire game startup, Defender, or the original failed binary.

With a native C++ compiler, CMake and libxml2 development package installed:

```sh
cmake -S tests/startup-xml -B build-xml-stress -DCMAKE_BUILD_TYPE=Debug
cmake --build build-xml-stress --parallel 4
ASAN_OPTIONS=detect_leaks=0 ./build-xml-stress/startup_xml_stress \
  data/gui/mainmenu.xml data/gui/newgame.xml
```

Leak scanning is not included in the reported preparation check. Address
checking remains enabled. Passing this check is not proof that the reported
Windows crash is fixed.
