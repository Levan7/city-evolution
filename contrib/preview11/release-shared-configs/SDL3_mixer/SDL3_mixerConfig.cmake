add_library(SDL3_mixer::SDL3_mixer SHARED IMPORTED)
set_target_properties(SDL3_mixer::SDL3_mixer PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "/workspace/scratch/085b248c71d7/work11/sdk/ucrt64/include" IMPORTED_IMPLIB "/workspace/scratch/085b248c71d7/work11/sdk/ucrt64/lib/libSDL3_mixer.dll.a" IMPORTED_LOCATION "/workspace/scratch/085b248c71d7/work11/sdk/ucrt64/bin/SDL3_mixer.dll")
