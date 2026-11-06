#!/usr/bin/env bash
# Build LinCity-NG (City Evolution) on Ubuntu 24.04. Run from the extracted source folder.
# Ubuntu 24.04 has no SDL3 packages, so SDL3 is built from source into ~/lc-prefix first.
set -e
P=$HOME/lc-prefix; mkdir -p $P ~/lc-deps; cd ~/lc-deps
sudo apt-get install -y build-essential cmake ninja-build pkg-config xsltproc gettext curl libxml2-dev zlib1g-dev \
  libfreetype-dev libpng-dev libvorbis-dev libogg-dev libx11-dev libxext-dev libxcursor-dev libxi-dev libxrandr-dev \
  libxfixes-dev libxss-dev libxkbcommon-dev libxtst-dev libxrender-dev libgl-dev libasound2-dev libpulse-dev libwayland-dev wayland-protocols libxkbcommon-dev libdecor-0-dev libegl1-mesa-dev libgles2-mesa-dev
get(){ [ -d "$2" ] || { curl -sL "https://codeload.github.com/libsdl-org/$1/tar.gz/$3" | tar xz; }; }
get SDL SDL-release-3.4.0 refs/tags/release-3.4.0
get SDL_image SDL_image-release-3.2.4 refs/tags/release-3.2.4
get SDL_ttf SDL_ttf-release-3.2.2 refs/tags/release-3.2.2
get SDL_mixer SDL_mixer-main refs/heads/main
export PKG_CONFIG_PATH=$P/lib/pkgconfig
B(){ cmake -S "$1" -B "b_$1" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$P -DCMAKE_PREFIX_PATH=$P "${@:2}" && cmake --build "b_$1" && cmake --install "b_$1"; }
# Resolve ALSA through its public SONAME across Ubuntu/Fedora versions.
sed -i 's/${ASOUND_LIB_SONAME}/libasound.so.2/g' SDL-release-3.4.0/cmake/sdlchecks.cmake
B SDL-release-3.4.0 -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF -DSDL_WAYLAND=ON -DSDL_WAYLAND_SHARED=ON -DSDL_WAYLAND_LIBDECOR=ON -DSDL_WAYLAND_LIBDECOR_SHARED=ON -DSDL_PIPEWIRE=OFF -DSDL_PULSEAUDIO=ON -DSDL_JACK=OFF -DSDL_KMSDRM=OFF
B SDL_image-release-3.2.4 -DSDLIMAGE_VENDORED=OFF -DSDLIMAGE_BACKEND_STB=ON -DSDLIMAGE_PNG=ON -DSDLIMAGE_JPG=ON -DSDLIMAGE_AVIF=OFF -DSDLIMAGE_TIF=OFF -DSDLIMAGE_WEBP=OFF -DSDLIMAGE_JXL=OFF
B SDL_ttf-release-3.2.2 -DSDLTTF_VENDORED=OFF -DSDLTTF_HARFBUZZ=OFF -DSDLTTF_PLUTOSVG=OFF
B SDL_mixer-main -DSDLMIXER_VENDORED=OFF -DSDLMIXER_OPUS=OFF -DSDLMIXER_MIDI=OFF -DSDLMIXER_GME=OFF -DSDLMIXER_WAVPACK=OFF -DSDLMIXER_MOD=OFF -DSDLMIXER_FLAC=OFF -DSDLMIXER_MP3=OFF -DSDLMIXER_VORBIS_VORBISFILE=OFF -DSDLMIXER_VORBIS_STB=ON -DSDLMIXER_SAMPLES=OFF
cd - >/dev/null
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$P -DLINCITYNG_EVOLUTION_CHECKS=ON
cmake --build build/linux
echo "Run with:  LD_LIBRARY_PATH=$P/lib build/linux/bin/lincity-ng --app-data build/linux/share/lincity-ng"
