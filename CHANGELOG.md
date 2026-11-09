# Changelog

## City Evolution — Preview 19

- Bus stops, terminals, passenger stations, metro tracks, warehouses and power
  infrastructure can be built while the treasury is negative. The normal cost
  still comes out of the treasury. Technology requirements remain in place.
- The warehouse is available at zero technology. A dirt track or road works
  for local distribution. Rail is useful for moving freight between networks.
- Bus capacity is checked at both stops on a trip. City Advisors identifies
  crowded stops and suggests a terminal or additional stops.
- City Advisors explains how far below zero the treasury is, how much interest
  it paid, and which operating costs to review. It estimates repayment time
  when the city has a recurring surplus.
- Automatic service funding responds to operating deficits as well as low cash
  and protects healthcare funding during outbreaks.
- Nearby employers improve the appeal of housing with food and water.
  Housing reports and City Advisors flag food and water problems, and ore mines
  report worker shortages, full stock or exhausted deposits.
- Occupied homes without electricity and factories without electricity available show
  a small lightning icon. Home lighting uses the last actual supply result.
- Rain and storms have fading ambient sound. Selecting a church plays a bell.
  Street actors render at a higher cadence than building animations.
- Consolidated preview notes here and refreshed the project README. City
  Evolution credits use Levan.

## City Evolution — Preview 18 (audit of Preview 17)

### Fixed
- **Auto-adjust taxes did nothing visible.** The button only switched automation on. The automation then changed income tax by at most one point at the next monthly review, did nothing at all if a review had already happened that month, and treated any rate above its hidden 15% ceiling as an error to nudge down slowly. Now pressing it sets income tax to the financial advisor's recommended rate immediately (any month, any size of change), records the reason, and then keeps following the recommendation one point a month. A manual tax change switches automation off, as before.
- **Budget / public services window rebuilt.** It was 20 near-identical buttons with the values in a header line far from the controls, and funding buttons that could only count upward (to go from 100% to 50% you had to wrap around). Now three tabs (Taxes, Services, Automation); every value sits on the same row as its own minus and plus buttons; funding goes 0-150% in 5% steps both ways; the advisor's recommendation and the reason for the last automatic change are shown on the Taxes tab; the tax ceiling has its own minus/plus; amounts show thousands separators and a "last year" summary of income, costs and balance.
- **"Preview 15" in the Windows title bar.** The window title was hard-coded. There is now one file, `src/lincity-ng/release_label.hpp`, and `tests/release_label_check.py` fails if the window title, main menu, Windows version resource or changelog disagree.
- **Plan tab.** The "Change focus" button caption overflowed the button and nothing said what focus means. The caption is short ("Focus: Technology hub") and the page explains the three focuses, or that the focus is locked while a plan runs.
- **Windows package readme** was copied from a Preview 15 file. It is now a permanent `contrib/windows/README.txt` that also explains the "Windows protected your PC" message.

### Cleaned up
Six old changelogs, four old readmes, five old audit reports and a 1.1 MB patch file moved to `doc/history/` or removed from the source root. Fresh test logs in `doc/audit/`.

### Verified
Eleven simulation test programs pass (including 168,536 integration checks); the same eleven also run with no findings under AddressSanitizer and UndefinedBehaviorSanitizer. The five interface/asset Python checks pass. The new tax rules have their own tests (deficit, surplus, low cash, ceiling, immediate apply after a same-month review, monthly one-point step). The six newest simulation source files compile with `-Wall -Wextra` and no warnings.

## City Evolution — Preview 17

- Bus stops now recognise dirt tracks and track bridges as well as roads, matching early horse-carriage traffic. Every side of a stop/terminal footprint is checked. Unstaffed stops are reported separately from physically disconnected stops.
- Direct transit journeys select a reachable home-to-stop walk, connected street/track route and stop-to-workplace walk. Walking is bounded by catchment size and cannot cross water or unrelated buildings. Actual route length and both walks reduce effective ridership gradually; distance does not disconnect a valid network. Existing rail/metro/elevated layers and interchanges remain.
- Added a local low-voltage distribution abstraction along roads/tracks between windmills/substations and occupied housing. It transfers actual stored electricity, deducts distance losses, does not create power, respects breaks and branches, and rotates delivery priority daily. At about 100 street tiles, roughly half the dispatched power arrives. Existing direct/wired distribution is retained. High-voltage wind/solar generators still require the usual conversion equipment.
- Mayor annual cap controls now show the current dollar amount in limited control. Full autonomy explicitly says the annual cap is inactive and uses cash minus its adaptive reserve. Clicking the inactive cap controls explains this instead of changing an unused value.
- Public-service controls say Funding and advance in 5% steps from 0% to 150%, including 5%, 15% and 25%. Manual funding changes turn Auto funding off. Funding is not a geographical coverage percentage.
- Budget text distinguishes Full autonomy (construction/management), Auto-adjust taxes (income tax only) and Auto funding (service spending). Full autonomy enables both financial delegates initially; either can be overridden independently.

Validation: ten simulation diagnostics pass, including all four transit modes, interchanges, broken routes, opposite-side dirt-track stops, staff/connection separation, longer-trip penalties, blocked walking, near/far power loss, energy conservation, road cuts and 5/15/25 funding. Ten-year autonomous settlement still completes with positive cash. GUI/style/art/menu checks pass. Windows is cross-compiled and dependency-checked, not tested on physical Windows.

Known limits: journeys remain aggregate estimates rather than individual citizens; one-transfer walking/vehicle schedules are still approximate. Local road power supplements the legacy grid and is not an engineering simulation of voltage, impedance or congestion. A connected but unstaffed windmill with no stored power cannot supply homes.

## City Evolution — Preview 16

Windows feedback fixes, 7 November 2026.

- City Hall, Budget, Help and other management windows pause simulation while open. Closing restores the selected simulation speed without a catch-up burst. Controls for pedestrians and plan focus never change the date directly. Window titles identify the pause.
- Repaired the right HUD overlap: information pages are 240 pixels high inside a matching 336-pixel HUD; 14-point text stays readable, and page content cannot paint or take clicks outside its bounds. Navigation/overlay buttons retain separate rows.
- Plan focus shows the selected focus. Proposed plans cycle Livable city / Skills and education / Technology hub; active plans show a locked focus and explain that their approved targets remain in effect.
- Added highlighted Auto-adjust taxes and Financial advisor controls. Automatic income tax works independently of mayor building autonomy. Enabling performs one review; further reviews occur monthly, at most one percentage point per month, under the player's ceiling. Repeated toggles cannot accelerate adjustments. Manual tax changes disable automation. Goods taxes and service funding have separate controls. Review history survives save/load.
- Financial advisor reports prior-year operating income/costs, excludes construction, flags deficits/social spending/low reserves, and recommends connecting jobs and supplying idle workplaces rather than blindly adding buildings. Advice does not secretly construct or cut services.
- Brighter ambient night rendering, stronger window lighting, reduced late-night fade, dim lantern fallback in occupied housing without electricity, and warm lamps along settled streets. No duplicate night sprite sets or new large image assets. Empty housing and unpowered electrical services can remain dark.
- Fixed a fresh Git checkout build issue: libxml++ configuration is generated in the build tree rather than depending on an ignored generated header.

