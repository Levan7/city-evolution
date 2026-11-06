# City Evolution — Preview 13

Based on Claude's supplied, audited Preview 12 source. Experimental integration of the prepared art and supporting public services. Core play remains city building, economy and transport.

## Existing menus preserved

The audited construction drawers, minimap, speed controls, Settings, New world, Load, Transport report, and City Hall page layouts remain intact. The main-menu version label is updated. A new `Budget / public services` button shares the City Hall footer with `Back to my city`; F7 is an additional shortcut. Public services uses its own window. No new construction controls were added to the Transport report.

`tests/menu_preservation_check.py` checks the original menu hashes and City Hall page trees. The new window has a build registration and the required stylesheet line. New building buttons use actual building thumbnails. The original Biscuit Maker Games logo is included in Credits; upstream credits and licenses remain.

## Art and night

- Installed Complete Art 03 daylight bindings into all 95 supplied resource families without changing their existing animation slot counts.
- Compact, varied homes and workshops; reduced industrial models; adjusted vehicle/human scale. Pedestrians now use the prepared walking sprites.
- Ground and yard details cover the supplied building footprints, addressing the black/empty wedges in the screenshots. Existing terrain and some bridge/effect art remains upstream.
- Updated Help images and building selector images to the prepared art.
- Aligned animated smoke to the new coal-plant and light-industry chimney tips.
- Integrated the prepared small window/lantern light masks, drawn immediately after their parent sprite in the same depth slot. Terrain, roads, trees, actors and vehicles darken; the HUD remains readable.
- Powered models require stored electricity or the residence's powered flag for electric lights. Lantern/candle models do not require electricity. Empty residences have no window lights. Existing fires remain emissive. Evening-only extra windows fade later at night.
- Added six optional late-game towers on 3×3 lots: ivory, brick and sand apartments, plus glass, bronze and Art Deco offices. Apartments hold 600 people, cost $60,000; offices cost $85,000. All require 60% technology. Offices exchange labor, water and electricity for service output represented by the native goods network.
- Added a compact police station and four-direction fire-engine, ambulance, police-car and historical pump-wagon sprites. Emergency lights use small runtime accents. Police windows have a small separate light mask.
- The public building name is **Church**. Its artwork and compatible internal resource/save identifiers remain unchanged.

Night adaptation applies to the world scene; nature does not receive fabricated windows. The existing animations are retained; this does not create new skeletal animations for every vehicle or building. Full duplicated night building textures are not shipped.

## Budgets, taxes and mayor

The new window controls existing income tax, goods tax and social support rates, and funding for education, healthcare, fire and police. Funding levels cycle through 0, 50, 75, 100, 125 and 150%. Reduced funding reduces operating days; running costs scale with funding. The school running cost now actually debits cash through the same expense mechanism used by other services.

The existing rule-based mayor, on/off switch, area/district/whole-map selection, permissions, pace, annual construction limit and plans are retained. Added needs include local food production/distribution, goods production, police and housing capacity. Placement still uses the normal construction and affordability rules and does not demolish player buildings.

Optional automatic taxes and service funding operate only while the mayor is ON. Taxes respect the selected ceiling. Automatic funding moves gradually and considers the cash reserve and illness. The displayed adjustable reserve applies to fiscal automation; construction retains the existing separate $25,000 reserve and yearly mandate limit. This mayor is a deterministic city-management policy, not an LLM or a general optimal planner. It connects isolated buildings; it does not yet design a global congestion-relief road network.

## Illness, education and police

The base game already had pollution-related mortality. Added a separate, saved pollution-illness count that reduces available household labor and residential desirability. Cleaner air and supplied healthcare support recovery. This does not apply a second mortality penalty. Education improves gradually from operating schools/universities; it lowers incident risk and improves office productivity. Employment, approval, social support and staffed policing also affect incident risk.

**Contagious flu/COVID transmission is not implemented.** No quarantine or epidemic-management game loop is claimed. These services support the city economy and operate automatically.

## Actual emergency dispatch

- Fire stations, health centres and police stations assign real crews to incidents. Rendered vehicles represent saved assignments.
- Crews can cross the map, rather than stopping at a coverage radius. Roads are faster; congested roads add delay; open land and trees are slower. Water needs a usable crossing. Unrelated building footprints block routes.
- Staffing, supplies, budgets and available crews matter. Funding at 100% supports two concurrent crews per station; 150% supports three.
- New building fires allow a rescue interval of 600 simulated days. Burning buildings suspend production, with residents still counted. If a crew arrives and suppresses the fire, the building survives. Unanswered fires eventually enter the original destroyed-building fire behavior.
- Native fires are suppressed by arrived crews; the old remote coverage suppression bonus was removed. Existing prevention/coverage bookkeeping remains.
- Crews travel back, reroute around invalidated paths, and clear assignments when incidents or stations disappear. State and paths are bounded and validated on load. Incident/crew limits are 64/128.

This is a first integration and needs longer playtesting on large, crowded maps. Incident routing and economic coefficients are not claimed fully balanced.

## Rare zombie Easter egg

Enabled by default, with a labelled OFF switch. A trigger requires at least 100 residents, at least half exposed to pollution above 1,500, sustained exposure for 24 months, a city age of at least five years and a rare monthly roll (1 in 240 after those conditions). Cooldown is ten years. The small event does not spread or demolish the city. Police handle it through ordinary dispatch; it expires after a bounded interval. Turning it off clears active events.

The notice says: “Your city was so polluted that zombies appeared. S.T.A.R.S. report for duty!” The tiny green incident markers are a prototype visual, not a new combat or zombie-animation system.

## Saves and packaging

Older Preview 12 saves remain loadable. New service state is optional in older saves and parsed after the actual map size is known. Fixed a related existing bug where underground/elevated cells beyond the default map dimensions could be lost when loading large maps. Cities saved with the new groups/state require Preview 13; keep copies of older saves if you want to return to Preview 12.

Windows portable: extract the whole ZIP and run `Play.cmd`. `PortableData/launch.log` records startup; `Verify-Simulation.cmd` runs the five bundled diagnostics.

Linux: x86_64 AppImage built on Ubuntu 24.04 (glibc 2.39 baseline), with SDL3 and runtime libraries bundled. Uses X11/XWayland. Make it executable and run. If FUSE is unavailable, add `--appimage-extract-and-run`. Saves use `~/.local/share/city-evolution` and configuration uses `~/.config/city-evolution` (or corresponding XDG directories).

## Validation

Five native simulation programs passed, including 168,536 integration assertions and 19 generated-world checks in the existing main suite. New checks cover road/off-road routing, fire rescue/failure, police/ambulance assignments, budgets, education, illness/recovery, rare event/off switch, state validation, route save/load, large-map transport layers and towers.

GUI style, assets/frame counts and protected-menu checks passed. The native Linux game and actual AppImage were opened on a virtual X display; new-world creation, load, public-service tabs, City Hall, Credits and night lighting were inspected. Ogg music decoding passed with a dummy audio device. Physical speakers were not tested.

Windows was cross-compiled and its packaged EXE/DLL imports audited. **This release was not run on a real Windows PC or under Wine.** Fedora hardware, physical GPU/audio and long-session balance remain untested.
