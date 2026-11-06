#!/usr/bin/env bash
# Cross-compile City Evolution (LinCity-NG) for 64-bit Windows from Ubuntu 24.04, and package it as a zip.
# Run from the root of the source tree:   contrib/windows-cross/build-windows.sh
# This is the exact recipe used to produce the Windows build. Takes about 30 minutes on one CPU core.
set -e
SRC=$(pwd); HERE=$SRC/contrib/windows-cross
export WINPREFIX=${WINPREFIX:-$HOME/win-prefix}; W=$HOME/win-deps; mkdir -p $WINPREFIX $W; cd $W
sudo apt-get install -y build-essential cmake ninja-build pkg-config xsltproc gettext curl git zip \
  g++-mingw-w64-x86-64-posix gcc-mingw-w64-x86-64-posix
TC="-DCMAKE_TOOLCHAIN_FILE=$HERE/toolchain.cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$WINPREFIX"
get(){ [ -d "$3" ] || curl -sL "https://codeload.github.com/$1/tar.gz/$2" | tar xz; [ -d "$3" ]; }
get madler/zlib refs/tags/v1.3.1 zlib-1.3.1
get GNOME/libxml2 refs/tags/v2.12.9 libxml2-2.12.9
get freetype/freetype refs/tags/VER-2-13-3 freetype-VER-2-13-3
get libsdl-org/SDL refs/tags/release-3.4.0 SDL-release-3.4.0
get libsdl-org/SDL_image refs/tags/release-3.2.4 SDL_image-release-3.2.4
get libsdl-org/SDL_ttf refs/tags/release-3.2.2 SDL_ttf-release-3.2.2
get libsdl-org/SDL_mixer refs/heads/main SDL_mixer-main
B(){ local s=$1; shift; cmake -S $s -B b_$s $TC "$@" && cmake --build b_$s && cmake --install b_$s; }
B zlib-1.3.1 -DBUILD_SHARED_LIBS=OFF -DZLIB_BUILD_EXAMPLES=OFF
[ -f $WINPREFIX/lib/libzlibstatic.a ] && cp $WINPREFIX/lib/libzlibstatic.a $WINPREFIX/lib/libz.a
B libxml2-2.12.9 -DBUILD_SHARED_LIBS=ON -DLIBXML2_WITH_PYTHON=OFF -DLIBXML2_WITH_ICONV=OFF -DLIBXML2_WITH_LZMA=OFF -DLIBXML2_WITH_TESTS=OFF -DLIBXML2_WITH_PROGRAMS=OFF -DLIBXML2_WITH_ZLIB=ON -DLIBXML2_WITH_HTTP=OFF -DCMAKE_PREFIX_PATH=$WINPREFIX
B freetype-VER-2-13-3 -DBUILD_SHARED_LIBS=OFF -DFT_DISABLE_ZLIB=ON -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_PNG=ON -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON
B SDL-release-3.4.0 -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
B SDL_image-release-3.2.4 -DCMAKE_PREFIX_PATH=$WINPREFIX -DSDLIMAGE_VENDORED=OFF -DSDLIMAGE_BACKEND_STB=ON -DSDLIMAGE_PNG=ON -DSDLIMAGE_JPG=ON -DSDLIMAGE_AVIF=OFF -DSDLIMAGE_TIF=OFF -DSDLIMAGE_WEBP=OFF -DSDLIMAGE_JXL=OFF
B SDL_ttf-release-3.2.2 -DCMAKE_PREFIX_PATH=$WINPREFIX -DSDLTTF_VENDORED=OFF -DSDLTTF_HARFBUZZ=OFF -DSDLTTF_PLUTOSVG=OFF
B SDL_mixer-main -DCMAKE_PREFIX_PATH=$WINPREFIX -DSDLMIXER_VENDORED=OFF -DSDLMIXER_OPUS=OFF -DSDLMIXER_MIDI=OFF -DSDLMIXER_GME=OFF -DSDLMIXER_WAVPACK=OFF -DSDLMIXER_MOD=OFF -DSDLMIXER_FLAC=OFF -DSDLMIXER_VORBIS_VORBISFILE=OFF -DSDLMIXER_VORBIS_STB=ON -DSDLMIXER_SAMPLES=OFF
cd $SRC
cmake -S . -B build/win -G Ninja -DCMAKE_TOOLCHAIN_FILE=$HERE/toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$WINPREFIX -DLINCITYNG_RELOCATABLE=ON -DLINCITYNG_EVOLUTION_CHECKS=OFF
cmake --build build/win
# Package: game files + the DLLs it needs.
OUT=$SRC/build/CityEvolution-Windows-x64
mkdir -p "$OUT/Game" "$OUT/Settings" "$OUT/PortableData"
cmake --install build/win --prefix "$OUT/Game"
GCCDLL=$(dirname $(find /usr/lib/gcc/x86_64-w64-mingw32 -name libstdc++-6.dll | grep posix | head -1))
cp "$WINPREFIX"/bin/*.dll "$GCCDLL/libstdc++-6.dll" "$GCCDLL/libgcc_s_seh-1.dll" /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll "$OUT/Game/bin/"
cp "$SRC/contrib/windows/default.xml" "$OUT/Settings/default.xml"
(cd "$SRC/contrib/windows" && x86_64-w64-mingw32-windres launcher.rc -O coff -o "$OUT/launcher-resource.o")
x86_64-w64-mingw32-g++-posix -static -municode -mwindows "$SRC/contrib/windows/launcher.cpp" "$OUT/launcher-resource.o" -o "$OUT/City Evolution.exe" -lshell32
rm "$OUT/launcher-resource.o"
cp "$SRC/README-Preview15.txt" "$OUT/README.txt"
(cd "$SRC/build" && zip -qr CityEvolution-Windows-x64.zip CityEvolution-Windows-x64)
echo "Done: $SRC/build/CityEvolution-Windows-x64.zip (run City Evolution.exe)"
