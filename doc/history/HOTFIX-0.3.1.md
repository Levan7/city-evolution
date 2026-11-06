# City Evolution 0.3.1 UCRT startup candidate

This release rebuilds the entire Windows executable, vendored libxml++ wrapper,
and standalone simulation diagnostic with LLVM-MinGW 20250613 / Clang 20.1.7,
targeting the Windows Universal C Runtime (UCRT). The C++ standard library is statically
linked from that toolchain; the matching libunwind.dll is bundled. Original C-interface runtime DLLs and game assets
remain the basis of the portable package. All 0.3.0 gameplay additions remain.

The 0.3.0 executable imported MSVCRT while the original working 2.15.0 release
and its bundled libraries use UCRT. That was a concrete build inconsistency.
The user's diagnostic confirmed intact files and a 0xC0000005 crash during
--version, before any startup messages or graphics initialization. The report
named ntdll.dll; it did not provide a stack trace proving the exact cause.
This release removes the runtime inconsistency. It is still a startup candidate,
not a claim that Windows execution has passed or that the crash is proven fixed.

Extract into a NEW writable folder and launch Play.cmd. Do not mix executables
or runtime DLLs from different packages. Your old saves need not be moved for
this test. The supplied showcase remains in Load Game slot 1.
If startup fails, run the included Diagnose-Startup.cmd beside Play.cmd and
attach StartupDiagnostic.txt. This collector logs the exit code, file integrity
and relevant application crash events even if the game produces empty logs.
It does not alter registry entries or security settings.

The save format remains 2141. City Evolution 0.3.0 saves remain compatible.
Keep untouched backups before importing earlier cities; saves written by this
upgrade cannot be loaded by unmodified 2.15.0 or Expanded Worlds 0.2.x.

Cross-build configuration is included at mk/cmake/toolchains/windows-ucrt.cmake.
Use a new build directory; pass LINCITY_LLVM_MINGW_ROOT and LINCITY_WIN_SDK_ROOT
as absolute paths, LINCITYNG_BUNDLED_CXX=ON and LINCITYNG_RELOCATABLE=ON.
The SDK supplies the C interfaces SDL3, SDL3_image, SDL3_mixer, SDL3_ttf,
libxml2, zlib and gettext. Rebuild every C++ object with the selected toolchain.
The LLVM toolchain archive is a build dependency, not a game runtime dependency.
Its license is supplied under contrib/expanded-worlds/LLVM-LICENSE.txt and in
this portable package's Licenses folder.

Validation: compilation/linking and executable import inspection are performed
on this candidate. The engine logic is unchanged from 0.3.0, which passed the
native Linux simulation/save-load checks documented in CITY-EVOLUTION-0.3.md.
Those checks do not verify Windows startup, graphics, input or performance.
