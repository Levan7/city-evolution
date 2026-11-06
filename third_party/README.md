# C++ dependency consistency

Expanded Worlds 0.2.1 builds libxml++ into the game with the same compiler,
headers and runtime, and uses fmt in header-only mode. The prior build linked
newly compiled C++ application code to unrelated prebuilt C++ DLLs. Import-name
checks alone cannot establish compatible C++ object layouts or allocation/free
behavior across that boundary.

This change removes that identified compatibility risk. It is a candidate fix
for the reported 0xC0000374 startup failure, not a diagnosis confirmed by a crash
dump. Windows runtime verification and the failed build's launch.log remain
necessary to confirm the user's particular failure.

* libxml++ 5.6.0: https://github.com/libxmlplusplus/libxmlplusplus
  commit 80ce21a37cfd3cb329590e071cf17df886fbbf89, LGPL-2.1-or-later.
  Source and COPYING are included. libxml++config.h is supplied for this build
  with static mode enabled; the source itself is unchanged.
* fmt 12.1.0: headers from the MSYS2 development package, MIT license, with
  the complete header-only implementation and original license included.

CMake's LINCITYNG_BUNDLED_CXX option defaults to ON. Disabling it restores the
upstream system-dependency route and requires an ABI-consistent toolchain.

The remaining SDL/libxml2/zlib dependencies expose C interfaces; their original
Windows DLLs are retained. The game binary statically links its C++ compiler
runtime. Original libraries can still use their own internal runtime.
