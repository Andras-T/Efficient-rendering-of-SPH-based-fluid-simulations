#pragma once

#include "gpu/Vulkan.hpp"

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace sph {

// Writes tightly packed RGBA8 pixels; throws on failure.
void WritePng(const std::filesystem::path& path, VkExtent2D extent,
              std::span<const std::uint8_t> rgba);

// Reads an RGBA8 PNG; throws on failure.
std::vector<std::uint8_t> ReadPng(const std::filesystem::path& path, VkExtent2D& extent);

}  // namespace sph