Validation: nine simulation diagnostics including finance, GUI style/layout/art checks, native graphical walkthrough, advisor interaction and date-pixel pause check. Windows cross-built, native launcher resource/dependency checked. Windows physical hardware and the user's exact save have not been reproduced here. No claim of a signed Windows executable.

## City Evolution — Preview 15

2026-11-06. Changes from the complete Preview 14 source; Preview 12's repaired menu layouts remain the basis.

- Replaced the added street actors' local random walk with destination-based shortest paths. Trips finish on arrival, destinations vary, and broken streets trigger replanning. Reject isolated two-tile fragments. Original commodity vehicles now avoid their last 12 positions and check map bounds before reading tiles.
- Added explicit full mayor autonomy in both Mayor and Budget / public services. It can establish a settlement on empty/wooded land, connect ordinary paid construction, reserve future street corridors, assess supplies/jobs/housing/services, respect a district or drawn zone, and persist its control state. Adaptive reserves replace the limited mayor's fixed $25,000 lock. Tax/funding automation also considers the prior year's operating deficit.
- Packed the minimap, overlays, zoom/map controls and up/down arrows into a compact lower-right panel. Removed the duplicate top-right building visibility button. Existing lower visibility control remains.
- Dark resource statistics replace the white panel. Tab captions fit the compact buttons.
- Police moved to Services; three residential towers to Homes; three office towers to Industry. Construction keeps normal technology and price rules. Removed the separate Budget building catalogue.
- Church has a native 'Church' caption, with no denominational name in the active English interface/Help. The church model is unchanged.
- Every left construction entry now has an illustrated English Help page. Repaired unsupported image tags and made document image bounding dimensions effective. Scrollbars hide when the document fits and remain for overflowing content.
- Removed the hidden event's public settings toggle and explanatory Help spoilers; the event simulation and default remain.
- Added a native, icon-bearing Windows 'City Evolution.exe' portable launcher, with version metadata and startup logging. Play.cmd is no longer the launcher. Added City Evolution by Levan, studio76@mail.com, to About / Credits; upstream and Biscuit Maker credits remain.
- Retained Preview 14's women pedestrians, flu/recovery, stock props, activity/animation frames, night overlays, emergency services, UI scaling and audio failure handling. Linux package supports native Wayland/X11, automatic PulseAudio and the public host ALSA SONAME.

Validation: all eight simulation diagnostics, GUI/style/layout/art checks, destination-route cases, ten-year autonomous simulation, graphical city/menu walkthrough and packaged native Wayland/audio/volume test passed. Windows cross-build and DLL closure/resource checks passed. Physical Windows and the user's Fedora desktop have not been tested here. See doc/AUDIT-Preview-15.md for the evidence and limits.

## City Evolution — Preview 14

Preview 14 builds on the audited Preview 12 menus and Preview 13 services. It fixes Linux audio failure handling and UI scaling, enables native Wayland, adds women pedestrians and optional flu, and makes building inventory changes visible. Existing save files are supported; keep backups before saving with new state. This remains an experimental release.

### Linux audio and crash handling

The supplied crash log reported an ALSA PipeWire plugin at an Ubuntu-specific path on Fedora. When mixer creation failed, the previous Sound constructor left track pointers uninitialized; volume controls could then dereference them. Tracks and mixer handles now start null, initialization failures leave a usable silent game, volume values are clamped and setters only touch valid tracks. Cleanup destroys the actual mixer in the correct order. The game releases Sound before the final SDL_Quit; a real packaged-game exit test exposed a separate double-free when that order was reversed.

Music completion now queues a request for the main thread; the audio callback does not load/destroy music files. Sound assets load synchronously, avoiding a loader/playback data race. Settings report when no audio device is available.

The Linux SDL build enables PulseAudio-compatible desktop audio with ALSA fallback. Host libpulse/libasound are used, rather than packaging Ubuntu's plugin lookup paths. Fedora PipeWire normally supplies the PulseAudio-compatible service. These changes do not install or reconfigure the user's sound server.

### Native Wayland and display scaling

SDL is built with Wayland, libdecor and X11 support. The launcher does not force X11. SDL selects an available desktop; explicit native Wayland can be requested with `SDL_VIDEO_DRIVER=wayland`. Wayland client, libdecor, xkbcommon, EGL and audio integration use the host desktop libraries. Hardware graphics drivers are not bundled.

The existing UI uses one logical size and matching mouse-coordinate conversion. At 3072×1590 the automatic UI scale is 1.75; at 1920×1080 and 1280×800 it is 1.0. Window dimensions saved in configuration use window units rather than framebuffer pixels. The legacy explicit OpenGL rendering path retains its old scale; the AppImage uses the SDL renderer by default. Window/renderer creation failures now produce an error instead of proceeding with null handles.

The audited menu layouts, colors, City Hall pages, category drawers and minimap artwork are retained. There is no Android build in the supplied source to compare directly; the readable supplied reference and existing menus guide this fix.

### Building visibility and dynamic artwork

The previous top-right button only restored visible models; clicking while already visible had no effect. It now toggles hide/show and changes its label. H uses the same state. Ground, roads, tracks, power lines and fire remain visible; building/tree models and their attached details hide together.

Original construction resource frame counts and working-state animations are retained. Farms, mines, workshops and windmills continue to choose frames from simulation state. Some market states used the same base image: new low/medium/full stock props now make inventory changes visible. Markets use their existing market ratio; warehouses display goods; ore/coal mines display their material; selected workshops/industries display output stock. Empty inventory has no stock prop. This is not an inventory overlay for every possible material or a new animation rig.

The new 12 small transparent stock sprites keep a consistent scale per material. They follow parent visibility, depth ordering and night tint. Health/fire/police window lights also respect active/ready service state.

### Pedestrians

Added 16 transparent woman walking sprites: four directions, four frames each. A stable mix of women and existing men uses the existing street-actor paths and pedestrian scale. This changes presentation, not employment/population gender rules. The raw generated atlases and export script are included for review.

### Optional flu

Flu is separate from existing pollution illness. Household cohorts model exposure, incubation, infectiousness, recovery and temporary immunity. Updates follow simulated time, not wall time. Local contacts and shared surface transport networks can spread infection; supplied healthcare and education lower its impact. Ambulance treatment supports recovery. Illness reduces available household labor and desirability without adding another mortality penalty.

The prototype uses 10 simulated days of incubation, 30 days infectious and 180 days immunity. Import rolls start after day 600 in cities with at least 100 residents; winter increases spread. Infections and notices are bounded to keep this a supporting mechanic. These are game tuning values, not a medical model or COVID-specific simulation. No quarantine, vaccine or pandemic-management system is claimed.

City Hall > Budget / public services (F7) has an ON/OFF switch and flu counts. Turning it off clears flu. State and the random generator are saved; older saves without flu state use defaults. The existing polluted-city zombie Easter egg, police, emergency dispatch, budgets and mayor permissions remain from Preview 13.

