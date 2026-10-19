#pragma once

#include <vk_mem_alloc.h>
#include <volk.h>

#include <string_view>

namespace sph::gpu {

// Loads the Vulkan loader once per process; throws if no loader is installed.
void LoadVulkan();

// Throws std::runtime_error naming `what` unless `result` is VK_SUCCESS.
void Check(VkResult result, std::string_view what);

}  // namespace sph::gpu
