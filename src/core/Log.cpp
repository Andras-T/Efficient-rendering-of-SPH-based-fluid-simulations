#include "core/Log.hpp"

#include <cstdio>
#include <mutex>

namespace sph {

namespace {

std::string_view LevelTag(LogLevel level) {
  switch (level) {
    case LogLevel::Debug:
      return "debug";
    case LogLevel::Info:
      return "info";
    case LogLevel::Warning:
      return "warning";
    case LogLevel::Error:
      return "error";
  }
  return "?";
}

std::mutex& LogMutex() {
  static std::mutex mutex;
  return mutex;
}

}  // namespace

void WriteLog(LogLevel level, std::string_view message) {
  const std::string line = std::format("[{}] {}\n", LevelTag(level), message);
  const std::scoped_lock lock(LogMutex());
  std::fwrite(line.data(), 1, line.size(), stderr);
}

}  // namespace sph
