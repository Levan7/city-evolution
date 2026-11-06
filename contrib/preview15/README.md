# Preview 15 reproducibility notes

The standard user build recipes are `contrib/build-ubuntu-2404.sh` and
`contrib/windows-cross/build-windows.sh`. This directory retains the isolated
SDK, packaging and graphical smoke-test procedures used for the release.
They expect a sibling `preview15-work` staging directory with the documented
source/build/sysroot/prefix structure; adapt that one staging root when reusing.
Test-only LD_PRELOAD hooks drive SDL events and capture screenshots. They are
not part of the installed game. Headless X11/Wayland use TCP or inherited
socketpairs because this build host does not allow named Unix sockets.

`package_linux.py` reuses the tested Preview 14 AppImage runtime via a local
`appimage-runtime-x86_64` file. That runtime is the first bytes up to the
baseline image's `--appimage-offset`. No download of a different runtime is
necessary. Build SDL with public host ALSA SONAME `libasound.so.2`.

The audit has build/test logs, exact dependency archive hashes, changed-source
overlay, patch and screenshots. No production binaries include smoke hooks.
