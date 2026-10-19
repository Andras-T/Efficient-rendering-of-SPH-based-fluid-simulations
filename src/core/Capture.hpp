#pragma once

#include <cstdint>
#include <filesystem>

namespace sph {

// `directory/{frameIndex:04}.png`
std::filesystem::path SequenceFramePath(const std::filesystem::path& directory,
                                        std::uint32_t frameIndex);

}  // namespace sph
