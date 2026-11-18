// SPDX-License-Identifier: GPL-2.0-or-later
// Crash reporting for uncaught C++ exceptions and fatal native signals.
//
// A C++ handler can explain an uncaught exception, but it cannot reliably
// diagnose every crash. For native faults (SIGSEGV, SIGBUS, stack overflow and
// similar) the handlers here only write a short note, then restore the default
// action so the operating system still produces a core dump or crash report.
#pragma once

#include <filesystem>
#include <string>

namespace crashlog {

// Lets the program describe its own exception types. It is called while an
// exception is being handled, so it may `throw;` inside a try block. It returns
// true and fills `description` when it recognised the exception.
using ExceptionDescriber = bool (*)(std::string& description);

// Install the handlers. Call first thing in main(), before anything can throw.
// Reports go to stderr (the launcher captures it into launch.log).
void install(const char* buildVersion, ExceptionDescriber describer = nullptr);

// Also append every report to this file (for example crash.log in the user
// data directory). Safe to call more than once; failures are ignored.
void setLogFile(const std::filesystem::path& file);

// Describe the exception currently being handled, as one readable string.
std::string describeCurrentException(ExceptionDescriber describer = nullptr);

// Log the exception currently being handled. Call from inside a catch block.
void reportCurrentException(const char* where);

}  // namespace crashlog
