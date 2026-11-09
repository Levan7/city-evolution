# Audit of Preview 17

## Method
Built Preview 17 from the supplied archive on Linux and cross-built it for Windows. Ran its eleven simulation programs and five interface checks. Rebuilt the simulation with AddressSanitizer and UndefinedBehaviorSanitizer and ran all eleven again. Compiled the six newest simulation files with `-Wall -Wextra`. Launched the game on a virtual screen and looked at the HUD, City Hall (all tabs) and the Budget window; reproduced the tax problem by hand. Read the tax, budget-window and packaging code. The Windows package was run under Wine.

## What was good
- Eleven simulation programs pass; the main suite is 168,536 checks. The same programs show no findings under the sanitizers, and the six newest source files produce no compiler warnings.
- Menus audited in Preview 12 are intact (protected-menu check passes), including the greyed-out unavailable buildings and the minimap.
- The packaging idea is right: a real `City Evolution.exe` (with the skyline icon and a version resource) starts the game with no console window and writes `PortableData\launch.log`.
- City Hall and the Budget window now pause the city while open ("CITY PAUSED" in the title).

## Problems found
| # | Problem (evidence) | Result |
| --- | --- | --- |
| 1 | Auto-adjust taxes "does nothing": only toggled a flag; changed at most one point at the monthly review; skipped entirely if a review already happened that month; forced rates above a hidden 15% ceiling down one point at a time; no change at all when income and cash looked fine (code in `civic_services.cpp`) | Immediate apply of the advisor's recommendation plus a reason; monthly follow-up one point; 9 new test assertions |
| 2 | Budget window: 20 equal buttons, values far from their buttons, funding buttons only increased (wrap-around to go down), advisor text in a separate dialog (screenshot) | Rebuilt with three tabs, per-row minus/plus, inline advisor |
| 3 | Window title hard-coded "Preview 15" while the menu said 17 | One label file and a consistency test |
| 4 | Windows packaging copied `README-Preview15.txt` as the readme | Permanent readme with the SmartScreen explanation |
| 5 | Plan tab: focus button caption overflowed, no explanation of focus | Short caption, explanatory line |
| 6 | Release notes and a 1.1 MB patch cluttered the source root | Moved or removed |

## Not changed, and open
- **Code signing.** The Windows exe is unsigned. A signature needs a paid certificate from a certificate authority (or a trusted-signing service); without it Windows shows "unknown publisher" / SmartScreen prompts for new downloads. It is not a malware detection. Reputation builds over time as more people run the file.
- **Real Windows PC and real audio hardware** are untested (Wine and a dummy audio device only).
- **Night lighting** was not re-evaluated here: the game offers no way to force night in the finished interface, and a natural night takes several real minutes.
- **Large, busy cities:** long-session balance and performance remain untested.
- The two Python checks `preview11_assets_check.py` and `menu_preservation_check.py` guard the older art and menu layout; they do not cover the new Budget window layout beyond style and build registration.
