# City Evolution

City Evolution is an open-source city builder by Levan, based on LinCity-NG.
Grow a settlement into a city by connecting homes, jobs, freight, public services
and passenger transport.

[Releases](https://github.com/Levan7/city-evolution/releases) ·
[Issues](https://github.com/Levan7/city-evolution/issues) ·
[Changelog](CHANGELOG.md)

[![Support City Evolution on Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/levan4kgaming)

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
