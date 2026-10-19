#pragma once

#include "core/Options.hpp"
#include "gpu/Vulkan.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace sph {

namespace gpu {
class Context;
}

// Renders `frames` frames offscreen and returns the last one as tightly packed sRGB RGBA8.
// When `sequenceDirectory` is set, each frame is written as DIR/0000.png.
std::vector<std::uint8_t> RenderOffscreen(const gpu::Context& context, VkExtent2D extent,
                                          std::uint32_t frames,
                                          const std::filesystem::path& sequenceDirectory = {});

int RunHeadless(const Options& options);

}  // namespace sph
