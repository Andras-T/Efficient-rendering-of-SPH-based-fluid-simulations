#include "core/Options.hpp"

#include <charconv>
#include <format>
#include <stdexcept>
#include <utility>

namespace sph {

namespace {

#ifdef NDEBUG
constexpr bool DefaultValidation = false;
#else
constexpr bool DefaultValidation = true;
#endif

std::uint32_t ParseNumber(std::string_view flag, std::string_view text) {
  std::uint32_t value = 0;
  const char* end = text.data() + text.size();
  const auto [last, error] = std::from_chars(text.data(), end, value);
  if (error != std::errc{} || last != end) {
    throw std::invalid_argument(std::format("{} expects a number, got '{}'", flag, text));
  }
  return value;
}

std::pair<std::uint32_t, std::uint32_t> ParseSize(std::string_view flag, std::string_view text) {
  const auto separator = text.find('x');
  if (separator == std::string_view::npos) {
    throw std::invalid_argument(std::format("{} expects WIDTHxHEIGHT, got '{}'", flag, text));
  }
  const auto width = ParseNumber(flag, text.substr(0, separator));
  const auto height = ParseNumber(flag, text.substr(separator + 1));
  if (width == 0 || height == 0) {
    throw std::invalid_argument(std::format("{} must not be zero, got '{}'", flag, text));
  }
  return {width, height};
}

}  // namespace

Options ParseOptions(std::span<const std::string_view> args) {
  Options options;
  options.validation = DefaultValidation;

  for (std::size_t i = 0; i < args.size(); ++i) {
    const std::string_view arg = args[i];
    const auto value = [&] {
      if (i + 1 >= args.size()) {
        throw std::invalid_argument(std::format("{} expects a value", arg));
      }
      return args[++i];
    };

    if (arg == "--headless") {
      options.headless = true;
    } else if (arg == "--frames") {
      options.frames = ParseNumber(arg, value());
    } else if (arg == "--size") {
      std::tie(options.width, options.height) = ParseSize(arg, value());
    } else if (arg == "--capture") {
      options.capturePath = value();
      options.headless = true;
    } else if (arg == "--sequence") {
      options.sequenceDirectory = value();
      options.headless = true;
    } else if (arg == "--scene") {
      options.scene = sim::ParseScenePreset(value());
    } else if (arg == "--view") {
      options.view = render::ParseDebugView(value());
    } else if (arg == "--solver") {
      options.solver = sim::ParseSolverMethod(value());
    } else if (arg == "--validation") {
      options.validation = true;
    } else if (arg == "--no-validation") {
      options.validation = false;
    } else if (arg == "--help" || arg == "-h") {
      options.showHelp = true;
    } else {
      throw std::invalid_argument(std::format("unknown option '{}'", arg));
    }
  }

  if (options.headless && options.frames == 0) {
    options.frames = 1;
  }
  return options;
}

std::string_view UsageText() {
  return R"(Usage: sph-fluid [options]

  --headless         render offscreen without a window
  --frames N         exit after N frames (headless default: 1)
  --size WxH         framebuffer size (default: 1280x720)
  --capture PATH     write the last frame as PNG (implies --headless)
  --sequence DIR     write every frame as DIR/0000.png (implies --headless)
  --scene NAME       empty, dam, drop, double, or dropin (default: dam)
  --view NAME        final, particles, density, velocity, cell, depth, smooth, thickness, normals, foam
  --solver NAME      wcsph or dfsph (default: wcsph)
  --validation       enable Vulkan validation layers (default in debug builds)
  --no-validation    disable Vulkan validation layers
  -h, --help         show this help
)";
}

}  // namespace sph
