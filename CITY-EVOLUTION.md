# Current release: Preview 17

See CHANGELOG-Preview-17.md and README-Preview17.txt.

# Current release: Preview 16

See CHANGELOG-Preview-16.md, README-Preview16.txt and doc/AUDIT-Preview-16.md.

# City Evolution — current release: Preview 15

Read [CHANGELOG-Preview-15.md](CHANGELOG-Preview-15.md), [doc/AUDIT-Preview-15.md](doc/AUDIT-Preview-15.md) and README-Preview15.txt. Earlier release notes below are historical.

# City Evolution (fork of LinCity-NG), Preview 13

New in this integration: [CHANGELOG-Preview-13.md](CHANGELOG-Preview-13.md). Review evidence: [doc/AUDIT-Preview-13.md](doc/AUDIT-Preview-13.md). Budget / public services opens from City Hall or F7.

Start here. Older release notes are in `doc/history/`. What changed in this release: `CHANGELOG-Preview-12.md`.
The audit that produced it, with evidence: `doc/AUDIT-Preview-12.md`.

## Folder map
| Folder | What it holds |
| --- | --- |
| `src/` | Game code. `src/lincity` is the simulation (economy, workforce, districts, mayor, plans, church, transit). `src/lincity-ng` is the game window and menus. `src/gui` is the widget toolkit. |
| `data/` | Everything the game loads: `gui/` layouts (XML), `images/`, `sounds/`, `music/`, `locale/`. `data/models/` are the original Blender models (upstream). |
| `tests/` | Simulation test programs (built with `-DLINCITYNG_EVOLUTION_CHECKS=ON`) and Python checks for the interface files. |
| `contrib/windows-cross/` | Script that builds the Windows version from Ubuntu. |
| `contrib/preview11/`, `contrib/revamp/`, `art/` | The art pipeline (scripts, prompts, metadata). Large art masters are in the separate archive `city-evolution-art-sources`. |
| `doc/AUDIT-Preview-12.md`, `doc/audit/` | Audit report and the test logs from this release. |

## Build and test (Ubuntu 24.04)
`contrib/build-ubuntu-2404.sh` builds SDL3 and the game. Then:
`build/linux/tests/city-evolution/{city-evolution,mayor,transit,community}-checks` and `python3 tests/gui_style_check.py`, `python3 tests/preview11_assets_check.py`.

## Windows
`contrib/windows-cross/build-windows.sh` cross-compiles a 64-bit Windows build. The package starts with native `City Evolution.exe`, launches `Game/bin/lincity-ng.exe`, and keeps saves/settings/logs in `PortableData/`.

## Rules that keep the interface working
1. Every file in `data/gui/` must start with an `<?xml-stylesheet ... .xsl?>` line. The build filters each file through it; without it the build stops. `tests/gui_style_check.py` now enforces this.
2. City Hall pages are fixed layouts with no scrolling. Keep each page within about 410 pixels of height.
3. Tool drawers in `buttonpanel.xml` are sized to their number of buttons (80 pixels per row of three, plus a 72 pixel header). Add a button, check the height.
4. Unavailable buildings are greyed out by `src/gui/CheckButton.cpp`; do not give buttons their own disabled look.
