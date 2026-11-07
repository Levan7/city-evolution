City Evolution — Preview 16
By Levan Chelidze | studio76@mail.com
Fork of LinCity-NG; original credits/licenses retained.

Windows: extract the ENTIRE ZIP to a writable folder and run City Evolution.exe.
Keep Game and Settings beside it. Saves, configuration and startup log are in
PortableData. Back up your saves before updating; use a separate extracted folder.
Linux: mark the AppImage executable and run it. --appimage-extract-and-run works
without FUSE. Native Wayland/X11 supported; requires glibc 2.39 or later.

Management windows now PAUSE the city. Closing resumes the selected speed.
The election countdown is time remaining to the next election, not a population
counter. It decreases as simulation days pass, not because a people toggle costs
days. Plan focus can change while a plan awaits approval; active targets remain.

Budget (F7): Auto-adjust taxes works independently of building autonomy. When ON,
it reviews income tax monthly, at most one point per month and under your ceiling.
Repeated toggles do not speed it up. Manual tax changes turn it OFF. Auto funding
is separate and may change service funding monthly. Financial advisor explains
revenue/costs and suggests economic priorities without constructing anything.

Night uses the same building sprites with warm window overlays and street lamps.
No large duplicate day/night image sets. Empty/unpowered premises may stay dark.

The launcher and game are UNSIGNED. A trusted publisher certificate is needed
for identity signing; signing does not guarantee that Windows skips scanning a
new file. The reported Windows screenshot requests a scan, not a malware verdict.
Do not disable Defender for the game. No certificate or private key is included.

Read CHANGELOG-Preview-16.md and doc/AUDIT-Preview-16.md in the source for tests
and limits. The native Linux checks passed; no physical Windows PC was available.
