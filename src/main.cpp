#include "app/Headless.hpp"
#include "app/Windowed.hpp"
#include "core/Log.hpp"
#include "core/Options.hpp"

#include <cstdio>
#include <exception>
#include <span>
#include <string_view>
#include <vector>

int main(int argc, char** argv) {
  try {
    const std::span<char*> rawArgs(argv, static_cast<std::size_t>(argc));
    const std::vector<std::string_view> args(rawArgs.begin() + 1, rawArgs.end());
    const sph::Options options = sph::ParseOptions(args);
    if (options.showHelp) {
      std::fputs(sph::UsageText().data(), stdout);
      return 0;
    }
    return options.headless ? sph::RunHeadless(options) : sph::RunWindowed(options);
  } catch (const std::exception& error) {
    sph::LogError("{}", error.what());
    return 1;
  }
}
