# Expanded Worlds 0.2.1 — startup compatibility candidate

The reported exit code -1073740940 is Windows status 0xC0000374 (heap
corruption). It does not, by itself, identify the instruction that corrupted
memory. The failed build's PortableData/launch.log or a crash dump is required
to establish the specific fault.

This rebuild removes the game's direct dependency on prebuilt C++ libxml++,
fmt, libstdc++ and libgcc DLLs. The XML wrapper is compiled with the game,
fmt is header-only, and the compiler support runtime is statically linked.
This addresses the known mixed-C++-runtime risk in 0.2, while retaining the
original engine and all previously delivered gameplay changes.

Startup phase markers are now flushed to launch.log around XML/configuration,
SDL/fonts, video, modules, sound and menu initialization. Runtime-Check.cmd
tests the version/configuration path without opening a game window and writes
PortableData/runtime-check.log.

This is NOT a release of the entire planned revamp. Implemented features:
map sizes, empty/barren starts, empty-start budget and basic on-demand advisors.
The original LinCity-NG economy remains. Elections, new economic districts,
dedicated warehouses, new technology-industry buildings and AI municipalities
are not implemented.

Extract this version into its own new folder. First launch Play.cmd without
copying old configuration; once startup works, back up and transfer saved
cities as needed. If startup still fails, retain launch.log and runtime-check.log.
Do not disable antivirus or add security exclusions to make this build start.
A Microsoft Defender cloud-scan notification is not a crash diagnosis, nor is
it by itself a clean or malicious verdict for this file.

Compilation, map-policy/UI structure checks and the import-symbol audit passed.
Windows startup has not been executed here; the reported crash is not yet
confirmed fixed.

Supplemental check: the rebuilt XML wrapper and header-only formatter completed 232,560 XML/string operations under native Linux AddressSanitizer, with leak scanning disabled. This was a library/string-lifetime check, not Windows startup verification.
