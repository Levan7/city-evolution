# Audit — City Evolution Preview 14

## Scope

Baseline: the supplied CityEvolution-Preview-13-Source.tar.xz, which contains the audited Preview 12 menus and Preview 13 services. The recovery overlay contains the earlier session's runtime changes; this rebuild adds native Wayland support, startup error reporting and correct final sound/SDL lifetime ordering. No unrelated menu redesign was made. The full source is supplied separately.

## Changes and tests

- Safe mixer/track initialization, bounded volume, main-thread music advancement, actual mixer teardown, no background loader race. The failed-ALSA regression compiles original Preview 13 Sound.cpp and fixed Sound.cpp: old exits with SIGSEGV, fixed exits zero.
- A graphical test exposed a second crash only on exit with working audio: Sound lived past SDL_Quit. Sound is now explicitly released first. The actual AppImage passes graphical volume controls and clean exit with PulseAudio output; the earlier failing trace is retained as audio-shutdown-before.log.
- Native Wayland and libdecor built into SDL, alongside X11. The host supplies desktop audio/Wayland/EGL client libraries; Ubuntu libasound/libpulse are excluded. SDL library configuration and bundled-library/import reports are included.
- The actual AppImage runs under Weston headless at 3072x1590 without DISPLAY or development library paths. UI scale is 1.75, logical dimensions 1755x909. Sixty synthetic graphical volume clicks raise both saved volumes from35/45 to100; SDL_QUIT exits normally. These are input events through the real menu hit-testing path, not direct volume-setter calls. A PulseAudio null-sink monitor records non-zero PCM from the real game music. This verifies protocol/output samples, not physical speakers.
- Named UNIX sockets are blocked in this execution environment. Weston/libwayland's supported preconnected socketpair mode permits a real native Wayland session. Software rendering is used for the test; physical Fedora/KDE/GNOME GPU/compositor behavior remains untested. The AppImage itself does not force software rendering or inherit these test hooks.
- Six simulation programs pass: services, community, mayor, transit, city-evolution, flu. The supplied Preview13 150x150 city loads, advances500 service/flu days and roundtrips. Audio tests cover working dummy/Pulse devices and absent/failed ALSA devices with1,000 volume changes and natural/manual music switching.
- Protected-menu/style checks pass. Original animation resource frame counts and build catalogue remain. Added16 woman walking sprites and12 inventory props; raw generated atlases/export code are included. Pollution illness remains separate from optional saved flu.

The complete logs under doc/audit/preview14/fresh-rebuild are current; previous-session contains the saved checkpoint evidence. Helper programs and SDL input hooks are test-only and are not bundled in executable game launch paths.

## Review material

The audit ZIP contains changed source/assets, a binary-capable Preview13-to14 patch, logs, dependency input hashes and graphical evidence. The full source archive contains the complete project and data. The binary patch can be applied to the unmodified Preview13 source; the source archive is preferable for ordinary building. Existing history/license/credit files are preserved.

Read CHANGELOG-Preview-14.md for gameplay/tuning details. No Android runtime, physical Fedora audio device or real Windows PC was available. Windows is cross-compiled with DLL import closure checked; Wine is not claimed tested here. Long gameplay, flu balance and large-city performance remain open validation items.