### Packaging and validation

Linux x86_64 AppImage: Ubuntu 24.04/glibc 2.39 baseline. Make executable and run; `--appimage-extract-and-run` avoids a FUSE requirement. Saves/configuration use the city-evolution XDG directories.

Windows x64: extract the whole ZIP and run Play.cmd. PortableData/launch.log records startup. Verify-Simulation.cmd runs the six simulation diagnostics. Cross-compilation/import checks do not replace a real Windows runtime test.

Six simulation programs, real audio integration tests, original-vs-fixed failed-ALSA regression, asset checks and protected-menu checks are provided. Recovery logs from the earlier session are marked separately from fresh rebuild evidence. Full results and platform limitations are in doc/AUDIT-Preview-14.md and the accompanying audit archive.

Physical Fedora speakers, a real Windows PC and Android runtime were not available for validation. Flu spread, service behavior and large-city performance need extended gameplay balancing. No claim of flawless hardware support is made.

## City Evolution — Preview 13

Based on Claude's supplied, audited Preview 12 source. Experimental integration of the prepared art and supporting public services. Core play remains city building, economy and transport.

### Existing menus preserved

The audited construction drawers, minimap, speed controls, Settings, New world, Load, Transport report, and City Hall page layouts remain intact. The main-menu version label is updated. A new `Budget / public services` button shares the City Hall footer with `Back to my city`; F7 is an additional shortcut. Public services uses its own window. No new construction controls were added to the Transport report.

`tests/menu_preservation_check.py` checks the original menu hashes and City Hall page trees. The new window has a build registration and the required stylesheet line. New building buttons use actual building thumbnails. The original Biscuit Maker Games logo is included in Credits; upstream credits and licenses remain.

### Art and night

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

### Budgets, taxes and mayor

The new window controls existing income tax, goods tax and social support rates, and funding for education, healthcare, fire and police. Funding levels cycle through 0, 50, 75, 100, 125 and 150%. Reduced funding reduces operating days; running costs scale with funding. The school running cost now actually debits cash through the same expense mechanism used by other services.

The existing rule-based mayor, on/off switch, area/district/whole-map selection, permissions, pace, annual construction limit and plans are retained. Added needs include local food production/distribution, goods production, police and housing capacity. Placement still uses the normal construction and affordability rules and does not demolish player buildings.

Optional automatic taxes and service funding operate only while the mayor is ON. Taxes respect the selected ceiling. Automatic funding moves gradually and considers the cash reserve and illness. The displayed adjustable reserve applies to fiscal automation; construction retains the existing separate $25,000 reserve and yearly mandate limit. This mayor is a deterministic city-management policy, not an LLM or a general optimal planner. It connects isolated buildings; it does not yet design a global congestion-relief road network.

### Illness, education and police

The base game already had pollution-related mortality. Added a separate, saved pollution-illness count that reduces available household labor and residential desirability. Cleaner air and supplied healthcare support recovery. This does not apply a second mortality penalty. Education improves gradually from operating schools/universities; it lowers incident risk and improves office productivity. Employment, approval, social support and staffed policing also affect incident risk.

**Contagious flu/COVID transmission is not implemented.** No quarantine or epidemic-management game loop is claimed. These services support the city economy and operate automatically.

### Actual emergency dispatch

- Fire stations, health centres and police stations assign real crews to incidents. Rendered vehicles represent saved assignments.
- Crews can cross the map, rather than stopping at a coverage radius. Roads are faster; congested roads add delay; open land and trees are slower. Water needs a usable crossing. Unrelated building footprints block routes.
- Staffing, supplies, budgets and available crews matter. Funding at 100% supports two concurrent crews per station; 150% supports three.
- New building fires allow a rescue interval of 600 simulated days. Burning buildings suspend production, with residents still counted. If a crew arrives and suppresses the fire, the building survives. Unanswered fires eventually enter the original destroyed-building fire behavior.
- Native fires are suppressed by arrived crews; the old remote coverage suppression bonus was removed. Existing prevention/coverage bookkeeping remains.
- Crews travel back, reroute around invalidated paths, and clear assignments when incidents or stations disappear. State and paths are bounded and validated on load. Incident/crew limits are 64/128.

This is a first integration and needs longer playtesting on large, crowded maps. Incident routing and economic coefficients are not claimed fully balanced.

### Rare zombie Easter egg

Enabled by default, with a labelled OFF switch. A trigger requires at least 100 residents, at least half exposed to pollution above 1,500, sustained exposure for 24 months, a city age of at least five years and a rare monthly roll (1 in 240 after those conditions). Cooldown is ten years. The small event does not spread or demolish the city. Police handle it through ordinary dispatch; it expires after a bounded interval. Turning it off clears active events.

The notice says: “Your city was so polluted that zombies appeared. S.T.A.R.S. report for duty!” The tiny green incident markers are a prototype visual, not a new combat or zombie-animation system.

### Saves and packaging

Older Preview 12 saves remain loadable. New service state is optional in older saves and parsed after the actual map size is known. Fixed a related existing bug where underground/elevated cells beyond the default map dimensions could be lost when loading large maps. Cities saved with the new groups/state require Preview 13; keep copies of older saves if you want to return to Preview 12.

Windows portable: extract the whole ZIP and run `Play.cmd`. `PortableData/launch.log` records startup; `Verify-Simulation.cmd` runs the five bundled diagnostics.

Linux: x86_64 AppImage built on Ubuntu 24.04 (glibc 2.39 baseline), with SDL3 and runtime libraries bundled. Uses X11/XWayland. Make it executable and run. If FUSE is unavailable, add `--appimage-extract-and-run`. Saves use `~/.local/share/city-evolution` and configuration uses `~/.config/city-evolution` (or corresponding XDG directories).

### Validation

Five native simulation programs passed, including 168,536 integration assertions and 19 generated-world checks in the existing main suite. New checks cover road/off-road routing, fire rescue/failure, police/ambulance assignments, budgets, education, illness/recovery, rare event/off switch, state validation, route save/load, large-map transport layers and towers.

GUI style, assets/frame counts and protected-menu checks passed. The native Linux game and actual AppImage were opened on a virtual X display; new-world creation, load, public-service tabs, City Hall, Credits and night lighting were inspected. Ogg music decoding passed with a dummy audio device. Physical speakers were not tested.

Windows was cross-compiled and its packaged EXE/DLL imports audited. **This release was not run on a real Windows PC or under Wine.** Fedora hardware, physical GPU/audio and long-session balance remain untested.

## City Evolution — Preview 12 (audit and clean-up of Preview 11)

