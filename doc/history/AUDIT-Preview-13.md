# Preview 13 audit handoff

Review against the supplied Preview 12 source, not against the earlier unreviewed previews. Preview 12's audit remains in `doc/AUDIT-Preview-12.md`. `CHANGELOG-Preview-13.md` describes implemented behavior and limitations.

## Evidence

- `doc/audit/preview13/`: fresh logs from the five simulation test programs, GUI style check, asset/frame check, menu-preservation check, and packaged-library audio decode.
- `doc/audit/Preview12-protected-menu-hashes.json`: baseline menu/image hashes. Version-label normalization is the only exception for mainmenu.xml.
- `doc/audit/Preview12-cityhall-pages.json`: canonical Preview 12 page trees, compared without the footer extension.
- `doc/audit/Windows-imports.json`: recursive packaged binary imports and unresolved list. A clean import list is not a Windows runtime test.
- `doc/audit/Linux-bundled-libraries.json`: library origins for the AppImage. System glibc and graphics drivers are intentionally not bundled.
- `doc/audit/ART-CHANGES-Preview13.json`: changed binary-art inventory, hashes, bytes and resource bindings.
- `preview12-to-preview13.patch`: every changed/new text file; binary images are in the full source. The separate audit ZIP also includes review screenshots and changed/new text files for convenient inspection.

## Important review points

1. Civic state loading is deferred until map dimensions are loaded. Validate malformed and oversized saves; route steps must be adjacent and in bounds.
2. Construction::torch and Residence::torch now intercept ignition to allow rescue. The forced-burn guard prevents re-interception when the 600-day interval expires. Check fire spread, multiple footprints and production suspension during longer playtests.
3. Service routing is weighted pathfinding across the map, with roads/off-road and blocked water/buildings. There is no coverage-radius ceiling. Test performance with many incidents/stations on 400×400 maps; no long soak/performance guarantee is claimed.
4. Money uses existing income/goods/social rates. Budgets gate service operation and scale expenses. Police has its own saved ledger; the fixed-size Other Costs page combines fire/police on one row.
5. Fiscal reserve and construction reserve have different roles, described in the changelog. Mayor mandates and district/scope controls are inherited, with added need rules. Autonomous optimal traffic engineering is not implemented.
6. Pollution illness supplements the existing pollution-death mechanism without adding duplicate deaths. Full contagious disease transmission remains out of scope.
7. The zombie event is bounded, default ON, rare, non-spreading and easily disabled. Its markers are a prototype visual. It should remain a minor addition.
8. Renderer lights are per-parent overlays, not an overlay on the whole UI. Electricity and residence occupancy gate lights. Prepared JSON describes original pack provenance; `NightCrops.hpp` is the concrete compiled mapping including the police addition.
9. Tower outputs and service costs are prototype economic coefficients. No claim of final balance or proof on real Windows/Fedora hardware is made.

## Rebuild

The normal project is still CMake/C++. `contrib/build-ubuntu-2404.sh` and `contrib/windows-cross/build-windows.sh` provide Linux/Windows dependency recipes. `contrib/preview13/recovery/` contains the exact helper scripts used during recovery, with environment-specific staging paths; adapt those paths for another machine. Dependency input hashes are included. SDL_mixer came from the main-branch tarball at build time; its exact downloaded bytes are hashed, so a later download from that moving URL may differ.

The initial recovered image dependency accidentally had PNG decoding disabled; graphical startup caught it. Both builds were corrected to use SDL_image's STB backend with PNG/JPEG enabled before final packaging. The developer-logo dimensions and new tower labels were also checked visually.
