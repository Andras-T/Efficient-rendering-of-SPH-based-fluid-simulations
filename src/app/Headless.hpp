#pragma once

#include "core/Options.hpp"
#include "gpu/Vulkan.hpp"

#include <cstdint>
#include <vector>

namespace sph {

namespace gpu {
class Context;
}

// Renders `frames` frames offscreen and returns the last one as tightly packed sRGB RGBA8.
std::vector<std::uint8_t> RenderOffscreen(const gpu::Context& context, VkExtent2D extent,
                                          std::uint32_t frames);

int RunHeadless(const Options& options);

}  // namespace sph
