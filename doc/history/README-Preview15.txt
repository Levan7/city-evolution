City Evolution — Preview 15
By Levan | studio76@mail.com
A fork of LinCity-NG; original credits and licensing retained.

Windows: extract the whole ZIP to a writable folder, then run City Evolution.exe.
Keep Game and Settings beside it. Saves/settings/log: PortableData.
Linux: mark CityEvolution-Preview-15-Linux-x86_64.AppImage executable and run it.
It selects Wayland or X11 through SDL; PulseAudio/PipeWire compatibility and ALSA
fallback use the host's audio libraries. If FUSE is unavailable, use
./CityEvolution-Preview-15-Linux-x86_64.AppImage --appimage-extract-and-run
Saves: ~/.local/share/city-evolution; settings: ~/.config/city-evolution
(or the corresponding XDG directories). Back up existing saves first.
Requires a recent 64-bit Linux with glibc 2.39 or newer.

Mayor: choose area in Mayor, then enable Full autonomy in Mayor or F7 Budget.
Full control spends real available cash, honors technology and stays in its area.
It is a rolling rule-based planner, not a promise to outperform a human mayor.
Poor funds, unsuitable terrain, technology or tiny zones can prevent projects.
Turn the Mayor OFF to stop autonomous building; status reports explain delays.
The street actors are visual city life, not a full individual commuting model.

Source: contrib/build-ubuntu-2404.sh or contrib/windows-cross/build-windows.sh.
Enable LINCITYNG_EVOLUTION_CHECKS to build the eight simulation diagnostics
and audio-checks. Run tests/preview15_ui_check.py, gui_style_check.py,
menu_preservation_check.py and preview11_assets_check.py.
Detailed changes: CHANGELOG-Preview-15.md; evidence: doc/AUDIT-Preview-15.md.