### Fixed
- **Source could not build its own data.** `buttonpanel.xml` and `speedpanel.xml` had lost their `xml-stylesheet` line, so the normal build stopped with "document is not a stylesheet". Restored, and `tests/gui_style_check.py` now fails if any GUI file lacks it.
- **City Hall.** The pages had been wrapped in scrolling views with large gaps; text ran past the window and under the "Back to my city" button. Restored the compact fixed layout (Overview, Districts, Plan, Mayor, Sky) in the Preview 11 colours. Overview again has "Rename the city" and "All advice (what to fix)".
- **Minimap.** Unselected tabs were pale grey with near-white text (unreadable). Tabs are now dark with white text, captions fit, and the frame behind the minimap is transparent again.
- **Tool drawers.** Every drawer painted a 320x360 panel even for three buttons. Drawers are now as tall as their buttons.
- **Church.** Month-end counters (operating days) only updated on a day the church was open. They now update every month-end. Regression test added.
- **Mayor plans.** Removed a second permission check added in Preview 11. It blocked roads and the technology campus whenever "Basic services" was blocked. The plan already respects the services and parks switches correctly.
- Version label is Preview 12 (window title and main menu).

### Removed from the source
Three patch files, the stale `SHA256SUMS.txt`, old validation logs, 60 unused Preview 10 tile images (12 MB), an obsolete asset check that contradicted the 52-tool layout. Old notes moved to `doc/history/`. Art masters moved to a separate archive.

### Verified
Four simulation test programs (all pass; logs in `doc/audit/`), GUI style check (passes; also proven to fail when the stylesheet line is removed), asset check. Windows build run under Wine through `Play.cmd`: loads a city, City Hall and menus draw correctly. Not tested on a real Windows PC; audio not tested.

## City Evolution — Preview 11
4 November 2026. A fork of LinCity-NG.

This release contains a newly compiled native Windows executable and integrates the church/traffic/UI work with the later request for lower, more varied buildings.

### Artwork and road visibility

The screenshots first showed the original hide-tall-buildings view: colored diamonds replace structures larger than one tile while a one-tile bus shelter remains visible. H toggles that view. The subsequent screenshots confirmed the models were present and showed that the previous palace-like replacements were oversized and repetitive.

- Restored low, utilitarian silhouettes based on the original game.
- Three forestry-commune appearances: small huts, wood/charcoal work areas and trees instead of repeated large courtyard factories.
- Twelve housing appearances: two variants for each of the six tiers. Timber cottages, stone houses, brick terraces, white/pastel bungalows and compact three-storey apartments have different materials and colours.
- Distinct low pottery kilns, a forge, a brick textile mill and market stalls.
- Farms are mainly crop fields with a small barn. Crop-state slots remain.
- All replacement building sprites are drawn at 80% of their prepared model scale over original-size native grass lots. This reduces their visual area to 64% without changing construction footprint, costs, commodity rules or road connectivity.
- Building thumbnails now show the current, smaller in-game models.
- Commune/housing variations are derived from map position; fixed-coordinate save/load retains the appearance without new save fields.
- The original animation slot counts remain. Most animated replacements use three painted poses in the original timing slots. Smoke sprites and their movement offsets are scaled with the factories.

The exact export dimensions, bindings and scale factors are in art/preview11/scale-metrics.json and compact-resource-layout.json. Art masters and generation prompts are supplied. The contact sheet is art/preview11/compact-review.jpg. The renderer still uses ordinary isometric depth ordering: a building can cover some ground behind it. This release reduces that obstruction; it does not promise every road segment is visible from every view.

### Menus and controls

- Restored the custom generated riverside city background, with menu buttons in its shaded left area.
- Restored one horizontal row of pause, green play, fast and fastest buttons, using the existing created dark teal theme.
- Mayor is at the top right, with Transport report directly below.
- Removed construction buttons and their callbacks from the transport report. Construction stays in the left categories.
- Kept 15 left categories and all previous 51 tool definitions; church is tool 52 in Civic.
- Categories stay openable when their default tool is locked; unavailable individual buildings remain disabled with requirement tooltips.
- Selecting tunnel/elevated construction from the sidebar automatically selects the corresponding transport layer view.
- Models reset visible when a world opens. A labelled button beneath Transport report restores model visibility; H remains the original toggle.
- Decorative details hide with their parent buildings. The long warehouse explanation is shortened to fit the fixed statistics panel.

### Horses, carriages and cars

The shared policy is src/lincity/traffic_era.hpp. Elapsed time alone does not unlock cars.

| Technology | Motor traffic share |
| --- | --- |
| Below 10% | 0%; horses and carriages |
| At 10% | 1% |
| 10% to 30% | Gradual deterministic transition |
| 30% or above | 100% |

The policy applies to native vehicles, newer street actors, parked warehouse detail and representative bus-route vehicles. Early bus routes display horse carriages. Parked trucks become carts before motorisation. There are four horse views and four carriage views. The native vehicle group retains 16 slots, reusing carriage views for turns/slopes. Vehicles move along built streets/tracks; empty wilderness has no ambient traffic.

This changes visual traffic eras, not commodity logistics, timetables, capacity or transport build technology requirements. Rail retains its own prerequisites. Horses/carts used as parked building details remain decorative. The moving sprites are not a detailed animal/driver economy or a full leg/wheel animation system.

### Georgian Orthodox Christian church

Find it in Civic. The model has Georgian stonework, a conical teal roof, a bell tower and visible Christian crosses.

| Rule | Value |
| --- | ---: |
| Construction | $6,000; 3×3 footprint |
| Technology/electricity requirement | None |
| Daily input | 2 labor + 1 goods |
| Daily upkeep, including closed days | $8 |
| Learning while operating | +1 technology point/day |
| Spiritual wellbeing radius | 8 Manhattan-distance tiles from footprint |
| Nearby residence morale | +8, non-stacking |
| Target city approval | Up to +6 according to covered housing share |

The church is an optional game amenity. No separate religious penalty applies when it is absent. It does not produce formal school/university graduates or satisfy their skilled-workforce prerequisites. Closed churches grant no learning or coverage benefit.

Operating counters and a separate expenditure ledger are saved. Active service is recalculated from supplies on the next simulated day. The compact Other Costs screen combines Schools/Church while retaining separate saved accounts. City Hall shows the homes covered and number of operating churches.

Old saves remain loadable. Saves containing churches require Preview 11 or later.

### Native code and audit coverage

New/changed code covers group 62 and group count 63, church registration/module, input consumption, education, coverage, residence morale, approval, spending, yearly totals, save/load, traffic progression, model visibility, report callbacks, layer selection, browsable categories, stable art variants and scaled smoke offsets. The game and four Windows simulation diagnostics are rebuilt.

The audit materials include full current engine and GUI source, tests, build notes, binary hashes, import/export results, active resource mappings, art previews and two textual patch stages:
1. The inherited Changes-from-Claude.patch records the earlier Claude Preview 8 → Preview 10 work.
2. Preview10-to-Preview11.patch records this release's changes.

The full Source ZIP supplies all binary artwork and third-party source. The focused hash report records binary changes as well. The previous baseline comparison is preserved as history; the combined current index records original-before and current-after hashes for changes since the supplied Claude archive. Patches alone are not a playable update.

### Verification

