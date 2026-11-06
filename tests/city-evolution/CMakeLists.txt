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
