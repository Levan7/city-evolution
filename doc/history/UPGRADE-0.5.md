# City Evolution 0.5 — readable controls and first delegated mayor

Extract into a new folder and run Play.cmd. This is a new UCRT executable built
from the modified engine. Existing 0.3/0.4 cities can be imported; keep backups.

## Playability first

All 53 construction/category controls now use recognizable pictures with names,
rather than repeated letter badges. These include the legacy building/tool art
and the four new building sprites, composed into larger, labeled controls.
Category controls are 144x54; tool controls are 72x64. Category labels stay fixed
when changing tools. The included TOOL-ICONS.png shows the actual shipped images.
Speed buttons have pause/play/fast pictograms. The top category is New places.

City hall (F6 or CITY HALL / SYSTEMS) has Council, Buildings, Weather and Mayor AI
tabs. Full-width building buttons and separate half-width controls replace the
oversized buttons that previously crossed column boundaries. Button captions
are measured before layout. Table text/buttons are clipped to their own cells
as a fallback; construction drawers retain their map-overlay behavior. Long
advisor dialogs scroll. The status bar is taller and has more space for text.

## A working first mayor planner

Open Mayor AI in City hall. The default is **Advice only**: no building or spending
is authorized. Cycle authority to **District manager** or **City manager**.
District manager captures a 25x25 area around the current camera center when
selected. Moving the camera afterward does not move that assignment. Cycle back
to Advice only to revoke authority; select District manager again to reassign.

A monthly offline planner assesses housing, service counts and direct transport
access. It can place a water well, school, health center, fire station or park
when needed, unlocked, supplied with an accessible vacant site, and affordable.
Optional road permission fills a single-tile road/track access gap at an isolated
building when an existing transport tile reaches that gap. It is not a full
route planner. It does not construct housing/industry or operate a passenger
transport network.

Services and parks have separate permission toggles; road gaps default OFF.
Annual **construction** caps cycle through $5,000/$10,000/$25,000/$50,000. The
planner preserves a $25,000 cash reserve, authorizes at most one project per
monthly assessment, pays the engine's real construction charge and records
completed projects in council news. Normal ongoing service operating expenses
remain part of the ordinary city economy; they are not covered by this separate
construction cap. Buildings still need actual labor, goods, water and power.

The mayor only uses bare/desert vacant sites. It cannot demolish, close factories,
change taxes, revoke your tools, or remove your control after an election. Its
permissions, scope, budget and spending persist in saves. This is deterministic
game AI, not a connected language model or a self-learning municipal simulation.
It can defer a project when technology, access, money or safe land is missing.

## Scope: implemented versus remaining

Implemented in the game: map sizes 50-400; natural empty/barren starts; slow
real-play sky/weather; solar consequences; decorative cars and pedestrians;
warehouse freight buffer; technology campus; cafe and food hall; monthly approval
and advisory elections; shortage/access advisors; new front end; and the first
permission-controlled mayor planner described above.

Still unfinished: named competing mayors and political parties, richer electoral
priorities and campaigns, multiple simultaneous municipalities, self-learning
planning, industrial modernization/filters/relocation policies, full dispatch and
congestion simulation, metro/bus/tram and airports, detailed district economics,
a complete replacement of world/vehicle art and translations for the new text.
These are not hidden or disabled options. They remain development work.

The long daylight/night periods remain independent of accelerated simulation:
daylight 8-16 active minutes, night 3-6, with pause stopping the clock.

## Validation and limits

The real native simulation core is tested under AddressSanitizer and
UndefinedBehaviorSanitizer (display/audio/config adapters, leak/vptr checks off).
Mayor checks exercise actual service construction/payment, cash reserve refusal,
annual cap, district restrictions, revocation, greenery permission, preservation
of existing housing, save/load and continued simulation. General engine checks
cover the earlier expansion and original scenarios. Windows executables are
compiled with LLVM-MinGW/UCRT and their DLL imports are checked.

Windows startup, rendered appearance, clicks, long-term balance and performance
of this new executable are not verified in this environment. Run the included
Verify-Mayor.cmd for the standalone Windows mayor diagnostic. Existing
Runtime-Check.cmd and Diagnose-Startup.cmd are also supplied.
