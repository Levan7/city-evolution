# City Evolution — Preview 11
A fork of LinCity-NG.

Complete engine source/data for the rebuilt Preview 11 release. Read CHANGELOG-Preview-11.md for behavior, validation and limits.

The changes integrate compact/varied building art, a Georgian Orthodox church, horse-to-motor traffic progression, restored menu/speed controls and a right-side report-only transport window.

Build with CMake as described in PREVIEW-9.md; older release notes describe historical revisions. Use ENABLE_NLS=OFF and LINCITYNG_EVOLUTION_CHECKS=ON for diagnostics. Targets: lincity-ng, community-checks, transit-checks, mayor-checks, city-evolution-checks. Bundled fmt/libxml++ must use the same C++ compiler as the game.

Tests: python3 tests/preview11_assets_check.py and python3 tests/gui_style_check.py. Native community/transit results and Windows import audit are in validation/. Windows graphical play-through remains unverified.

Audit: the earlier Changes-from-Claude.patch plus Preview10-to-Preview11.patch; full current source and binary artwork are included here. The baseline/current hash index is validation/current-changes-from-claude.json. Raw new art and export information are in art/preview11.

buy me cofe
https://ko-fi.com/levan4kgaming
