// SPDX-License-Identifier: GPL-2.0-or-later
#include "CrashLog.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <system_error>
#include <typeinfo>

#if defined(__GNUC__)
#  include <cxxabi.h>
#endif

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <unistd.h>
#endif

namespace crashlog {
namespace {

char buildVersionText[128] = "unknown build";
ExceptionDescriber gDescriber = nullptr;
std::mutex gMutex;
std::filesystem::path gLogFile;

#ifdef _WIN32
HANDLE gLogHandle = INVALID_HANDLE_VALUE;
#else
volatile int gLogFd = -1;
#endif

// Readable type name (GCC/Clang mangle typeid names; MSVC does not).
std::string typeName(const std::type_info& info) {
#if defined(__GNUC__)
  int status = 0;
  char* demangled = abi::__cxa_demangle(info.name(), nullptr, nullptr, &status);
  if(status == 0 && demangled) {
    std::string out = demangled;
    std::free(demangled);
    return out;
  }
  std::free(demangled);
#endif
  return info.name();
}

std::string timestamp() {
  char buf[64];
  const std::time_t now = std::time(nullptr);
  std::tm tmv{};
#ifdef _WIN32
  localtime_s(&tmv, &now);
#else
  localtime_r(&now, &tmv);
#endif
  if(!std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &tmv))
    return "unknown time";
  return buf;
}

// Write to stderr and the crash log file. Used for ordinary C++ exceptions.
void writeReport(const std::string& text) {
  std::lock_guard<std::mutex> lock(gMutex);
  const std::string line = "[crash] " + timestamp() + " | " + buildVersionText
    + " | " + text + "\n";
  std::fputs(line.c_str(), stderr);
  std::fflush(stderr);
  if(!gLogFile.empty()) {
    if(std::FILE* f = std::fopen(gLogFile.string().c_str(), "ab")) {
      std::fputs(line.c_str(), f);
      std::fclose(f);
    }
  }
}

void terminateHandler() {
  if(std::current_exception())
    writeReport("std::terminate called with an active exception: "
      + describeCurrentException(gDescriber));
  else
    writeReport("std::terminate called without an active exception");
  // Keep the default outcome: abort() raises SIGABRT, which yields a core dump.
  std::abort();
}

#ifdef _WIN32

LONG WINAPI unhandledFilter(EXCEPTION_POINTERS* info) {
  char buf[320];
  const DWORD code = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode : 0;
  const void* where = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionAddress : nullptr;
  const char* name = "unknown";
  switch(code) {
    case EXCEPTION_ACCESS_VIOLATION: name = "access violation"; break;
    case EXCEPTION_STACK_OVERFLOW: name = "stack overflow"; break;
    case EXCEPTION_ILLEGAL_INSTRUCTION: name = "illegal instruction"; break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO: name = "integer divide by zero"; break;
    case EXCEPTION_IN_PAGE_ERROR: name = "page error"; break;
    default: break;
  }
  const int n = std::snprintf(buf, sizeof buf,
    "[crash] fatal exception 0x%08lX (%s) at %p | %s\r\n",
    static_cast<unsigned long>(code), name, where, buildVersionText);
  if(n > 0) {
    DWORD written = 0;
    const DWORD len = static_cast<DWORD>(n < int(sizeof buf) ? n : int(sizeof buf) - 1);
    if(HANDLE err = GetStdHandle(STD_ERROR_HANDLE); err && err != INVALID_HANDLE_VALUE)
      WriteFile(err, buf, len, &written, nullptr);
    if(gLogHandle != INVALID_HANDLE_VALUE)
      WriteFile(gLogHandle, buf, len, &written, nullptr);
  }
  // Let Windows continue with its normal crash handling (and any debugger).
  return EXCEPTION_CONTINUE_SEARCH;
}

void abortSignalHandler(int sig) {
  static const char note[] = "[crash] abort() called\r\n";
  DWORD written = 0;
  if(HANDLE err = GetStdHandle(STD_ERROR_HANDLE); err && err != INVALID_HANDLE_VALUE)
    WriteFile(err, note, sizeof note - 1, &written, nullptr);
  if(gLogHandle != INVALID_HANDLE_VALUE)
    WriteFile(gLogHandle, note, sizeof note - 1, &written, nullptr);
  std::signal(sig, SIG_DFL);
  std::raise(sig);
}

#else  // POSIX

struct SignalNote { int sig; const char* name; char text[320]; };
SignalNote gNotes[] = {
  {SIGSEGV, "SIGSEGV (segmentation fault)", {}},
  {SIGBUS,  "SIGBUS (bus error)", {}},
  {SIGFPE,  "SIGFPE (arithmetic error)", {}},
  {SIGILL,  "SIGILL (illegal instruction)", {}},
  {SIGABRT, "SIGABRT (abort)", {}},
};
alignas(16) char gAltStack[64 * 1024];

void signalHandler(int sig) {
  // Only async-signal-safe calls: write() of a pre-built string.
  for(const auto& note : gNotes) {
    if(note.sig != sig) continue;
    const size_t len = std::strlen(note.text);
    if(::write(2, note.text, len) < 0) { /* nothing useful to do */ }
    const int fd = gLogFd;
    if(fd >= 0 && ::write(fd, note.text, len) < 0) { /* ignore */ }
    break;
  }
  // SA_RESETHAND already restored the default action. Re-raise it so the
  // process still dies the normal way and the system can write a core dump.
  ::raise(sig);
}

#endif

}  // namespace

