# Preview 12 (audit and clean-up of Preview 11)

## Fixed
- **Source could not build its own data.** `buttonpanel.xml` and `speedpanel.xml` had lost their `xml-stylesheet` line, so the normal build stopped with "document is not a stylesheet". Restored, and `tests/gui_style_check.py` now fails if any GUI file lacks it.
- **City Hall.** The pages had been wrapped in scrolling views with large gaps; text ran past the window and under the "Back to my city" button. Restored the compact fixed layout (Overview, Districts, Plan, Mayor, Sky) in the Preview 11 colours. Overview again has "Rename the city" and "All advice (what to fix)".
- **Minimap.** Unselected tabs were pale grey with near-white text (unreadable). Tabs are now dark with white text, captions fit, and the frame behind the minimap is transparent again.
- **Tool drawers.** Every drawer painted a 320x360 panel even for three buttons. Drawers are now as tall as their buttons.
- **Church.** Month-end counters (operating days) only updated on a day the church was open. They now update every month-end. Regression test added.
- **Mayor plans.** Removed a second permission check added in Preview 11. It blocked roads and the technology campus whenever "Basic services" was blocked. The plan already respects the services and parks switches correctly.
- Version label is Preview 12 (window title and main menu).

## Removed from the source
Three patch files, the stale `SHA256SUMS.txt`, old validation logs, 60 unused Preview 10 tile images (12 MB), an obsolete asset check that contradicted the 52-tool layout. Old notes moved to `doc/history/`. Art masters moved to a separate archive.

## Verified
Four simulation test programs (all pass; logs in `doc/audit/`), GUI style check (passes; also proven to fail when the stylesheet line is removed), asset check. Windows build run under Wine through `Play.cmd`: loads a city, City Hall and menus draw correctly. Not tested on a real Windows PC; audio not tested.
