#pragma once

#include "Vulkan/Utils/Utils.h"
#include <iostream>
#include <string>

enum Severity { Info, Diag, Trace };

class Logger {
private:
  Logger() { severity = Info; }

  Severity severity;
  const char* endFormat = "\x1b[0m";

public:
  void setSeverity(Severity severity) { this->severity = severity; }

  static Logger &getInstance() {
    static Logger instance;
    return instance;
  }

  Logger(Logger const &) = delete;
  void operator=(Logger const &) = delete;

  void LogInfo(const std::string &log,
               const std::string &prefix = Utils::ENGINE,
               const std::string_view &color = "\x1b[38m");

  void LogDiag(const std::string &log,
               const std::string &prefix = Utils::ENGINE,
               const std::string_view &color = "\x1b[38m");

  void LogTrace(const std::string &log,
                const std::string &prefix = Utils::ENGINE,
                const std::string_view &color = "\x1b[38m");

  void LogError(const std::string &log,
                const std::string &prefix = Utils::ENGINE);

  std::ostringstream formatMessage(const std::string &messageStr,
                                   bool addDoubleSpace = false,
                                   int indentation = 36, int lineLimit = 170);
};