- Windows x64 UCRT game and four diagnostics compiled successfully.
- Native real-engine community tests passed for traffic thresholds, monotonicity, seed mixes, church construction/costs, staffing, upkeep, learning, coverage, wellbeing, save/load, closure without goods and demolition.
- Stable placement-based commune variant assignment passed.
- All four transit modes passed native regression tests: ridership, coverage, fares, upkeep, save/load, route cuts, staffing, interchange and independent layers.
- Source/package GUI style checks passed in both NLS branches.
- Asset checks passed: 52 tools, 15 categories, 42 table layouts, all original native animation slot counts, three commune variants, twelve housing appearances and 95 rescaled building frames.
- Windows PE dependencies and imported symbols passed, including the dynamic PNG/Vorbis/font libraries used by the shipped files.
- Release ZIP CRCs and per-file SHA-256 entries are checked.

Headless tests use inert display/audio adapters and do not test graphical interaction. Windows startup, final rendering alignment and audio playback have not been play-tested here. Isometric screenshots/contact sheets and static layout checks are not a Windows play-through.

### Remaining limits

Terrain, bridges, network tiles and some effects still use upstream art. Many working-building loops reuse three new poses rather than repainting every original pose. The complete source preserves original/input images for reproduction; the playable package removes unreferenced tile images and has one ordered binding per active resource.

### Install

Extract the complete Windows ZIP into a new writable folder and run Play.cmd. Keep personal saves backed up before copying them into PortableData. Do not mix old and new DLL folders. Verify-Community.cmd runs the church/traffic diagnostic. For startup failures, Diagnose-Startup.cmd records diagnostics; PortableData/launch.log contains startup output.

## City Evolution — Preview 10
4 November 2026

This revision rebuilds the construction sidebar, restores the original main-menu scene, and expands the consistent isometric artwork. It is a data/UI revision for the existing Preview 9 native engine. It is a fork of LinCity-NG.

### What changed in this revision

| Area | Result |
| --- | --- |
| Main menu | Original LinCity-NG city background restored. Only its title/logo group is replaced with City Evolution branding. Fixed spacing separates title, seven buttons and footer. |
| Construction sidebar | 15 compact categories on the left; one drawer opens at a time. All 51 original tools retain their actions, construction group IDs and help links. |
| Public transport | Separate Buses, Rail, Metro and Elevated drawers, plus Streets and Freight. The passenger report is an icon button on the left. Mayor stays at the top right. |
| Building choices | 45 choices show the actual in-game building, road or rail sprite. Metro/elevated track tools retain their route-layer diagrams; the four action tools retain action symbols. Categories use simple symbols. |
| Speed controls | Two rows of speed controls under the category rail, followed by City Hall and Menu / Save buttons. |
| Artwork | 22 new or revised sprite sheets: 21 building/facility/landscape sets and a 16-view native car. Combined with retained Preview 9 art, 43 distinct building/facility models have replacement art bindings. |
| Working buildings | Pottery, blacksmith, mill, commune, ore mine, heavy industry, wind turbine, fire station, school and sports use multiple new poses in their original native frame slots. |
| Effects and stages | Original gray/black smoke restored (10/11 frames), five native farm states, landfill fill levels, monument construction/finished states, and eight rocket states. |
| Old images | 157 unused replaced legacy/revision images removed from the playable package. Reproducible input assets remain in source. Each active resource has a single ordered binding. |
| Startup styles | Main menu explicitly defines boxtext and dialogbutton before importing the Load/Save screens, addressing the previously identified undefined-style startup failure. |
| Packaging | Complete portable game and complete source archives, with hash manifests and a manifest-based diagnostic script. |

New/revised art sets: pottery, blacksmith, mill, commune, ore mine, coal mine, landfill, harbor, sports field, monument, rocket complex, park, shanty, wind turbine, heavy factory, fire station, farm, school, park with lake, light factory, coal plant, native car.

### Animation details

Native timing code and frame counts have not been changed. Most working-building loops now repeat three distinct newly painted poses across their original slots. This preserves activity and timing, but is not a frame-for-frame repaint of every distinct upstream pose. School uses a shared full-building pivot for its animated overlay. A common canvas and pivot keep each building stable between poses. Native car frames preserve the original directional/slope order and per-frame placement. Light factory and coal-plant art follow the original geometry so their original smoke emitters remain usable.

The actual checked frame counts and distinct image counts are recorded in validation/preview10-assets.json. This is more precise than calling every new model fully animated.

### Compared with the Claude source you supplied

Baseline: city-evolution-Preview-8-source.tar(1).xz, extracted root city-evolution-Preview-8-source. The source package contains Changes-from-Claude.patch for textual engine/UI/build/script changes and validation/changes-from-claude.json for the complete path-by-path SHA-256 comparison, including binary assets. The patch requires the new binary artwork from this source package; it is not a standalone game update.

The mayor/advisor, district, world-generation and broader city systems already present in the supplied archive remain inherited work. This changelog does not claim they were created in Preview 10.

The earlier Preview 9 changes, retained here, include:

- Four passenger networks: bus, rail, underground metro and elevated transit; five station/stop facilities and two transit track construction types.
- Network reach, passengers and fares, residential transit desirability, save/load state and passenger report.
- Moving route vehicles and drawing of the underground/elevated layers.
- Expanded building art, transport sprites, readability changes, revised world/menu data and a stocked transport sandbox.
- Explicit integer casts in mayor-plan handling, permission checks for delegated park/service actions, and startup markers.

Native files changed relative to that supplied archive are listed below. Preview 10 retains these native implementations; this menu/art revision does not rebuild or alter the Windows executable.

- src/lincity/CMakeLists.txt
- src/lincity/city_evolution.cpp
- src/lincity/groups.hpp
- src/lincity/mayor_plan.cpp
- src/lincity/modules/CMakeLists.txt
- src/lincity/modules/all_modules.cpp
- src/lincity/modules/all_modules.hpp
- src/lincity/modules/public_transport.cpp
- src/lincity/modules/public_transport.hpp
- src/lincity/modules/residence.cpp
- src/lincity/simulate.cpp
- src/lincity/stats.cpp
- src/lincity/stats.hpp
- src/lincity/transit.cpp
- src/lincity/transit.hpp
- src/lincity/world.hpp
- src/lincity/xmlloadsave.cpp
- src/lincity-ng/Game.cpp
- src/lincity-ng/Game.hpp
- src/lincity-ng/GameView.cpp
- src/lincity-ng/GameView.hpp
- src/lincity-ng/MainMenu.cpp
- src/lincity-ng/Mps.cpp
- src/lincity-ng/UserOperation.cpp
- src/lincity-ng/main.cpp

### What is still incomplete or unverified

- Terrain, road/rail/powerline networks, bridges and disaster effects retain upstream graphics. This is not a replacement of every image in the engine.
- Horse, horse cart and delivery truck remain decorative. They are not independent simulated logistics systems.
- The passenger system is the inherited Preview 9 implementation. This revision reorganizes its controls and changes its facility artwork; it does not add a new simulation engine.
- The original native animation slot counts remain, with three-pose loops for many buildings as described above.
- The Windows window caption can still say Preview 9 because the native executable is retained. The main menu footer and package identify the data/UI revision as Preview 10.
- The provided screenshots for the latest menu complaint were not attached in this request; this revision follows the written requirements.
- Windows startup and interactive play have not been verified in this environment. Static GUI/import/resource/layout checks passed, but do not prove crash-free runtime behavior. The prior undefined-style failure is specifically addressed; unrelated runtime faults remain possible.
- Existing Preview 9 simulation logs included in the source are historical evidence. Those simulations were not rerun for this asset/UI revision.

