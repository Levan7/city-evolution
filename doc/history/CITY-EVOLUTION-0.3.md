# City Evolution 0.3.0 — first integrated original-engine upgrade

Windows x64 portable candidate based on LinCity-NG 2.15.0. The previous expanded-world maps and empty starts are retained. This release starts the broader upgrade programme; it does not finish the entire design.

## Launch and controls

Extract to a new folder, run `Play.cmd`, and open **City hall** beside the speed controls (or press F6). City hall contains new building tools, approval/elections, news, atmosphere controls and advisors. Click a building, then click a clear site in the city. Normal building rules and construction charges apply.

To inspect the new content immediately, open Load Game, select `1_CityEvolution-Showcase.scn.gz` (the showcase is included in PortableData). It is a funded sandbox with seeded stocks, advanced technology, roads, rail, the four new buildings, schools and a university. Its first election is deliberately scheduled after 300 days; standard cities use four-year terms. It is a demonstration town, not a balanced or self-sufficient city.

The default sky starts with twelve minutes of daylight. Later daylight periods last 8–16 active real minutes; nights last 3–6 minutes; dusk/dawn take 45 seconds each. Simulation speed does not shorten these periods. Pausing stops this clock. Sky controls cycle through occasional nights, always day and always night. Weather cycles through changing weather, clear, cloudy, rain and storm. The current selection appears in City hall. Rain, storm flashes, darkening and powered-building lights are rendered within the world view. Storms have occasional low-opacity lightning flashes.

Cars follow roads/tracks; pedestrians walk on road/track edges. These visible agents are decorative. Real goods continue to move through the engine's commodity inventories and connections. Cars use the existing Options car toggle; pedestrians have their own City hall toggle. Actors are bounded in number and restricted to the visible street network. Buildings in front of a street occlude its traffic.

## Working gameplay additions

| Building | Build cost | Unlock/demand | Operation and effects |
| --- | ---: | --- | --- |
| Freight warehouse, 2×2 | $6,000 | Available immediately | $6/day upkeep; 20,000 capacity each for food, coal, goods, ore and steel. Connect rail and road to its edges to transfer freight through the stock buffer. No free resources are generated. |
| Technology campus, 3×3 | $35,000 | 25% technology, at least one university | Per operating day: consumes 8 labor, 4 steel, 20 low-voltage power and 2 water; makes 60 goods, 2 waste and 2 technology points. Stops for missing inputs, university removal or full outputs. |
| Neighborhood café, 1×1 | $3,000 | 2% technology; four housing buildings per café | Per operating day: consumes 4 labor, 10 food, 2 goods, 3 water and 1 low-voltage power; makes 1 waste and pays $12 to the city. |
| Food hall / bar, 2×2 | $8,000 | 15% technology; eight housing buildings per venue | Per operating day: consumes 8 labor, 20 food, 8 goods, 6 water and 5 low-voltage power; makes 3 waste and pays $22 to the city. |

Café/food-hall demand is currently measured by housing-building counts. Empty or undersupplied housing is not a full consumer-demand simulation. Both venues also contribute to monthly amenity/approval calculations when operating. Existing schools, universities, farms, factories, markets, housing, ports and transport retain their original functions. Each new building has a distinct generated isometric sprite; the original art elsewhere is retained. Sprite paths, generation method and prompts are in `art/EVOLUTION-GRAPHICS.md`.

The existing power grid now receives weather/daylight-dependent solar output: clear daylight 100%, clouds 70%, rain 45%, storm 25%; full night 0%, with dawn/dusk blending. Existing coal/wind, substations, wiring and commodity transport remain available. Stored power runs down normally. New businesses must have electricity to operate. Advisors report unpowered housing, depleted business inputs and freight access with coordinates. Rooftop panels on the warehouse sprite are decorative.

## Elections and news

Approval is assessed monthly from fed/employed housing, powered housing, local pollution, operating amenities and debt. Elections occur every four simulation years and preserve their results in a bounded news history. The council endorses or criticizes the programme according to approval. **You always remain City Steward and retain construction control.** There is no forced loss or mayor takeover. City hall shows monthly business activity, jobs represented by input labor, amenities and powered housing. These are initial civic systems, not a complete political simulation. Dashboard values are monthly snapshots.

## Saves and compatibility

The game reads original 2.15.0 scenarios and earlier Expanded Worlds saves. New saves use format 2141 so older builds reject them safely. Keep an untouched backup before migrating a city; a city saved in this release cannot be loaded by 0.2.x or unmodified 2.15.0. Sky/weather progress, choices, election state, news, dashboard snapshots and building inventories are saved. Decorative street actors are recreated on load.

Settings and cities stay beside Play.cmd in PortableData. Back up your previous PortableData before copying cities. Do not overwrite its only copy. No previous settings or saved cities are required to test the supplied showcase.

`Runtime-Check.cmd` checks configuration/runtime startup. `Verify-Worlds.cmd` checks world generation and save/load using the game executable. `Verify-Evolution.cmd` runs the standalone simulation integration diagnostic, including long-clock bounds, real resource consumption, blackout behavior, rail-to-road freight, elections, new saves, all map presets and original scenarios. It writes only inside PortableData/EvolutionChecks and a diagnostic log.

The reported 0xC0000374 startup crash is not confirmed resolved on the user's PC. The static C++ dependency/runtime compatibility changes from 0.2.1 are retained. If launch fails, PortableData/launch.log records startup stages. Windows execution is not available in the build environment; cross-compilation and native Linux engine tests are not a substitute for Windows launch/render testing.

## Validation

The actual simulation core passed AddressSanitizer/UndefinedBehaviorSanitizer checks on Linux with display/audio/config adapters (leak scanning and vptr checks disabled). This included 168,498 clock/resource/demand/civic/save checks, 19 generated worlds up to 400×400, all six original scenarios and showcase save/reload. The diagnostic source and inert adapters are included under tests. The Windows main executable and standalone diagnostic compile with the same vendored C++ XML dependency and static compiler runtime. GUI XML structure, resource files and imported DLL symbols are checked separately. See the packaged VALIDATION.txt for final build evidence and limitations.

Build diagnostic: configure with `-DLINCITYNG_EVOLUTION_CHECKS=ON`, then build target `city-evolution-checks`. Arguments: writable temporary save filename, optional original-scenario directory, optional showcase output filename. Never use an existing city as the temporary filename.

## Remaining upgrade work

| Area | Current state / remaining work |
| --- | --- |
| New graphics | Four new building sprites plus atmosphere and street agents. A full terrain/building/vehicle-art replacement is still pending. |
| Traffic and logistics | Visible cars/pedestrians and a functioning freight warehouse. Dedicated truck dispatch, passenger destinations and congestion simulation are pending. |
| Municipalities and mayors | City Steward, approval and council election reports implemented. Municipal boundaries, competing mayors and autonomous planning are pending. |
| Economy and places | Four new buildings with real inputs/costs/effects. Separate fast-food chains/bars, businesses by era, district land values and detailed consumer demand are pending. |
| Public transport and travel | Original rail and ports retained. Metro, bus/tram routes, passenger rail and airports are pending. |
| Weather and power | Visual changing weather, slow sky periods and solar consequences implemented. Seasonal ecology, flood/hazard simulation, batteries and grid dispatch are pending. |
| Advisors and news | Location-specific shortages and election results implemented. A full interactive advisor roster, richer newspapers and municipal policy workflows are pending. |
| Languages | Original translations retained. New City Evolution text is English; translation work is pending. |

This is an unofficial modified community build. Original engine/source/assets retain their notices and licenses. The new integration code is GPL-2.0-or-later.
