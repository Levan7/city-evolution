# Audit of Preview 11 (done on the supplied source, then fixed)

## Method
Built Preview 11 from the supplied source on Linux and cross-built it for Windows. Ran its four simulation test programs and the interface checks. Launched the game on a virtual screen and looked at the main menu, the in-game screen, every City Hall tab, the Transport report, tool drawers, Settings and New world. Read the code ChatGPT changed against the previous version. The Windows build was run under Wine.

## What was already good
- Simulation tests all pass: city-evolution (`doc/audit/city-evolution-checks.log`), mayor, transit (bus, rail, two metros), community (church, horse/car eras).
- All earlier features survived: workforce, districts, mayor mandate, five-year plans, city naming, greyed-out unavailable buildings, tool selection reset (Esc).
- Traffic era rule is deterministic: 0% cars below 10% technology, 1% at 10%, 100% from 30%.
- The new menu background, speed buttons, Transport report and the building catalogue draw correctly.

## Problems found, and what was done
| # | Problem (evidence) | Result |
| --- | --- | --- |
| 1 | Normal build fails: `buttonpanel.xml`, `speedpanel.xml` had no `xml-stylesheet` line (xsltproc "document is not a stylesheet") | Fixed; check added; negative test shows it fails when the line is removed |
| 2 | City Hall pages in scroll views, gaps of about 64 px per line, text hidden behind buttons (screenshot) | Compact layout restored |
| 3 | Minimap tabs unreadable (pale tab image 222,222,222 with text colour #edf0df); opaque frame covering the map | Dark tabs, white text, smaller captions, transparent frame |
| 4 | Drawers 320x360 for 3 buttons | Sized per button count |
| 5 | Church: month-end counters inside `if(active)`, stale when closed on last day | Fixed + regression test |
| 6 | Mayor: extra block in `mayorStep` paused any non-park building when Services were blocked (also roads, campus) | Removed (plan already handles switches) |
| 7 | 60 images from Preview 10 referenced by nothing (12 MB) | Removed |
| 8 | `tests/preview10_assets_check.py` fails against the current 52 tools | Removed (the Preview 11 check replaces it) |
| 9 | Patches, hotfix notes, stale logs and checksums in the source root | Filed in `doc/history/` or removed |

## Not changed, and open
- Real Windows PC: never run. Preview 9 reportedly crashed at start; the Preview 11 build starts and runs under Wine, but that is not proof for Windows.
- Audio never tested (Wine had no sound device).
- 411 other images are not named in any file (about 18 MB, mostly upstream tiles and old menu images). They were kept because the game could build some names at run time; removing them needs a play-through.
- Terrain, bridges and some effects still use upstream art. Buildings can hide ground behind them in the isometric view.
- The restyled Settings, New world and Load screens keep two button looks (flat and dark blue); harmless but not fully uniform.
- The church regression test was not run against the old code to confirm it fails there.