### Verification and use

The checks preserve all 51 tool definitions, verify all native resource frame counts, check referenced images and the main-menu style import order, verify 40 revised table layouts for overlapping cells, and check sidebar/menu space budgets at 1024×720, 1280×800, 1366×768 and 1920×1080. The archive builder checks ZIP CRCs and every packaged SHA-256 entry. Windows interactive testing remains outstanding.

Extract the entire Windows ZIP into a new writable folder and run Play.cmd. Do not place the update over a mixture of older game directories. Continue city opens the bundled transport example; New world creates a fresh city. Personal saves can be copied into PortableData after extraction, with backups kept separately.

If startup fails, run Diagnose-Startup.cmd beside Play.cmd and provide its output together with PortableData/launch.log.

The source includes the complete engine, game data, final PNG masters, available generation prompts, exported frames/pivots, resource and thumbnail mappings, an art contact sheet, reproduction scripts and validation reports. Consult PREVIEW-9.md for the retained engine build instructions and dependencies.

## LinCity-NG 2.15.0

date: 2026-05-08

###### Gameplay
- Added a chance for waste burning to catch the building on fire.
- Increased fire probability for hospitals.
- Changed the crash site calculation for rockets.

###### User Interface
- Added partial language switching without restarting the game. (GUI components
  that were already translated are not (yet) re-translated into the new
  language. A restart is still required for that.) Notably, the pop-up dialog
  suggesting to the user to restart the game now uses the new language.

###### Internal
- Migrated from SDL2 to SDL3; requires SDL >= 3.2.0.
- Fixed Ninja build generation.
- Added gettext/libintl optional library dependency.
- Removed the `tinygettext` gettext implementation.
- Changed the installation of locale files to binary (.mo) format.
- Combined the source and GUI translation PO files into one.
- Migrated GUI module from libxml2 to libxml++.
- Added locale-independent formatting and parsing for XML.
- Added environment variables `LINCITYNG_CONFIG_FILE`, `LINCITYNG_APP_DATA_DIR`,
  and `LINCITYNG_USER_DATA_DIR` for setting the config file, app data, and user
  data locations respectively.

###### Documentation / Translation
- Reformatted the `--help` output, and added a brief command-line template.
- Added documentation on build configuration options.
- Added documentation on runtime config options.
- Moved dependencies list from README.md to a separate file.


## LinCity-NG 2.14.2

date: 2025-09-22

###### Gameplay
- Fixed coal mines mining from the wrong tiles.
- Fixed fires not spreading.

###### User Interface
- Fixed port load/save corruption of trade rules.
- Fixed map overlay updates not triggering re-draw.
- Changed the bulldoze warning to not issue when bulldozing isn't allowed
  anyway.
- Changed the app icon for the Windows version to match the (newer) icon for the
  Linux version.

###### Internal
- Fixed a segfault when saving a game with libxml2 <2.13.0.
- Added a build option to make a relocatable binary.
- Added NSIS installer for Windows relocatable builds.

###### Documentation / Translation
- Fixed the english fallback for help pages.


## LinCity-NG 2.14.1

date: 2025-08-07

###### User Interface
- Fixed an issue causing some UI components to appear distorted.
- Added smooth scrolling support for devices where it is available, e.g.
  touchpads.

###### Internal
- Added appstream metainfo.
- Added a CMake option `LINCITYNG_VERSION_SUFFIX` to allow overriding the
  version suffix. This is useful when packaging with patches or when building
  from a source tarball.
- Optimized size of image assets.
- Renamed the desktop file and icon to follow application ID naming.
- Added a 256x256 version of the icon (up-scaled from the existing one).


## LinCity-NG 2.14.0

date: 2025-07-04

###### Gameplay
- Fixed a bug that allowed uncompleted monuments to be bulldozed.
- Improved terrain/ecology generation algorithms.

###### User Interface
- Fixed grid-lines appearing on the map.
- Fixed duplicate click events in the new-game menu.
- Added warning dialogs when a construction cannot be built.
- Reworded most in-game pop-up dialogs.
- Fixed the "map size" configuration option so it no longer resets to 100 when
  loading a scenario.
- Added `--config`, `--app-data`, and `--user-data` command-line options for
  specifying custom paths.
- Removed support for restarting the game automatically. This means the user
  must restart the game manually if they change the language setting.

