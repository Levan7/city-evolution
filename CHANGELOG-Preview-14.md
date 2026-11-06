# City Evolution — Preview 14

Preview 14 builds on the audited Preview 12 menus and Preview 13 services. It fixes Linux audio failure handling and UI scaling, enables native Wayland, adds women pedestrians and optional flu, and makes building inventory changes visible. Existing save files are supported; keep backups before saving with new state. This remains an experimental release.

## Linux audio and crash handling

The supplied crash log reported an ALSA PipeWire plugin at an Ubuntu-specific path on Fedora. When mixer creation failed, the previous Sound constructor left track pointers uninitialized; volume controls could then dereference them. Tracks and mixer handles now start null, initialization failures leave a usable silent game, volume values are clamped and setters only touch valid tracks. Cleanup destroys the actual mixer in the correct order. The game releases Sound before the final SDL_Quit; a real packaged-game exit test exposed a separate double-free when that order was reversed.

Music completion now queues a request for the main thread; the audio callback does not load/destroy music files. Sound assets load synchronously, avoiding a loader/playback data race. Settings report when no audio device is available.

The Linux SDL build enables PulseAudio-compatible desktop audio with ALSA fallback. Host libpulse/libasound are used, rather than packaging Ubuntu's plugin lookup paths. Fedora PipeWire normally supplies the PulseAudio-compatible service. These changes do not install or reconfigure the user's sound server.

## Native Wayland and display scaling

SDL is built with Wayland, libdecor and X11 support. The launcher does not force X11. SDL selects an available desktop; explicit native Wayland can be requested with `SDL_VIDEO_DRIVER=wayland`. Wayland client, libdecor, xkbcommon, EGL and audio integration use the host desktop libraries. Hardware graphics drivers are not bundled.

The existing UI uses one logical size and matching mouse-coordinate conversion. At 3072×1590 the automatic UI scale is 1.75; at 1920×1080 and 1280×800 it is 1.0. Window dimensions saved in configuration use window units rather than framebuffer pixels. The legacy explicit OpenGL rendering path retains its old scale; the AppImage uses the SDL renderer by default. Window/renderer creation failures now produce an error instead of proceeding with null handles.

The audited menu layouts, colors, City Hall pages, category drawers and minimap artwork are retained. There is no Android build in the supplied source to compare directly; the readable supplied reference and existing menus guide this fix.

## Building visibility and dynamic artwork

The previous top-right button only restored visible models; clicking while already visible had no effect. It now toggles hide/show and changes its label. H uses the same state. Ground, roads, tracks, power lines and fire remain visible; building/tree models and their attached details hide together.

Original construction resource frame counts and working-state animations are retained. Farms, mines, workshops and windmills continue to choose frames from simulation state. Some market states used the same base image: new low/medium/full stock props now make inventory changes visible. Markets use their existing market ratio; warehouses display goods; ore/coal mines display their material; selected workshops/industries display output stock. Empty inventory has no stock prop. This is not an inventory overlay for every possible material or a new animation rig.

The new 12 small transparent stock sprites keep a consistent scale per material. They follow parent visibility, depth ordering and night tint. Health/fire/police window lights also respect active/ready service state.

## Pedestrians

Added 16 transparent woman walking sprites: four directions, four frames each. A stable mix of women and existing men uses the existing street-actor paths and pedestrian scale. This changes presentation, not employment/population gender rules. The raw generated atlases and export script are included for review.

## Optional flu

Flu is separate from existing pollution illness. Household cohorts model exposure, incubation, infectiousness, recovery and temporary immunity. Updates follow simulated time, not wall time. Local contacts and shared surface transport networks can spread infection; supplied healthcare and education lower its impact. Ambulance treatment supports recovery. Illness reduces available household labor and desirability without adding another mortality penalty.

The prototype uses 10 simulated days of incubation, 30 days infectious and 180 days immunity. Import rolls start after day 600 in cities with at least 100 residents; winter increases spread. Infections and notices are bounded to keep this a supporting mechanic. These are game tuning values, not a medical model or COVID-specific simulation. No quarantine, vaccine or pandemic-management system is claimed.

City Hall > Budget / public services (F7) has an ON/OFF switch and flu counts. Turning it off clears flu. State and the random generator are saved; older saves without flu state use defaults. The existing polluted-city zombie Easter egg, police, emergency dispatch, budgets and mayor permissions remain from Preview 13.

## Packaging and validation

Linux x86_64 AppImage: Ubuntu 24.04/glibc 2.39 baseline. Make executable and run; `--appimage-extract-and-run` avoids a FUSE requirement. Saves/configuration use the city-evolution XDG directories.

Windows x64: extract the whole ZIP and run Play.cmd. PortableData/launch.log records startup. Verify-Simulation.cmd runs the six simulation diagnostics. Cross-compilation/import checks do not replace a real Windows runtime test.

Six simulation programs, real audio integration tests, original-vs-fixed failed-ALSA regression, asset checks and protected-menu checks are provided. Recovery logs from the earlier session are marked separately from fresh rebuild evidence. Full results and platform limitations are in doc/AUDIT-Preview-14.md and the accompanying audit archive.

Physical Fedora speakers, a real Windows PC and Android runtime were not available for validation. Flu spread, service behavior and large-city performance need extended gameplay balancing. No claim of flawless hardware support is made.
