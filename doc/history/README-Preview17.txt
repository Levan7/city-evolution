City Evolution — Preview 17
By Levan | studio76@mail.com
Fork of LinCity-NG; original credits and licenses retained.

Windows: extract the whole ZIP into a NEW folder, run City Evolution.exe, and
keep Game/Settings beside it. Back up old PortableData before moving your saves.
Linux: mark AppImage executable and run. --appimage-extract-and-run avoids FUSE.
Native Wayland/X11 supported, glibc 2.39+ required.

Management windows pause the city. Close them to let simulation run.
Bus stops can touch dirt tracks or roads from any side. Staff and connection
are different requirements. Riders need a walkable home catchment and a reachable
job near a different connected stop. Longer journeys reduce effective service.
Windmills/substations can feed housing along connected streets, with distance
losses. Generation, staffing, stored electricity and supply capacity still matter.

Funding is service spending, not population/map coverage. Click a funding button
to advance by 5% (0 to 150%, then 0). Manual funding turns Auto funding OFF.
Auto-adjust taxes only delegates income tax. Full autonomy delegates construction
and initially enables tax/funding automation; you can override either separately.
Annual cap controls apply to limited mayor control; full control uses available
cash minus a reserve. The interface now labels that distinction.

The executable is unsigned. Windows may scan it. Physical Windows hardware has
not been tested here. Source, audit and CHANGELOG-Preview-17.md explain the checks.
