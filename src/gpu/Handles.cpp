#include "gpu/Handles.hpp"

#include <stdexcept>

namespace sph::gpu {

Unique<VkFence> CreateFence(VkDevice device, bool signaled) {
  const VkFenceCreateInfo info{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = signaled ? VkFenceCreateFlags{VK_FENCE_CREATE_SIGNALED_BIT} : 0,
  };
  VkFence fence = VK_NULL_HANDLE;
  Check(vkCreateFence(device, &info, nullptr, &fence), "vkCreateFence");
  return {device, fence, vkDestroyFence};
}

Unique<VkSemaphore> CreateBinarySemaphore(VkDevice device) {
  const VkSemaphoreCreateInfo info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkSemaphore semaphore = VK_NULL_HANDLE;
  Check(vkCreateSemaphore(device, &info, nullptr, &semaphore), "vkCreateSemaphore");
  return {device, semaphore, vkDestroySemaphore};
}

Unique<VkCommandPool> CreateCommandPool(VkDevice device, std::uint32_t queueFamily,
                                        VkCommandPoolCreateFlags flags) {
  const VkCommandPoolCreateInfo info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = flags,
      .queueFamilyIndex = queueFamily,
  };
  VkCommandPool pool = VK_NULL_HANDLE;
  Check(vkCreateCommandPool(device, &info, nullptr, &pool), "vkCreateCommandPool");
  return {device, pool, vkDestroyCommandPool};
}

Unique<VkShaderModule> CreateShaderModule(VkDevice device, std::span<const std::uint32_t> spirv) {
  if (spirv.empty()) {
    throw std::invalid_argument("empty SPIR-V module");
  }
  const VkShaderModuleCreateInfo info{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = spirv.size_bytes(),
      .pCode = spirv.data(),
  };
  VkShaderModule module = VK_NULL_HANDLE;
  Check(vkCreateShaderModule(device, &info, nullptr, &module), "vkCreateShaderModule");
  return {device, module, vkDestroyShaderModule};
}

}  // namespace sph::gpu
