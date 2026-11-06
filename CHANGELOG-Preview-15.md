# City Evolution — Preview 15

2026-11-06. Changes from the complete Preview 14 source; Preview 12's repaired menu layouts remain the basis.

- Replaced the added street actors' local random walk with destination-based shortest paths. Trips finish on arrival, destinations vary, and broken streets trigger replanning. Reject isolated two-tile fragments. Original commodity vehicles now avoid their last 12 positions and check map bounds before reading tiles.
- Added explicit full mayor autonomy in both Mayor and Budget / public services. It can establish a settlement on empty/wooded land, connect ordinary paid construction, reserve future street corridors, assess supplies/jobs/housing/services, respect a district or drawn zone, and persist its control state. Adaptive reserves replace the limited mayor's fixed $25,000 lock. Tax/funding automation also considers the prior year's operating deficit.
- Packed the minimap, overlays, zoom/map controls and up/down arrows into a compact lower-right panel. Removed the duplicate top-right building visibility button. Existing lower visibility control remains.
- Dark resource statistics replace the white panel. Tab captions fit the compact buttons.
- Police moved to Services; three residential towers to Homes; three office towers to Industry. Construction keeps normal technology and price rules. Removed the separate Budget building catalogue.
- Church has a native 'Church' caption, with no denominational name in the active English interface/Help. The church model is unchanged.
- Every left construction entry now has an illustrated English Help page. Repaired unsupported image tags and made document image bounding dimensions effective. Scrollbars hide when the document fits and remain for overflowing content.
- Removed the hidden event's public settings toggle and explanatory Help spoilers; the event simulation and default remain.
- Added a native, icon-bearing Windows 'City Evolution.exe' portable launcher, with version metadata and startup logging. Play.cmd is no longer the launcher. Added City Evolution by Levan Chelidze, studio76@mail.com, to About / Credits; upstream and Biscuit Maker credits remain.
- Retained Preview 14's women pedestrians, flu/recovery, stock props, activity/animation frames, night overlays, emergency services, UI scaling and audio failure handling. Linux package supports native Wayland/X11, automatic PulseAudio and the public host ALSA SONAME.

Validation: all eight simulation diagnostics, GUI/style/layout/art checks, destination-route cases, ten-year autonomous simulation, graphical city/menu walkthrough and packaged native Wayland/audio/volume test passed. Windows cross-build and DLL closure/resource checks passed. Physical Windows and the user's Fedora desktop have not been tested here. See doc/AUDIT-Preview-15.md for the evidence and limits.
