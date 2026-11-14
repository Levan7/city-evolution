# City Evolution

City Evolution is an open-source city builder by Levan, based on LinCity-NG.
Grow a settlement into a city by connecting homes, jobs, freight, public services
and passenger transport.

[Releases](https://github.com/Levan7/city-evolution/releases) ·
[Issues](https://github.com/Levan7/city-evolution/issues) ·
[Changelog](CHANGELOG.md)

[![Support City Evolution on Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/levan4kgaming)

## What City Evolution adds to LinCity-NG

City Evolution builds on the original C++ simulation. The table describes the
cumulative changes through Preview 22. The changelog records the detailed
changes and fixes by version.

| Area | Changes |
| --- | --- |
| World creation | Six map sizes from 50 × 50 to 400 × 400, with starter villages, empty natural landscapes, optional tree removal, and barren flatland. Classic scenarios retain their own maps. |
| City government | City Hall brings together approval, elections, advisors, city news, districts, workforce and development plans. Name districts and give them a purpose such as farming, industry, trade, education or technology. Plans cover livability, skills and education, or technology. |
| Mayor and finances | Choose limited or full mayor autonomy and let the mayor work within a selected area or across the city. Control construction permissions, pace and spending, and review its decisions. Income, goods and coal taxes and export proceeds post monthly, with annual totals retained. Preview monthly income tax before changing it, set a tax ceiling, and automate income tax and service funding independently. Financial advice separates operating costs from construction and flags deficits, low reserves and debt. |
| People and jobs | The expanded workforce tracks education, skills and retirement. Schools and universities train workers, specialized industries depend on qualified staff, and access to homes, jobs and supplies influences settlement. Advisors distinguish unemployment, housing shortages and supply problems. |
| Passenger transport | Build bus stops and terminals, passenger rail, underground metro and elevated metro. Walking coverage, staffing, connections, transfers and capacity determine service. Bus terminals have a larger catchment than ordinary stops. Direct routes carry labor to jobs and groceries to homes. Follow routes in the transport report, inspect stop coverage on the map, and jump to crowded stops from City Advisors. |
| Freight and trade | Warehouses buffer and ship goods along connected road, track and rail corridors. They can bridge rail and road, supply industry directly or feed local markets. Markets reach 14 tiles. Warehouses serve nearby buildings and relay surplus to other connected warehouses. The larger distribution center holds four times as much stock. Market controls distinguish local receiving and supplying from port imports and exports. Selected infrastructure can be built when city cash is negative. |
| Power and buildings | Roads and tracks carry local low-voltage power from staffed generation or substations to occupied homes, with distance losses and supply warnings. New building choices include a church, warehouses, cafés, food halls, technology campuses, police stations, three apartment towers, three office towers, a football stadium, computer and car factories, and a nuclear power station. |
| Public services and health | Set education, healthcare, fire and police funding in 5% steps from 0% to 200%. Fire engines, ambulances and police crews route to actual incidents. Pollution illness and optional seasonal flu affect households, with recovery and temporary immunity in the flu simulation. The game also includes a rare hidden event. |
| Art, animation and sound | New and revised buildings, thumbnails and illustrated Help, more varied housing and stock details, animated pedestrians and route-following traffic. Horses and carriages give way to motor vehicles as technology advances. Buildings retain their production and construction stages. Day and night lighting, weather, street lamps, storm ambience and a church bell add visible and audible changes. |
| Interface and desktop support | Reworked main menu, construction drawers, speed controls, city management windows and minimap. Management windows pause the simulation. The minimap supports scroll zoom and click navigation, with improved overlays and statistics. The portable Windows launcher and Linux AppImage bundle the game runtime. The Linux build supports Wayland and X11, automatic UI scaling and desktop audio fallback. |

Full release history is in [CHANGELOG.md](CHANGELOG.md). Press **F1** in the
game for illustrated Help.

## Play

Download a package from Releases. On Windows, extract the whole ZIP and open
**City Evolution.exe**. On Linux, make the AppImage executable and launch it.
Preview builds are for testing. Back up saves before trying a new version.

## Build from source

Build with CMake and the [required dependencies](doc/DEPENDENCIES.md). On Ubuntu
24.04, [the build script](contrib/build-ubuntu-2404.sh) builds the SDL dependencies
and game. With SDL already installed:

```sh
cmake -S . -B build -DENABLE_NLS=OFF -DLINCITYNG_EVOLUTION_CHECKS=ON
cmake --build build --parallel
./build/bin/lincity-ng --app-data build/share/lincity-ng
```

The optional simulation checks are built under `build/tests/city-evolution`.
Run the Python checks in `tests/` individually. See the
[build options](doc/BUILD_OPTIONS.md) and
[Preview 18 audit](doc/AUDIT-Preview-18.md) for more detail.

Code and asset licenses are recorded in the `COPYING*.txt` files. See the
[upstream README](doc/UPSTREAM-README.md) for LinCity-NG.
