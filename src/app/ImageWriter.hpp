#pragma once

#include "gpu/Vulkan.hpp"

#include <cstdint>
#include <filesystem>
#include <span>

namespace sph {

// Writes tightly packed RGBA8 pixels; throws on failure.
void WritePng(const std::filesystem::path& path, VkExtent2D extent,
              std::span<const std::uint8_t> rgba);

}  // namespace sph
