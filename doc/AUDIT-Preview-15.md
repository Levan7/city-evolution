# Preview 15 review report

## Scope and evidence
Built from the full Preview 14 source, not the earlier text-only patches. This release integrates the user's numbered UI corrections, autonomy work and the later reported horses/people circling in the supplied five-second screencast.

## Routing
`src/lincity/street_routes.*` builds a bounded BFS over connected road/track and bridge tiles. It selects a distant destination (preferably beside a building) from the reachable component and reconstructs a simple shortest path. Work is capped at 4,096 visited tiles / 64 steps. Different seeds vary destinations. Street actors follow that path, leave on arrival and replan after demolition. One- and two-tile isolated fragments do not generate endless traffic. Spawning is bounded and avoids occupied starting tiles.

The original commodity-driven vehicles retain their economy strategy, with a 12-position history to stop short circuits and bounds checks before map access. This is not a traffic congestion solver or an individual resident journey model; decorative actors do not carry simulated goods. Railway public-transport route logic was preserved.

`street-routes-checks` verifies contiguous on-street paths without repeated tiles, escape from a junction/loop, destination diversity over 64 seeds, road demolition, isolated fragments and bounds.

## Autonomous mayor
The full-control flag is saved. Enabling it uses the normal paid construction rules immediately, then reassesses regularly. Full control has separate adaptive reserves and six buildings / 30 street tiles per month rather than the limited mayor's fixed reserve and annual spending limit. Limited mode stays available. Master OFF stops spending.

The planner establishes water, food, distribution, housing and a goods chain; it connects isolated projects before duplicating services, reserves future street corridors, and considers demand, stocks, jobs, service coverage, technology, training, traffic and cash. Routes and complete building footprints stay inside the selected district/zone. It skips locked/unaffordable priorities instead of allowing one to block everything. Budget automation responds to previous-year operating deficits, subject to the player's tax ceiling. No free money, free residents, automatic loans or demolition of existing buildings.

A deterministic ten-year real simulation from bare land (80×80, ordinary $200,000 starting cash) ended with 175 residents, $122,823 cash, 31 constructions and technology 12,855. It built/maintained essentials and schooling. A separate wooded-zone bootstrap established four homes while every footprint stayed inside the zone. Save/load retained scope/full control; OFF and insufficient-funding cases passed.

This demonstrates a working settlement planner, not universal balance or human-level competition. Archipelagos, very small/poor zones, long-range logistics, zero startup funds and late-game expansion need more scenario testing. No claim that it outplays a skilled human mayor.

## Interface
Compact lower-right HUD (264×300 logical pixels), four readable tabs, dark statistics, unified bottom map controls and scroll arrows. Only Mayor/Transport report remain at top right. Police are under Services, residential towers under Homes, offices under Industry. Existing technology gating remains. The duplicate Budget development catalogue is gone.

Church's caption is native text over its current thumbnail, because the old wording was baked into the prior button picture. No church model or building sprite was changed. Active English church labels/Help contain no denominational name. Per-object Help illustrations use the Document parser's supported `img` tag. Document image width/height now bound natural size, so full-resolution models fit. Scrollbars appear only for actual overflow. The hidden event remains in the simulation but has no public toggle/spoiler text.

The repaired New world / Load / Save / Settings / speed / Transport layouts and non-Mayor City Hall pages are protected by preservation tests. Authorized changed files are explicit exceptions; old hashes have not been silently replaced. About retains upstream and Biscuit Maker credits and adds the requested name/email.

## Builds and runtime checks
- Native release build and all eight diagnostics: services, community, mayor, transit, city-evolution, flu, autonomy and street-routes passed.
- GUI style checks with and without NLS, table occupancy, asset/frame counts, construction/Help references, menu preservation and Preview 15 UI contracts passed.
- Actual graphical walkthrough loaded the ten-year city and opened Budget, Homes, Services, Industry, Civic, Church Help and dark Stats, then exited normally. Review screenshots are in the audit bundle. Short Church Help fits without a scrollbar; long documents retain scrolling.
- Audio diagnostics: 1,000 volume changes with a working dummy device, unavailable driver and failed ALSA setup; completion/track changes and shutdown passed.
- Final Linux package ran under native Wayland in Weston at 3072×1590 with logical scale 1.75, no DISPLAY and no development library paths. Automatic PulseAudio produced non-zero PCM (peak 19,725); 30 music and 30 effects '+' clicks saved both volumes at 100 and exited cleanly. Software rendering was used in this headless environment; hardware GPUs/physical Fedora are not certified.
- SDL 3.4.0 uses host `libasound.so.2`, host PulseAudio/Wayland/libdecor/XKB/graphics drivers, with no Ubuntu-specific audio plugin bundled. X11 fallback remains supported. Linux requires glibc 2.39+.
- Windows game and nine diagnostic executables cross-built. Root launcher is a native GUI executable with icon/version resources and static C++ runtime; all 26 PE files' DLL imports resolved to bundled or Windows system libraries. No physical Windows runtime was available. Portable paths are Unicode and based on the launcher directory; startup output goes to PortableData/launch.log.

## Source and review materials
The complete source is independently buildable using the supplied dependency recipes. SDL_mixer's rolling-main input is identified by archive SHA-256 in the dependency record. The audit ZIP supplements source with the precise changed-file overlay, binary-capable Preview14-to-15 patch, file hashes, logs, screenshots and packaging procedures. It is not another game installer. Optional high-resolution art masters remain separate, as in Preview 12. Existing night crops, women sprites and animation resource counts are unchanged from Preview 14.
