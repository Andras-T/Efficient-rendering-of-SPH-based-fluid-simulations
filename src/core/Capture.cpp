#include "core/Capture.hpp"

#include <format>

namespace sph {

std::filesystem::path SequenceFramePath(const std::filesystem::path& directory,
                                        std::uint32_t frameIndex) {
  return directory / std::format("{:04}.png", frameIndex);
}

}  // namespace sph
