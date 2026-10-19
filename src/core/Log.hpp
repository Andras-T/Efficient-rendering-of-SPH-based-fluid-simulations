#pragma once

#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

namespace sph {

enum class LogLevel : std::uint8_t { Debug, Info, Warning, Error };

void WriteLog(LogLevel level, std::string_view message);

template <typename... Args>
void LogInfo(std::format_string<Args...> format, Args&&... args) {
  WriteLog(LogLevel::Info, std::format(format, std::forward<Args>(args)...));
}

template <typename... Args>
void LogWarning(std::format_string<Args...> format, Args&&... args) {
  WriteLog(LogLevel::Warning, std::format(format, std::forward<Args>(args)...));
}

template <typename... Args>
void LogError(std::format_string<Args...> format, Args&&... args) {
  WriteLog(LogLevel::Error, std::format(format, std::forward<Args>(args)...));
}

}  // namespace sph
