# City Evolution

**Build a settlement. Connect its people. Shape its future.**

City Evolution is an experimental city-building simulation fork of **LinCity-NG**, developed by **Levan**. It combines the original simulation with refreshed isometric artwork, public transport, city-management tools and an optional autonomous mayor.

[Download the game](https://github.com/Levan7/city-evolution/releases) · [Report a bug](https://github.com/Levan7/city-evolution/issues) · [Support development](https://ko-fi.com/levan4kgaming)

[![Support City Evolution on Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/levan4kgaming)

## Build a city that works

- **Grow your economy:** connect homes, farms, workshops and industry, manage resources and keep essential services supplied.
- **Connect neighbourhoods:** build bus stops, railway stations, underground metro and elevated transit. Trips consider access to homes and jobs, connected routes and journey distance.
- **Choose your level of control:** manage the city yourself, delegate limited tasks, or give the mayor full autonomy within a district, a drawn zone or the whole map.
- **Manage public finances:** adjust taxes and education, healthcare, police and fire-service funding. Optional tax and funding automation can help, while the financial advisor explains economic priorities.
- **Watch the city change:** refreshed buildings, activity visuals, pedestrians, horses and carriages, technology-based motor traffic, changing weather and day/night lighting.
- **Respond to city problems:** pollution-related illness, flu transmission and emergency crews add consequences to infrastructure and service decisions.

## Current development version: Preview 17

The latest source package adds dirt-track connections for bus stops, clearer connection/staffing reports, distance-sensitive transit journeys, road-based local electricity delivery with losses, and finer service-funding controls.

See [CHANGELOG-Preview-17.md](CHANGELOG-Preview-17.md) for changes and [doc/AUDIT-Preview-17.md](doc/AUDIT-Preview-17.md) for validation and limitations. Check the release title before downloading: published builds may be older than the development source.

**This is a work in progress.** Bugs, balance problems and platform-specific issues remain possible. Back up your city saves before updating.

## Download and play

Get a build from the [Releases page](https://github.com/Levan7/city-evolution/releases).

### Windows

1. Download the Windows x64 ZIP.
2. Extract the **entire archive** into a writable folder.
3. Run **City Evolution.exe** and keep the accompanying folders beside it.

Saves, settings and the startup log are stored in `PortableData`. Preview builds are unsigned; Windows may request a security scan.

### Linux

Download the x86_64 AppImage, allow it to run as a program, then launch it. For Preview 17:

```bash
chmod +x CityEvolution-Preview-17-Linux-x86_64.AppImage
./CityEvolution-Preview-17-Linux-x86_64.AppImage
```

If FUSE is unavailable:

```bash
./CityEvolution-Preview-17-Linux-x86_64.AppImage --appimage-extract-and-run
```

The current AppImage requires **glibc 2.39 or newer** and includes Wayland/X11 support. Saves normally live in `~/.local/share/city-evolution`; settings in `~/.config/city-evolution`, subject to your XDG configuration.

## Mayor and budget controls

| Control | What it delegates |
| --- | --- |
| Full autonomy | Construction and city management inside the selected area; initially enables tax and funding automation |
| Auto-adjust taxes | Income-tax adjustment only |
| Auto funding | Public-service funding adjustments |

Funding percentages describe **spending levels**, not geographical coverage. Buildings still need workers, supplies and connections. An autonomous mayor spends actual available money and cannot build without sufficient funds.

Management windows pause the simulation. Close them to resume the selected game speed.

## Building from source

The project uses the original LinCity-NG engine, CMake and SDL3. Start with:

- [Ubuntu build instructions](contrib/build-ubuntu-2404.sh)
- [Windows cross-build instructions](contrib/windows-cross/build-windows.sh)
- [Source directory guide](CITY-EVOLUTION.md)

Enable `LINCITYNG_EVOLUTION_CHECKS=ON` to build simulation diagnostics. GUI checks include:

```bash
python3 tests/gui_style_check.py
python3 tests/preview15_ui_check.py
python3 tests/menu_preservation_check.py
python3 tests/preview11_assets_check.py
```

Some test and pipeline filenames retain their original preview numbers; they remain relevant to the current source.

## Testing and bug reports

Native simulation and graphical checks have been run. Windows builds are cross-compiled and dependency-checked; this does **not** replace testing on a physical Windows PC. The current release audit records what was tested.

Please [open an issue](https://github.com/Levan7/city-evolution/issues) with:

- Your game version, operating system and relevant hardware.
- Steps to reproduce the problem and what you expected to happen.
- A screenshot or short recording, plus a save file or startup log when relevant.

Contributions, testing and practical feedback are welcome.

## Support development

If you enjoy City Evolution or want to support its continued development, you can [buy me a coffee on Ko-fi](https://ko-fi.com/levan4kgaming). Support is optional and appreciated.

## Credits and licensing

City Evolution builds on the work of the **LinCity-NG contributors**. Original attribution and license notices are retained.

See `COPYING.txt`, `COPYING-data.txt`, `COPYING-fonts.txt` and `COPYING-dll.txt` for the applicable terms. Code, artwork, fonts and dependencies may have different licensing requirements.

City Evolution development: **Levan** · **studio76@mail.com**
