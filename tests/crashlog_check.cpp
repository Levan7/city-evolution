// SPDX-License-Identifier: GPL-2.0-or-later
// Checks crash logging in child processes: uncaught exceptions are explained in
// stderr and in the log file, and native faults are noted but still die with the
// original signal so a core dump / crash report stays possible.
#include "lincity-ng/CrashLog.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
struct GameError { std::string text; };

bool describeGameError(std::string& out) {
  try { throw; }
  catch(const GameError& e) { out = "GameError: " + e.text; return true; }
  catch(...) { return false; }
}

[[noreturn]] void recurse(volatile int* p) { volatile char pad[4096]; pad[0] = char(*p); (void)pad[0]; recurse(p); }

[[noreturn]] void child(const std::string& mode, const std::string& logFile) {
  crashlog::install("City Evolution test-build 9.9", &describeGameError);
  crashlog::setLogFile(logFile);
  struct rlimit unlimited{RLIM_INFINITY, RLIM_INFINITY};
  setrlimit(RLIMIT_CORE, &unlimited);
  if(mode == "exception") {
    try { throw std::runtime_error("save file is broken"); }
    catch(...) { crashlog::reportCurrentException("main"); std::abort(); }
  }
  if(mode == "game-exception") {
    try { throw GameError{"not enough money for import"}; }
    catch(...) { crashlog::reportCurrentException("main"); std::abort(); }
  }
  if(mode == "terminate") {
    void (*thrower)() = [] { throw std::logic_error("thrown inside noexcept"); };
    [thrower]() noexcept { thrower(); }();
  }
  if(mode == "segv") { volatile int* p = nullptr; *p = 1; }
  if(mode == "stack") { volatile int x = 0; recurse(&x); }
  _exit(99);
}

struct Result { std::string err; int status; };

Result run(const std::string& mode, const std::string& logFile) {
  int fds[2]; if(pipe(fds)) std::exit(2);
  const pid_t pid = fork();
  if(pid == 0) { dup2(fds[1], 2); close(fds[0]); child(mode, logFile); }
  close(fds[1]);
  std::string err; char buf[512]; ssize_t n;
  while((n = read(fds[0], buf, sizeof buf)) > 0) err.append(buf, size_t(n));
  int status = 0; waitpid(pid, &status, 0);
  return {err, status};
}

std::string slurp(const std::string& f) { std::ifstream in(f); std::stringstream s; s << in.rdbuf(); return s.str(); }

int failures = 0;
void need(bool ok, const std::string& what) { if(!ok) { std::cerr << "FAIL: " << what << "\n"; ++failures; } }
bool has(const std::string& s, const char* t) { return s.find(t) != std::string::npos; }
}  // namespace

int main() {
  const std::string log = "/tmp/crashlog_check.log";
  std::remove(log.c_str());

  auto r = run("exception", log);
  need(has(r.err, "save file is broken") && has(r.err, "test-build 9.9"), "exception message and build version on stderr");
  need(WIFSIGNALED(r.status) && WTERMSIG(r.status) == SIGABRT, "uncaught exception still aborts (core dump possible)");
  need(has(slurp(log), "save file is broken"), "exception written to the log file");

  r = run("game-exception", log);
  need(has(r.err, "GameError: not enough money for import"), "game exception type described through the describer");

  r = run("terminate", log);
  need(has(r.err, "thrown inside noexcept") && has(r.err, "std::terminate"), "std::terminate explains the active exception");

  r = run("segv", log);
  need(has(r.err, "fatal signal 11") && has(r.err, "test-build 9.9"), "SIGSEGV noted with build version");
  need(WIFSIGNALED(r.status) && WTERMSIG(r.status) == SIGSEGV, "SIGSEGV still kills the process with SIGSEGV");
  need(has(slurp(log), "fatal signal 11"), "SIGSEGV noted in the log file");
  std::cout << "  segv core dumped flag: " << (WIFSIGNALED(r.status) && WCOREDUMP(r.status) ? "yes" : "no (depends on system core settings)") << "\n";

  r = run("stack", log);
  need(has(r.err, "fatal signal 11"), "stack overflow is logged (alternate signal stack)");
  need(WIFSIGNALED(r.status) && WTERMSIG(r.status) == SIGSEGV, "stack overflow still dies with SIGSEGV");

  if(failures) { std::cerr << failures << " crash logging check(s) failed\n"; return 1; }
  std::cout << "crash logging checks passed\n";
  return 0;
}