void install(const char* buildVersion, ExceptionDescriber describer) {
  std::snprintf(buildVersionText, sizeof buildVersionText, "%s",
    buildVersion ? buildVersion : "unknown build");
  gDescriber = describer;
  std::set_terminate(terminateHandler);

#ifdef _WIN32
  SetUnhandledExceptionFilter(unhandledFilter);
  std::signal(SIGABRT, abortSignalHandler);
#else
  for(auto& note : gNotes)
    std::snprintf(note.text, sizeof note.text,
      "[crash] fatal signal %d: %s | %s | a core dump may be available (see 'coredumpctl')\n",
      note.sig, note.name, buildVersionText);

  stack_t alt{};
  alt.ss_sp = gAltStack;
  alt.ss_size = sizeof gAltStack;
  alt.ss_flags = 0;
  const bool haveAltStack = sigaltstack(&alt, nullptr) == 0;  // lets us log a stack overflow

  for(const auto& note : gNotes) {
    struct sigaction sa{};
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND | SA_NODEFER | (haveAltStack ? SA_ONSTACK : 0);
    sigaction(note.sig, &sa, nullptr);
  }
#endif
}

void setLogFile(const std::filesystem::path& file) {
  {
    std::lock_guard<std::mutex> lock(gMutex);
    gLogFile = file;
  }
#ifdef _WIN32
  HANDLE h = CreateFileW(file.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
    nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  gLogHandle = h;
#else
  const int fd = ::open(file.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
  gLogFd = fd;
#endif
}

std::string describeCurrentException(ExceptionDescriber describer) {
  if(!std::current_exception()) return "no exception is being handled";
  if(!describer) describer = gDescriber;
  std::string out;
  if(describer) {
    try { if(describer(out)) return out; }
    catch(...) { out.clear(); }
  }
  try {
    throw;
  } catch(const std::system_error& e) {
    return typeName(typeid(e)) + " (code " + std::to_string(e.code().value())
      + ", " + e.code().category().name() + "): " + e.what();
  } catch(const std::exception& e) {
    return typeName(typeid(e)) + ": " + e.what();
  } catch(...) {
    return "unknown (non-standard) exception type";
  }
}

void reportCurrentException(const char* where) {
  writeReport(std::string("uncaught exception in ") + where + ": "
    + describeCurrentException(gDescriber));
}

}  // namespace crashlog
