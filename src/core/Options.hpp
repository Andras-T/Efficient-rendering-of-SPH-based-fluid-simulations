#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>

namespace sph {

struct Options {
  bool headless = false;
  bool validation = false;
  bool showHelp = false;
  std::uint32_t width = 1280;
  std::uint32_t height = 720;
  // Frames to render before exiting; 0 runs until the window closes.
  std::uint32_t frames = 0;
  std::filesystem::path capturePath;
  std::filesystem::path sequenceDirectory;
};

// Throws std::invalid_argument on unknown flags or malformed values.
Options ParseOptions(std::span<const std::string_view> args);

std::string_view UsageText();

}  // namespace sph
