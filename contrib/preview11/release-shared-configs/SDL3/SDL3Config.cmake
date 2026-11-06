add_library(SDL3::SDL3 SHARED IMPORTED)
set_target_properties(SDL3::SDL3 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "/workspace/scratch/085b248c71d7/work11/sdk/ucrt64/include" IMPORTED_IMPLIB "/workspace/scratch/085b248c71d7/work11/sdk/ucrt64/lib/libSDL3.dll.a" IMPORTED_LOCATION "/workspace/scratch/085b248c71d7/work11/sdk/ucrt64/bin/SDL3.dll")
