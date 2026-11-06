# City Evolution — Preview 9

A fork of LinCity-NG. This is a development preview built from the uploaded
Preview 8 source, preserving its city naming, districts, mayor mandate/pace,
planning, workforce, elections, economy and atmosphere features.

## Start on Windows

Extract the entire portable ZIP into a NEW writable folder. Run `Play.cmd`.
Choose **Continue city** for Green Junction, the included transport sandbox.
It has temporary commodity stocks and $5 million; it is not a sustainable city.
For your own city, use New world or copy a backed-up save into PortableData.
Do not overlay this package onto an older installation.

The heading and launcher both say Preview 9. Internal engine filenames retain
`lincity-ng` for configuration and save compatibility. Original authors and
licenses remain in Credits and the supplied license files.

## What changed

- Matching dark green menus, dialogs, settings, save/load pages and toolbar chrome.
- City Evolution branding and application icon; readable light text.
- Compact pause / green play / fast / fastest controls, labelled GAME SPEED.
- Top-right Mayor and Transport buttons. No function key is required.
- Restored framed map/statistics area, larger statistics text, scrollable City Hall.
- 29 building model replacements, including six housing tiers, businesses, farms,
  schools, healthcare, university, fire service, utilities, industry and stations.
- 45 art masters/exported sprites, including four views each of car, bus and railcar.
- Cars, buses and representative trains move along actual street/track paths.
- Decorative horse at markets, horse cart at farms and truck at warehouses.
- Five passenger facilities and two independent track layers, with seven new icons.

## Build public transport

Open **Transport**, at the top right, to choose a tool or read the service report.
The left Transport construction drawer also contains the new tools.

1. Put one stop near homes and another near workplaces.
2. Join bus stops/terminals with **Road**, or passenger railway stations with **Rail**.
   A surface railway has separate passenger stations; existing rail still carries cargo.
3. Join underground entrances using **Tunnel track**. Join elevated stations with
   **Elevated track**. These are separate layers, even on the same map square.
4. Stations must touch their route orthogonally; diagonal corners do not connect.
   Tunnels/elevated track may run through a station's footprint and across buildings.
5. Supply workers. Rail and both metro modes also need low-voltage electricity.
   Supply stations through the normal surface commodity network. Track layers carry
   passengers, not the original engine's food, labor or electricity inventory.
6. Wait through a month of service, then press Refresh service report. The first
   month has partial availability. Nearby different-mode stations can provide one transfer.

Catchment is six walking tiles for bus and ten for rail/metro (Manhattan distance).
Interchanges must be within four tiles. Trips are an aggregate monthly estimate
based on nearby residents, workplaces, service days and line capacity. They are
not individual citizens following a timetable. Fare income is real city money,
collected once a month; upkeep is charged even when there are no riders.

| Facility | Build cost | Size | Technology | Daily upkeep |
| --- | ---: | --- | ---: | ---: |
| Bus stop | $800 | 1x1 | none | $4 |
| Bus terminal | $7,500 | 2x2 | none | $18 |
| Passenger railway station | $15,000 | 3x3 | 5% | $35 |
| Underground metro entrance | $18,000 | 1x1 | 10% | $40 |
| Elevated metro station | $22,000 | 2x2 | 10% | $45 |

A bus facility needs two labor units per day; other stations need eight labor
and fifteen low-voltage power units per day. Tunnel costs $1,000/tile; elevated
track $1,800/tile. Track upkeep is $2/tile/month. Fares are $1/bus boarding and
$2/rail or metro boarding. One-transfer journeys count both legs. Served housing
gets a desirability benefit. A connected line needs two supplied stations and a
workplace destination; roads and freight buffers do not count as workplaces.

Cycle the view button in Transport: Normal / All transit / Underground / Elevated.
Underground routes are shown only in All transit or Underground view. Elevated
routes have a raised deck, rails, piers and moving railcars. In Underground or
Elevated view, the bulldozer removes that track layer first and keeps surface
buildings. Return to Normal view before bulldozing a surface building.

## Validation and limits

The Windows x64 UCRT executable and three diagnostic executables were cross-built.
The bundled DLL import/export audit passed. Native real-engine tests with Address
and Undefined Behavior sanitizers passed for all four modes, transfers, route
cuts, missing supplies, fares/upkeep, save/load and independent track layers.
The original mayor suite and 168,536 evolution checks passed, including 19 world
generation cases. All six original scenarios loaded, re-saved and simulated.
XML asset paths, table cells, resource frame slots and static dialog names passed.
See `validation/` for recorded outputs. Diagnostics use the real simulation with
inert display/audio adapters; they do not exercise Windows graphics or startup.

Windows launch, click interaction, exact sprite alignment and final text rendering
have NOT been play-tested here. Use `Verify-Transit.cmd`, `Verify-Mayor.cmd` and
`Verify-Evolution.cmd` to run the Windows diagnostics on your PC. Failure output
is written into PortableData. Game startup output is PortableData/launch.log.

This is a substantial preview, not the completed total graphics overhaul. Terrain,
bridges, mines, monuments and some other assets still use original art. New
building sprites are static; old smoke/swing overlays tied to old geometry are
suppressed while keeping their frame slots. Horse, cart and truck are decorative.
The older single-house master is preserved as source art but does not replace the
homeless/shanty simulation. Day/night and weather behavior from Preview 8 remains;
this release does not supply complete separate night/rain variants for every model.
Passenger routing is aggregate, with one representative vehicle per used connected
line, one transfer maximum, no timetables, congestion model or route editor.

New transit fields are additive to the existing save format. Old saves load, but
opening new-transit saves in an older executable is not supported. Keep backups.

## Source build

Complete C++ sources, vendored C++ dependencies, data, generated art masters and
prompt records are in the accompanying source archive. Use CMake and the normal
LinCity-NG dependencies described in `doc/DEPENDENCIES.md`.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_NLS=OFF -DLINCITYNG_EVOLUTION_CHECKS=ON
cmake --build build --parallel
```

For a Linux-to-Windows UCRT build, use `mk/cmake/toolchains/windows-ucrt.cmake`,
setting `LINCITY_LLVM_MINGW_ROOT` and `LINCITY_WIN_SDK_ROOT` to the LLVM-MinGW
UCRT toolchain and MSYS2 mingw64 dependency SDK. C++ fmt/libxml++ are built from
vendored source; the application statically links libc++ and uses matching
libunwind. Do not mix prebuilt C++ ABI libraries from a different compiler.
The provided RC wrapper handles Ninja depfile flags unsupported by LLVM windres.
Use host xsltproc to generate namespaced GUI XML; copy those generated files,
not raw data/gui XML, into the portable application's share/lincity-ng/gui folder.

`tests/transit_integration.cpp` and `tests/transport_showcase.cpp` document the
network fixtures; the latter can generate the included stocked sandbox.
`art/manifest.json` maps replacement sprites to engine resources. Building
anchors use the original engine's bottom-of-lot convention. Existing image frame
counts remain intact for simulation and save compatibility. Original engine and
third-party license notices are retained. See the original README in
`doc/UPSTREAM-README.md` for upstream background.
