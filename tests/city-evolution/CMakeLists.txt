# Real engine integration tests; display and audio adapters are deliberately inert.
add_executable(city-evolution-checks ../city_evolution_integration.cpp headless_adapters.cpp)
target_link_libraries(city-evolution-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(city-evolution-checks PRIVATE ../../src)

add_executable(mayor-checks ../mayor_integration.cpp headless_adapters.cpp)
target_link_libraries(mayor-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(mayor-checks PRIVATE ../../src)

add_executable(transit-checks ../transit_integration.cpp headless_adapters.cpp)
target_link_libraries(transit-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(transit-checks PRIVATE ../../src)

add_executable(community-checks ../community_integration.cpp headless_adapters.cpp)
target_link_libraries(community-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(community-checks PRIVATE ../../src)

add_executable(services-checks ../services_integration.cpp headless_adapters.cpp)
target_link_libraries(services-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(services-checks PRIVATE ../../src)

add_executable(flu-checks ../flu_integration.cpp headless_adapters.cpp)
target_link_libraries(flu-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(flu-checks PRIVATE ../../src)

add_executable(audio-checks ../audio_integration.cpp audio_adapters.cpp ../../src/lincity-ng/Sound.cpp ../../src/lincity-ng/Config.cpp)
target_link_libraries(audio-checks PRIVATE LinCity::lib LinCity::gui SDL3_mixer::SDL3_mixer fmt::fmt cfgpath gettextutil xmlutil ptrutil)
target_include_directories(audio-checks PRIVATE ../../src ../../src/lincity-ng)

target_include_directories(audio-checks PRIVATE ${CMAKE_BINARY_DIR}/src)

add_executable(autonomy-checks ../autonomy_integration.cpp headless_adapters.cpp)
target_link_libraries(autonomy-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(autonomy-checks PRIVATE ../../src)
add_executable(street-routes-checks ../street_routes_integration.cpp headless_adapters.cpp)
target_link_libraries(street-routes-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(street-routes-checks PRIVATE ../../src)
add_executable(finance-checks ../finance_integration.cpp headless_adapters.cpp)
target_link_libraries(finance-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(finance-checks PRIVATE ../../src)
add_executable(network-checks ../network_integration.cpp headless_adapters.cpp)
target_link_libraries(network-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(network-checks PRIVATE ../../src)

# Info panels must never ask for more lines than Mps::PARAGRAPH_COUNT.
add_executable(panel-capacity-checks ../panel_capacity_integration.cpp headless_adapters.cpp)
target_link_libraries(panel-capacity-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(panel-capacity-checks PRIVATE ../../src)

# Damaged saves must give a readable load error, not terminate the game.
add_executable(save-robustness-checks ../save_robustness_integration.cpp headless_adapters.cpp)
target_link_libraries(save-robustness-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(save-robustness-checks PRIVATE ../../src)
target_compile_definitions(save-robustness-checks PRIVATE FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../fixtures")

# Port import pricing: 64-bit arithmetic, unchanged rounding.
add_executable(import-math-checks ../import_math_integration.cpp headless_adapters.cpp)
target_link_libraries(import-math-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(import-math-checks PRIVATE ../../src)

# Crash logging (uses fork and POSIX signals, so not built on Windows).
if(UNIX)
  add_executable(crashlog-checks ../crashlog_check.cpp ../../src/lincity-ng/CrashLog.cpp)
  target_include_directories(crashlog-checks PRIVATE ../../src)
endif()

add_executable(airport-checks ../airport_integration.cpp headless_adapters.cpp)
target_link_libraries(airport-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(airport-checks PRIVATE ../../src)

add_executable(river-industries-checks ../river_industries_integration.cpp headless_adapters.cpp)
target_link_libraries(river-industries-checks PRIVATE LinCity::lib fmt::fmt)
target_include_directories(river-industries-checks PRIVATE ../../src)
