# City Evolution — Preview 11
4 November 2026. A fork of LinCity-NG.

This release contains a newly compiled native Windows executable and integrates the church/traffic/UI work with the later request for lower, more varied buildings.

## Artwork and road visibility

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

## Menus and controls

- Restored the custom generated riverside city background, with menu buttons in its shaded left area.
- Restored one horizontal row of pause, green play, fast and fastest buttons, using the existing created dark teal theme.
- Mayor is at the top right, with Transport report directly below.
- Removed construction buttons and their callbacks from the transport report. Construction stays in the left categories.
- Kept 15 left categories and all previous 51 tool definitions; church is tool 52 in Civic.
- Categories stay openable when their default tool is locked; unavailable individual buildings remain disabled with requirement tooltips.
- Selecting tunnel/elevated construction from the sidebar automatically selects the corresponding transport layer view.
- Models reset visible when a world opens. A labelled button beneath Transport report restores model visibility; H remains the original toggle.
- Decorative details hide with their parent buildings. The long warehouse explanation is shortened to fit the fixed statistics panel.

## Horses, carriages and cars

The shared policy is src/lincity/traffic_era.hpp. Elapsed time alone does not unlock cars.

| Technology | Motor traffic share |
| --- | --- |
| Below 10% | 0%; horses and carriages |
| At 10% | 1% |
| 10% to 30% | Gradual deterministic transition |
| 30% or above | 100% |

The policy applies to native vehicles, newer street actors, parked warehouse detail and representative bus-route vehicles. Early bus routes display horse carriages. Parked trucks become carts before motorisation. There are four horse views and four carriage views. The native vehicle group retains 16 slots, reusing carriage views for turns/slopes. Vehicles move along built streets/tracks; empty wilderness has no ambient traffic.

This changes visual traffic eras, not commodity logistics, timetables, capacity or transport build technology requirements. Rail retains its own prerequisites. Horses/carts used as parked building details remain decorative. The moving sprites are not a detailed animal/driver economy or a full leg/wheel animation system.

## Georgian Orthodox Christian church

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

## Native code and audit coverage

New/changed code covers group 62 and group count 63, church registration/module, input consumption, education, coverage, residence morale, approval, spending, yearly totals, save/load, traffic progression, model visibility, report callbacks, layer selection, browsable categories, stable art variants and scaled smoke offsets. The game and four Windows simulation diagnostics are rebuilt.

The audit materials include full current engine and GUI source, tests, build notes, binary hashes, import/export results, active resource mappings, art previews and two textual patch stages:
1. The inherited Changes-from-Claude.patch records the earlier Claude Preview 8 → Preview 10 work.
2. Preview10-to-Preview11.patch records this release's changes.

The full Source ZIP supplies all binary artwork and third-party source. The focused hash report records binary changes as well. The previous baseline comparison is preserved as history; the combined current index records original-before and current-after hashes for changes since the supplied Claude archive. Patches alone are not a playable update.

## Verification

- Windows x64 UCRT game and four diagnostics compiled successfully.
- Native real-engine community tests passed for traffic thresholds, monotonicity, seed mixes, church construction/costs, staffing, upkeep, learning, coverage, wellbeing, save/load, closure without goods and demolition.
- Stable placement-based commune variant assignment passed.
- All four transit modes passed native regression tests: ridership, coverage, fares, upkeep, save/load, route cuts, staffing, interchange and independent layers.
- Source/package GUI style checks passed in both NLS branches.
- Asset checks passed: 52 tools, 15 categories, 42 table layouts, all original native animation slot counts, three commune variants, twelve housing appearances and 95 rescaled building frames.
- Windows PE dependencies and imported symbols passed, including the dynamic PNG/Vorbis/font libraries used by the shipped files.
- Release ZIP CRCs and per-file SHA-256 entries are checked.

Headless tests use inert display/audio adapters and do not test graphical interaction. Windows startup, final rendering alignment and audio playback have not been play-tested here. Isometric screenshots/contact sheets and static layout checks are not a Windows play-through.

## Remaining limits

Terrain, bridges, network tiles and some effects still use upstream art. Many working-building loops reuse three new poses rather than repainting every original pose. The complete source preserves original/input images for reproduction; the playable package removes unreferenced tile images and has one ordered binding per active resource.

## Install

Extract the complete Windows ZIP into a new writable folder and run Play.cmd. Keep personal saves backed up before copying them into PortableData. Do not mix old and new DLL folders. Verify-Community.cmd runs the church/traffic diagnostic. For startup failures, Diagnose-Startup.cmd records diagnostics; PortableData/launch.log contains startup output.
