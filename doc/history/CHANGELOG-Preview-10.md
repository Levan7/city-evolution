# City Evolution — Preview 10
4 November 2026

This revision rebuilds the construction sidebar, restores the original main-menu scene, and expands the consistent isometric artwork. It is a data/UI revision for the existing Preview 9 native engine. It is a fork of LinCity-NG.

## What changed in this revision

| Area | Result |
| --- | --- |
| Main menu | Original LinCity-NG city background restored. Only its title/logo group is replaced with City Evolution branding. Fixed spacing separates title, seven buttons and footer. |
| Construction sidebar | 15 compact categories on the left; one drawer opens at a time. All 51 original tools retain their actions, construction group IDs and help links. |
| Public transport | Separate Buses, Rail, Metro and Elevated drawers, plus Streets and Freight. The passenger report is an icon button on the left. Mayor stays at the top right. |
| Building choices | 45 choices show the actual in-game building, road or rail sprite. Metro/elevated track tools retain their route-layer diagrams; the four action tools retain action symbols. Categories use simple symbols. |
| Speed controls | Two rows of speed controls under the category rail, followed by City Hall and Menu / Save buttons. |
| Artwork | 22 new or revised sprite sheets: 21 building/facility/landscape sets and a 16-view native car. Combined with retained Preview 9 art, 43 distinct building/facility models have replacement art bindings. |
| Working buildings | Pottery, blacksmith, mill, commune, ore mine, heavy industry, wind turbine, fire station, school and sports use multiple new poses in their original native frame slots. |
| Effects and stages | Original gray/black smoke restored (10/11 frames), five native farm states, landfill fill levels, monument construction/finished states, and eight rocket states. |
| Old images | 157 unused replaced legacy/revision images removed from the playable package. Reproducible input assets remain in source. Each active resource has a single ordered binding. |
| Startup styles | Main menu explicitly defines boxtext and dialogbutton before importing the Load/Save screens, addressing the previously identified undefined-style startup failure. |
| Packaging | Complete portable game and complete source archives, with hash manifests and a manifest-based diagnostic script. |

New/revised art sets: pottery, blacksmith, mill, commune, ore mine, coal mine, landfill, harbor, sports field, monument, rocket complex, park, shanty, wind turbine, heavy factory, fire station, farm, school, park with lake, light factory, coal plant, native car.

## Animation details

Native timing code and frame counts have not been changed. Most working-building loops now repeat three distinct newly painted poses across their original slots. This preserves activity and timing, but is not a frame-for-frame repaint of every distinct upstream pose. School uses a shared full-building pivot for its animated overlay. A common canvas and pivot keep each building stable between poses. Native car frames preserve the original directional/slope order and per-frame placement. Light factory and coal-plant art follow the original geometry so their original smoke emitters remain usable.

The actual checked frame counts and distinct image counts are recorded in validation/preview10-assets.json. This is more precise than calling every new model fully animated.

## Compared with the Claude source you supplied

Baseline: city-evolution-Preview-8-source.tar(1).xz, extracted root city-evolution-Preview-8-source. The source package contains Changes-from-Claude.patch for textual engine/UI/build/script changes and validation/changes-from-claude.json for the complete path-by-path SHA-256 comparison, including binary assets. The patch requires the new binary artwork from this source package; it is not a standalone game update.

The mayor/advisor, district, world-generation and broader city systems already present in the supplied archive remain inherited work. This changelog does not claim they were created in Preview 10.

The earlier Preview 9 changes, retained here, include:

- Four passenger networks: bus, rail, underground metro and elevated transit; five station/stop facilities and two transit track construction types.
- Network reach, passengers and fares, residential transit desirability, save/load state and passenger report.
- Moving route vehicles and drawing of the underground/elevated layers.
- Expanded building art, transport sprites, readability changes, revised world/menu data and a stocked transport sandbox.
- Explicit integer casts in mayor-plan handling, permission checks for delegated park/service actions, and startup markers.

Native files changed relative to that supplied archive are listed below. Preview 10 retains these native implementations; this menu/art revision does not rebuild or alter the Windows executable.

- src/lincity/CMakeLists.txt
- src/lincity/city_evolution.cpp
- src/lincity/groups.hpp
- src/lincity/mayor_plan.cpp
- src/lincity/modules/CMakeLists.txt
- src/lincity/modules/all_modules.cpp
- src/lincity/modules/all_modules.hpp
- src/lincity/modules/public_transport.cpp
- src/lincity/modules/public_transport.hpp
- src/lincity/modules/residence.cpp
- src/lincity/simulate.cpp
- src/lincity/stats.cpp
- src/lincity/stats.hpp
- src/lincity/transit.cpp
- src/lincity/transit.hpp
- src/lincity/world.hpp
- src/lincity/xmlloadsave.cpp
- src/lincity-ng/Game.cpp
- src/lincity-ng/Game.hpp
- src/lincity-ng/GameView.cpp
- src/lincity-ng/GameView.hpp
- src/lincity-ng/MainMenu.cpp
- src/lincity-ng/Mps.cpp
- src/lincity-ng/UserOperation.cpp
- src/lincity-ng/main.cpp

## What is still incomplete or unverified

- Terrain, road/rail/powerline networks, bridges and disaster effects retain upstream graphics. This is not a replacement of every image in the engine.
- Horse, horse cart and delivery truck remain decorative. They are not independent simulated logistics systems.
- The passenger system is the inherited Preview 9 implementation. This revision reorganizes its controls and changes its facility artwork; it does not add a new simulation engine.
- The original native animation slot counts remain, with three-pose loops for many buildings as described above.
- The Windows window caption can still say Preview 9 because the native executable is retained. The main menu footer and package identify the data/UI revision as Preview 10.
- The provided screenshots for the latest menu complaint were not attached in this request; this revision follows the written requirements.
- Windows startup and interactive play have not been verified in this environment. Static GUI/import/resource/layout checks passed, but do not prove crash-free runtime behavior. The prior undefined-style failure is specifically addressed; unrelated runtime faults remain possible.
- Existing Preview 9 simulation logs included in the source are historical evidence. Those simulations were not rerun for this asset/UI revision.

## Verification and use

The checks preserve all 51 tool definitions, verify all native resource frame counts, check referenced images and the main-menu style import order, verify 40 revised table layouts for overlapping cells, and check sidebar/menu space budgets at 1024×720, 1280×800, 1366×768 and 1920×1080. The archive builder checks ZIP CRCs and every packaged SHA-256 entry. Windows interactive testing remains outstanding.

Extract the entire Windows ZIP into a new writable folder and run Play.cmd. Do not place the update over a mixture of older game directories. Continue city opens the bundled transport example; New world creates a fresh city. Personal saves can be copied into PortableData after extraction, with backups kept separately.

If startup fails, run Diagnose-Startup.cmd beside Play.cmd and provide its output together with PortableData/launch.log.

The source includes the complete engine, game data, final PNG masters, available generation prompts, exported frames/pivots, resource and thumbnail mappings, an art contact sheet, reproduction scripts and validation reports. Consult PREVIEW-9.md for the retained engine build instructions and dependencies.
