The exported data is complete; normal engine builds do not rerun the asset scripts. The Python files record one-time production transforms against a Preview 10 workspace. Paths in those transforms identify that workspace.

Windows release: LLVM-MinGW 20261104 UCRT x86_64, Clang 23.1.5; bundled C++ fmt/libxml++ built with the game. CMake 4.4.4 and Ninja 1.13.2. The minimal shared SDL import configs are release environment wiring, not fake implementation libraries: their IMPORTED_IMPLIB and IMPORTED_LOCATION point to the actual MSYS2 import libraries/DLLs. For a normal full SDK, use its standard package configs.

Native headless tests: GNU 13.3.0, host libxml2 2.9.14, matching Ubuntu development/ICU headers. SDL interfaces were declarations only; display/audio use the existing inert test adapters. Simulation code is real. No Windows graphics claim is made.
