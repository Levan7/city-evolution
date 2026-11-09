City Evolution Preview 14

Linux: make CityEvolution-Preview-14-Linux-x86_64.AppImage executable and launch it. Native Wayland and X11 are enabled. SDL selects the available desktop; to explicitly request Wayland, launch with SDL_VIDEO_DRIVER=wayland. FUSE alternative: --appimage-extract-and-run.

Linux saves: ~/.local/share/city-evolution. Settings: ~/.config/city-evolution (or the corresponding XDG locations). Back up existing saves before trying this experimental build. Audio uses the desktop's PulseAudio-compatible server, with host ALSA fallback; the AppImage does not bundle Ubuntu's libasound or libpulse.

Windows: extract the entire ZIP to a writable directory and launch Play.cmd. Startup log and saves: PortableData. Verify-Simulation.cmd runs the six simulation diagnostics. This cross-compiled Windows build has not been run on a physical Windows PC.

The top-right Buildings button switches building/tree models on and off. It leaves transport and ground visible. Flu controls are in City Hall > Budget / public services (F7). Flu is optional and saved; it is a small background simulation, not a pandemic-management game.

Existing menu layouts and artwork are retained. Women pedestrians and inventory-level props are included. Read CHANGELOG-Preview-14.md and the audit bundle for behavior, test evidence and limits. City Evolution is a fork of LinCity-NG; original licenses and credits are preserved.