###### Internal
- Removed PhysFS library dependency in favor of C++17 Filesystem library.
- Added cfgpath library for detecting config and user data locations.
- Added [libfmt](https://fmt.dev/latest/index.html) library dependency for
  formatting translatable strings.
- Changed SDL2 dependency minimum version from 2.0.0 to 2.0.18.
- Removed SDL2_gfx library dependency.
- Changed the default `CMAKE_BUILD_TYPE` to **Release** instead of being
  system-dependent.
- Changed detection of app data location to use `CMAKE_INSTALL_PREFIX`. Note:
  `--install-prefix` may need to be specified at configure time with the
  expected install location; otherwise, the default app data directory will be
  incorrect.
- Added `appDataDir` and `userDataDir` config options for storing non-standard
  app data and user data locations.
- Changed format and location of config file.
- Added hardware-accelerated rendering in SDL video mode.
- Added hardware-accelerated blitting.
- Removed GL video mode.
- Fixed two memory leaks from paragraph textures.
- Improved separation between front-end and back-end logic.
- Removed many singletons from the code.
- Removed `error.h` dependency for Unix-based systems. This improves portability
  as not all Unix-based systems have the `error.h` header.

###### Documentation / Translation
- Fixed translatable phrases appearing as fragments to translators due to string
  concatenation.
- Added documentation about the CMAKE_BUILD_TYPE options to the readme.
- Added explanation of very-negative residence desirability to the help page.


## LinCity-NG 2.13.1

date: 2025-02-15

###### User Interface
- Fixed popup dialog text not showing.
- Fixed input lag in main menu.

###### Internal
- Fixed random crashing on Windows caused by a bad string termination.
- Fixed segfault when loading communes from a saved game.
- Fixed freeze on launch.
- Fixed freeze when starting a game.
- Fixed segfault and dangling thread when quitting the game in debug mode.


## LinCity-NG 2.13.0 [SECURITY]

date: 2025-01-26

###### Gameplay
- Fixed rocket pads resetting after save-load cycle.

###### User Interface
- Fixed the camera position when starting a game.
- Added popup dialog for game load errors
- Fixed lag and high CPU utilization in menus.
- Changed dialogs to be not resizable by default.
- Removed construction ID numbers.

###### Internal
- [SECURITY] Fixed a heap overflow in game loading.
- Changed the load/save format. (Games saved with previous versions of
  LinCity-NG are incompatible with the current version and vise-versa.)
- Added a script to convert games from the load/save format of version 2.12.x.
- Added libxml++ dependency for load/save parsing.
- Changed starting scenarios to current load/save format.
- Fixed an issue where games may not load correctly on a different platform than
  the one used for saving.
- Changed the C++ standard from C++11 to C++17.
- Added BetaTest and DebugOpt build configurations.
- Removed ~/.lincity-ng from the search path.

###### Documentation / Translation
- Added a contributing guide.
- Added Hungarian translation for GUI and source strings.
- Fixed formatting on information tabs help page.


## LinCity-NG 2.12.0

date: 2024-11-13

###### Gameplay
- Fixed tech requirement enforcement after tech decrease.

###### User Interface
- Changed rocket icon.
- Fixed duplicate/erroneous invention messages.
- Changed the left- and right- click actions for button panel menus. Now,
  left-click toggles the drawer, and right-click activates the shown tool.
- Fixed button-panel buttons displaying incorrect checked/unchecked/hovered
  state.
- Removed 'quickness' parameter from command-line options and config file.

###### Internal
- Deprecated binary load/save format.
- Deprecated old load/save format.
- Deprecated seed-compression load/save mode.
- Converted starting scenarios to current save format.
- Fixed simulation tick on load when paused.
- Fixed segfault while bulldozing.
- Removed many unused files from install.
- Changed GUI callbacks to use C++ std::function.
- Added a class to manage radio buttons.
- Cleaned up (fully rewrote) the button-panel internals.
- Moved button-panel tool definitions from the source code to the GUI XML.
- Changed PhysFS-SDL2 glue layer from custom implementation to physfswrops.
- Fixed repeated git merge conflicts in CHANGELOG.md.

###### Documentation / Translation
- Updated the controls help page.


## LinCity-NG 2.11.2 [SECURITY]

date: 2024-09-26 <br/>
tag: lincity-ng-2.11.2

###### Gameplay
- Fixed sustainability not resetting when dumping on a tip.
- Fixed tech-based output for farms and power stations.

###### User Interface
- Fixed cars not spawning on track/road/rail.

###### Internal
- [SECURITY] Fixed a heap overflow in game loading.
- Fixed an error raised by CPack for missing README.md file.
- Removed unneeded image source files from install.

###### Documentation / Translation
- Fixed the tip icon in help pages.
- Added Italian translation for .desktop file.
- Fixed several inconsistencies in translated help pages.
- Added a script for detecting inconsistencies in translated help pages.
- Fixed a typo in the rocket help page.


## LinCity-NG 2.11.1

date: 2024-07-19 <br/>
tag: lincity-ng-2.11.1 <br/>
git: f42e76c3

###### Internal
- Fixed crash when loading randomized scenario.
- Fixed a compile-time error in src/gui/event.cpp when building with
  SDL <= 2.26.0.
- Fixed a compile-time error in src/lincity/xmlloadsave.cpp when building with
  older g++ version.
- Fixed parsing boolean from XML save file.
- Fixed blacksmiths sometimes skipping monthly update. This was causing
  blacksmiths to display more than 100% busy.


## LinCity-NG 2.11.0

date: 2024-06-24 <br/>
tag: lincity-ng-2.11.0 <br/>
git: afaede3d

###### Gameplay
- Nerfed ports by randomly disabling import/export.
- Reduced coal tax by 40%.

###### User Interface
- Renamed MWh and kWh commodities to "high-voltage power" and
  "low-voltage power" respectively with short names "hi-volt" and "lo-volt".
- Renamed "jobs" commodity to "labor".
- Fixed failure to load sound effects.
- Made in-game windows and dialogs resizable by dragging the edges/corners.
- Changed date format in status bar.

###### Internal
- Fixed portability for Windows.
- Added support for cross-compiling from Linux to Windows with MinGW-w64. (Now
  Windows binaries can be included in releases.)
- Fixed "Invalid ExtraFrames" warning caused by waste fires.
- Fixed some warnings re. excessive production/consumption.
- Added error checking for loading sound effects.
- Removed messages.pot generation from normal build. This removes the gettest
  build dependency except for translators. Translators must now generate the
  messages.pot file with a separate build step.
- Fixed missing git describe in package versions that are generated with CPack.
- Fixed extraneous link libraries.
- Added Include What You Use as a build step, and cleaned up includes.
- Improved handling of waste fires, so they are less bug prone.
- Refactored lintypes.cpp/hpp and moved some symbols to other files.
- Switched internal libraries from object to static type.
- Cleaned up target link libraries.
- Add fatal check for null in getGameView.
- Fixed handling of window size in fullscreen mode.
- Fixed some typos.

###### Documentation
- Added production/consumption formulas for each module to help pages.

###### Known Issues
- Some translations may be outdated since renaming commodities.
- Production/Consumption formulas do not exist in translated help pages.
- Requires SDL >= 2.26.0
- Crashes when the first game loaded is a new randomized scenario.
- Blacksmiths may display more than 100% busy.


## LinCity-NG 2.10.2

date: 2024-04-19 <br/>
tag: lincity-ng-2.10.2 <br/>
git: a6101294

###### User Interface:
- Fixed a bug in installation/packaging which prevented creation of certain GUI
  components especially dialogs.


## LinCity-NG 2.10.1

date: 2024-03-16 <br/>
tag: lincity-ng-2.10.1 <br/>
git: 3b8c58bf

###### Internal
- Fixed an unsafe memory write which could cause the game to crash.
- Added 'git' to the list of build dependencies.

###### Documentation / Translation
- Added a note in the readme to warn users of different build instructions for
  different versions of LinCity-NG.


## LinCity-NG 2.10.0

date: 2024-03-08 <br/>
tag: lincity-ng-2.10.0 <br/>
git: f03cbe18

###### Gameplay
- Fixed the amount of swing jobs created by health cover. You may now find that
  you need fewer residences and/or more jobs demand in your economy.
- Changed the effect of swing jobs. Now jobs production at residences scales
  down as inventory increases. This should make employment feel more stable and
  make it easier to strike a balance in employment. The amount of the production
  decrease is dependent on "swing" jobs, so sports cover and health cover
  increase the effect. The previous method using a "swingometer" has been
  abandoned.
- Fixed evacuation logic to avoid a construction getting negative inventory.
- Added water to the market trade dialog.

###### User Interface
- Fixed jumpy and laggy scrolling near the map edge.
- Fixed a bug where the shift key (used to increase scroll speed) was
  "forgotten" when hitting the map edge.
- Fixed a bug where moving the cursor outside the game window may cause the map
  to scroll.
- Fixed the speed and direction of scrolling with the scroll wheel in help
  pages.
- Changed the panning (i.e. scrolling by dragging) button from the right mouse
  button to the middle mouse button.
- Changed the panning (i.e. scrolling by dragging) direction, so the map moves
  in the expected direction.
- Added map scrolling with WASD.
- Changed the zoom anchor to be relative to the cursor position instead of the
  center of the screen. This makes map navigation much more natural. And it
  plays nice with hitting the map edge (which is more complex then you may first
  expect).
- Normalized scrolling speed for direction and zoom level. (So scrolling
  diagonally while zoomed in no longer feels like mach three.)
- Changed right-click action to show information about construction or tile.
- Added G-key action to toggles the right-click function between construction
  info or tile info.
- Decoupled the speeds of simulation, animation, screen redrawing, and GUI so
  animation and frame rate is no longer dependent on simulation speed. This
  makes the UI much smoother at slow simulation speed.
- Added a page to the MPS display for most constructions that shows monthly
  production/consumption of commodities (both as absolute and percentage).
- Tweaked the total inventory MPS page so that the red/green bars are faster to
  reflect changes in production.
- Constrained he minimap position to avoid showing space outside the map.
- Changed the cursor appearance for panning (i.e. scrolling by dragging).
- Created a (much prettier IMO) black background for the map.
- Fixed icon and desktop file install location for Linux systems.
- Added gd translation for desktop file.

###### Internal
- Fixed a buffer overflow when the home directory path name is very long.
- Fixed a bug in load/save where parks could erroneously get very high
  inventory.
- Fixed a bug where building the project could overwrite changes to GUI dialogs.
- Improved simulation performance. (Fast speed is now blazing fast.)
- Changed the location of save games from ~/.lincity-ng to
  ~/.local/share/lincity-ng. For now, existing save games from the old location
  are still loaded, but subsequent saves go to the new location.
- Replaced the outdated autoconf and Jam build tools with CMake. (Be sure to
  follow the updated instructions for building the project.)
- Changed install directory structure to match GNU style.
- Added binary generation to CPack.
- Added Dockerfile to help create compatible binaries for Ubuntu. (Now we can
  release binary packages.)
- Added a hook in the build system to append the current git hash to the version
  number when not on a release version. This should help reduce ambiguity in bug
  reports.
- Upgraded the MPS display API so constructions can have any number of pages.
- Removed uses of deprecated PhysFS features.
- Cleaned up the logic for binary relocation (and probably fixed a few things).
- Fixed errors and warnings when compiling with C++20.
- Fixed warnings when compiling with -Wall option.
- Added a construction initialization stage to loading and building
  constructions.
- Tidied up the source tree and deleted dead code.
- Added translation file generation build step.

###### Documentation
- Converted the readme to GitHub Markdown.
- Expanded the dependency list in the readme to include build dependencies.
- Included commands for installing dependencies on Ubuntu in the readme.
- Removed dead links in the readme.
- Updated the list of editors in translation instructions.
- Updated formatting in COPYING file.

###### Known Issues
- Untested on Windows and Mac.
- Infrastructure for building for Mac may be missing since updating the build
  tool.
- Ports are OP.
- Does not autodetect screen size on startup in fullscreen.


## LinCity-NG 2.9.0

date: 2023-12-18 <br/>
tag: lincity-ng-2.9.0 <br/>
git: ff3e337b

- New sprites
- Overlay sprites
- Roads spawn cars that drive around if there is jobs traffic.
- Right-click switches to query tool.
- Number keys + '`' switches overlay modes.
- Distinction between river, lake, and pond
- Lots of fixes
- Translation updates
- OSX support
- Migraded to SDL2 as backend for interaction with user.


## LinCity-NG 2.9.0-beta

date: 2013-12-22 <br/>
tag: lincity-ng-2.9.0-beta <br/>
git: 803a97df <br/>
svn: r1797

- custom map size, see README-Unlimited
- new water management, see README-WaterWell
- more supported languages
- less bugs


## LinCity-NG 2.0.0

date: 2009-01-25 <br/>
tag: lincity-ng-2.0.0 <br/>
git: 52160033 <br/>
svn: r1518

- Updated translations (cs, de, nl, pt_BR, ru, sv)
- Use language specific font if `fonts/<style>-<Language>.ttf` exists.
  Note: To use the Japanese translation you have to copy or symlink
  a font with Japanese characters to fonts/sans-ja_JP.ttf.
- improved images
- less bugs


## LinCity-NG 1.97.0-beta

date: 2008-12-24 <br/>
tag: lincity-ng-1.97.0-beta <br/>
git: 0e21f221 <br/>
svn: r1474

- less bugs
- up to date English documentation


## LinCity-NG 1.92.0-beta

date: 2008-09-13 <br/>
tag: lincity-ng-1.92.0-beta <br/>
git: 66f7f4b1 <br/>
svn: r1456

- water management
- Bridges
- improved images
- changes in savegame format, saves to ~/.lincity-ng/
- documentation is NOT up to date


## LinCity-NG 1.1.2

date: 2007-11-04 <br/>
tag: lincity-ng-1.1.2 <br/>
git: 9cf5a31c <br/>
svn: r1270

- Show coal below buildings on mini map.
- Bugfix for solar power plant.
- Updated translations (ca, nl, ru)
- Show range for windmill and coal mine while building.


## LinCity-NG 1.1.1

date: 2007-08-19 <br/>
tag: lincity-ng-1.1.1 <br/>
git: 886ebe78 <br/>
svn: r1263

- History in online help
- Heavy industry no longer storing too much coal.
- Sustainability progress is reloaded from saved games.
- Draw range for selected building around cursor
- Online help includes a tutorial
- Russian online help
- Brazilian Portuguese online help
- Change colour of economy tab when people are unemployed or starving
- Fixed problem with multi byte characters in WinXP
- Reuse open statistics dialog


## LinCity-NG 1.1.0

date: 2007-02-19 <br/>
tag: lincity-ng-1.1.0 <br/>
git: 43342ee0 <br/>
svn: r1139

- Display correct values for unemployment rate.
- Port connects on all land parts.
- Change background music in menu.
- Show range of markets in traffic view
- French online help
- German online help


## LinCity-NG 1.0.3

date: 2006-04-14 <br/>
tag: lincity-ng-1.0.3 <br/>
git: 5b50f632 <br/>
svn: r1089

- Show correct demolition costs for powerlines, substations and farms.
- Quantizing tile-images saves about 12MB.
- Fixed Mills. Now they work.
- Tool-Menu now opens on left mousebutton.
- Separate menus for loading and saving games.


## LinCity-NG 1.0.2

date: 2005-11-09 <br/>
tag: lincity-ng-1.0.2 <br/>
git: 035ca774 <br/>
svn: r1063

- Catalan translation
- Spanish translation
- French translation
- Swedish translation
- Fixed: unwanted double-clicks in file menu


## LinCity-NG 1.0.1

date: 2005-07-13 <br/>
tag: lincity-ng-1.0.1 <br/>
git: 9ce566c5 <br/>
svn: r1038

- Adjust volume in options
- Load game on doubleclick
- Fixed: Endgame evacuation causes termination of game.
- Fixed: Hightech houses require tech 30, but the tooltip says 20.
- Fixed: Suppressed duplicated tech-messages.
- Fixed: Duplicated command line arguments.
- Fixed: Crash when empty scenario is selected.


## LinCity-NG 1.0.0

date: 2005-06-20 <br/>
tag: lincity-ng-1.0.0 <br/>
git: f12f7943 <br/>
svn: r1026

- The first release of LincityNG features everything from classic lincity
  but with brand new graphics and modern user interface.
