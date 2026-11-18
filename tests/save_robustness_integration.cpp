// SPDX-License-Identifier: GPL-2.0-or-later
// A damaged save must produce a readable load error (std::runtime_error, which
// loadCityNG turns into the "Failed to load game" dialog and returns the player
// to the menu) and never terminate the program.
#include "lincity/init_game.hpp"
#include "lincity/modules/all_modules.hpp"
#include "lincity/world.hpp"
#include <zlib.h>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void need(bool ok, const std::string& what) { if(!ok) throw std::runtime_error(what); }

std::string readGz(const std::filesystem::path& file) {
  gzFile g = gzopen(file.string().c_str(), "rb");
  need(g != nullptr, "cannot open " + file.string());
  std::string text; char buf[65536]; int n;
  while((n = gzread(g, buf, sizeof buf)) > 0) text.append(buf, size_t(n));
  gzclose(g);
  return text;
}

void writeGz(const std::filesystem::path& file, const std::string& text) {
  gzFile g = gzopen(file.string().c_str(), "wb");
  need(g != nullptr, "cannot write " + file.string());
  gzwrite(g, text.data(), unsigned(text.size()));
  gzclose(g);
}

// Replace the contents of <tag>...</tag> in a saved game.
std::string withTagValue(std::string xml, const std::string& tag, const std::string& value) {
  const std::string open = "<" + tag + ">", close = "</" + tag + ">";
  const auto a = xml.find(open);
  need(a != std::string::npos, "saved game has no <" + tag + "> element");
  const auto b = xml.find(close, a);
  need(b != std::string::npos, "unterminated <" + tag + ">");
  return xml.substr(0, a + open.size()) + value + xml.substr(b);
}

enum class Outcome { Loaded, LoadError, OtherException };

struct Attempt { Outcome outcome; std::string message; long long exportsThisYear = 0; };

Attempt tryLoad(const std::filesystem::path& file) {
  try {
    auto w = World::load(file);
    return {Outcome::Loaded, "", w->taxable.exportsThisYear};
  } catch(const std::runtime_error& e) {       // what loadCityNG catches
    return {Outcome::LoadError, e.what()};
  } catch(const std::exception& e) {            // would terminate the game
    return {Outcome::OtherException, e.what()};
  }
}
}  // namespace

int main(int argc, char** argv) {
  try {
    initializeModules();
    const std::filesystem::path dir = std::filesystem::temp_directory_path() / "save_robustness";
    std::filesystem::create_directories(dir);
#ifdef FIXTURE_DIR
    const std::filesystem::path defaultFixture = std::filesystem::path(FIXTURE_DIR) / "Transport-Sandbox.scn.gz";
#else
    const std::filesystem::path defaultFixture = "tests/fixtures/Transport-Sandbox.scn.gz";
#endif
    const std::filesystem::path fixture = argc > 1 ? std::filesystem::path(argv[1]) : defaultFixture;

    // 1. A valid older save (written before exportsThisYear existed) still loads;
    //    the missing optional field keeps its default.
    need(readGz(fixture).find("exportsThisYear") == std::string::npos,
      "fixture is expected to predate exportsThisYear");
    auto older = tryLoad(fixture);
    need(older.outcome == Outcome::Loaded, "older save without exportsThisYear failed to load: " + older.message);
    need(older.exportsThisYear == 0, "missing exportsThisYear did not keep its default");

    // 2. A current save round-trips the 64-bit value.
    {
      auto w = World::load(fixture);
      w->taxable.exportsThisYear = 123456789012LL;
      w->save(dir / "current.scn.gz");
    }
    auto current = tryLoad(dir / "current.scn.gz");
    need(current.outcome == Outcome::Loaded && current.exportsThisYear == 123456789012LL,
      "current save did not round-trip exportsThisYear");
    const std::string xml = readGz(dir / "current.scn.gz");

    // 3. Values that are accepted.
    struct Good { const char* text; long long expected; };
    for(Good g : {Good{" 42 ", 42}, Good{"0007", 7}, Good{"9223372036854775807", 9223372036854775807LL},
                  Good{"-5", 0 /* negative totals are clamped, as before */}}) {
      writeGz(dir / "good.scn.gz", withTagValue(xml, "exportsThisYear", g.text));
      auto r = tryLoad(dir / "good.scn.gz");
      need(r.outcome == Outcome::Loaded && r.exportsThisYear == g.expected,
        std::string("valid exportsThisYear \"") + g.text + "\" was not accepted: " + r.message);
    }

    // 4. Malformed or out-of-range values give a readable load error, not a crash
    //    and not a logic_error (which nothing on the load path catches).
    for(const char* bad : {"abc", "12abc", "", "   ", "1.5", "0x10", "9223372036854775808",
                           "99999999999999999999999"}) {
      writeGz(dir / "bad.scn.gz", withTagValue(xml, "exportsThisYear", bad));
      auto r = tryLoad(dir / "bad.scn.gz");
      need(r.outcome != Outcome::OtherException,
        std::string("exportsThisYear \"") + bad + "\" threw a non-runtime_error exception: " + r.message);
      need(r.outcome == Outcome::LoadError,
        std::string("malformed exportsThisYear \"") + bad + "\" was accepted");
      need(r.message.find("exportsThisYear") != std::string::npos,
        std::string("load error does not name the damaged field: ") + r.message);
    }

    // 5. Trailing text in an ordinary numeric field used to assert() and abort
    //    debug builds before any load error could be shown. Reaching the next
    //    line at all is the check; the load may succeed or report an error.
    writeGz(dir / "trailing.scn.gz", withTagValue(xml, "total_time", "5abc"));
    auto trailing = tryLoad(dir / "trailing.scn.gz");
    need(trailing.outcome != Outcome::OtherException, "trailing characters threw: " + trailing.message);

    std::filesystem::remove_all(dir);
    std::cout << "PASS older save, round trip, accepted values, damaged-save errors, trailing text\n";
  } catch(const std::exception& e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
