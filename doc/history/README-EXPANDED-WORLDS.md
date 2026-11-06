Current release: [UPGRADE-0.5.md](UPGRADE-0.5.md).

Latest Windows startup candidate: [HOTFIX-0.3.1.md](HOTFIX-0.3.1.md).

Current upgrade release: see [CITY-EVOLUTION-0.3.md](CITY-EVOLUTION-0.3.md). The notes below describe the earlier map/start release.

# LinCity-NG Expanded Worlds 0.2.1

Unofficial original-engine community build for Levan's city-building project.
Based on LinCity-NG 2.15.0, commit 6f6ffe2c8c5558b3d68ed6b4b8f68721817c3792.
This version changes and recompiles the original C++ engine.

## New-world setup

Open **New Game**. The land-size arrows cycle through these choices:

| Name | Map dimensions | Area compared with the old default |
| --- | --- | --- |
| Small | 50 x 50 | 0.25x |
| Medium-small | 100 x 100 | 1x |
| Medium | 150 x 150 | 2.25x |
| Large | 200 x 200 | 4x |
| Huge | 300 x 300 | 9x |
| Vast | 400 x 400 | 16x |

Leave **Start with a village** unchecked for an empty natural landscape with
no placed buildings or roads. Choose **Remove trees** for clear natural land.
Select **Barren flatland** for a completely clear, flat map without buildings,
roads, trees or surface rivers. Groundwater and underground minerals remain so
wells, farms and mining can work. This mode always overrides the village option.
Empty starts receive $200,000; the original starter village remains available.

Original scenarios retain their own size, buildings and starting money. Their
world controls are disabled when a scenario is selected. Existing cities are
not resized: land-size choices apply when generating a NEW world. Larger maps
require more memory and take longer to generate and simulate.

New-world preferences are saved in the portable configuration after creation.
Map loading now keeps the altitude scale at least 1, including after loading a
flat world, to avoid division by zero in terrain-height rendering.

## Advisors

The **Advisors** button beside the speed/menu controls opens an initial city
diagnostic. It identifies factories/ports lacking a direct transport neighbor,
imported materials accumulating at ports, and missing schools or universities.
Port/factory messages include tile coordinates. This is an on-demand report,
not a complete traffic-routing or employment analysis.

Dedicated warehouses, new technology-industry buildings, detailed freight
capacity/routing and competing municipalities are NOT part of this release.
The existing original simulation continues to handle production and trade.

## Portable use and existing saves

Extract the complete ZIP, then launch **Play.cmd**. Keep the Game, Settings and
PortableData folders together. Use a writable location.

To transfer cities from the previous working native build, make a backup and
copy its PortableData folder into this build's extracted folder. Existing
configuration values are retained. Godot prototype saves remain incompatible.

The executable has a new version suffix: `2.15.0-expanded-worlds-0.2.1`.
Original assets, DLLs and licenses are preserved. The newly built executable
statically links its compiler's C++ support runtime; the original DLL runtimes
remain available for the original libraries. See contrib/expanded-worlds for
runtime notices and the GCC Runtime Library Exception.

## Building

On Windows use the MSYS2 MINGW64 environment with the development packages for
GCC, CMake, Ninja, SDL3, SDL3_image, SDL3_mixer, SDL3_ttf, libxml2,
libxml++-5.0, fmt, zlib, gettext, pkgconf and libxslt installed. Follow the
upstream README.md and doc/DEPENDENCIES.md for the normal build prerequisites.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DLINCITYNG_RELOCATABLE=ON -DLINCITYNG_VERSION_SUFFIX=expanded-worlds-0.2.1 \
  -DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static-libgcc"
cmake --build build --parallel 4
cmake --install build --prefix release/Game
```

The delivered executable was cross-compiled on Linux with GCC 13.2 MinGW-w64
POSIX, CMake 3.28.3 and Ninja 1.11.1. Development headers/import libraries for C-interface dependencies:
SDL3 3.4.10, SDL3_image 3.2.6, SDL3_mixer 3.2.4, SDL3_ttf 3.2.2,
libxml++ 5.6.0, libxml2 2.15.3, fmt 12.1.0, zlib 1.3.1 and gettext 0.26.
Only shared-library SDL CMake targets were enabled for cross-compilation.

## Tests and validation limits

The map-policy test runs without SDL:

```sh
g++ -std=c++17 -Isrc tests/world_setup_policy.cpp -o world-policy-test
./world-policy-test
```

The build also exposes `--verify-worlds <scratch-directory>` for generation,
save/load, flat-map simulation and empty/village regression checks. The
portable package includes **Verify-Worlds.cmd** to run these checks on Windows
and write PortableData/world-verification.log. It does not alter saved cities.

During preparation, the policy test, GUI structure/asset checks, compilation
and imported-DLL-symbol checks passed. The Windows executable and integration
suite have NOT been executed here. The menu layout has been checked against
the GUI parser/layout code and source assets, but not rendered by this modified
executable. See VALIDATION.txt in the portable package for the exact checks.

## Licensing

GPL-2.0-or-later, with original notices preserved. Source is supplied separately
as a complete archive of this modified tree. The original asset/font/library
terms remain in COPYING-data.txt, COPYING-fonts.txt and COPYING-dll.txt.

See HOTFIX-0.2.1.md and third_party/README.md for the startup compatibility rebuild. libxml++ 5.6.0 is now built from included source and fmt 12.1.0 is header-only; no prebuilt C++ wrapper/formatting DLL is linked into the game. The original reported crash remains unconfirmed until Windows testing.
