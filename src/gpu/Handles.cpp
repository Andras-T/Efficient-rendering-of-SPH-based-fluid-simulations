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

Unique<VkSampler> CreateSampler(VkDevice device, VkSamplerAddressMode addressU,
                                VkSamplerAddressMode addressV) {
  const VkSamplerCreateInfo info{
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = addressU,
      .addressModeV = addressV,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .maxLod = 0.0F,
  };
  VkSampler sampler = VK_NULL_HANDLE;
  Check(vkCreateSampler(device, &info, nullptr, &sampler), "vkCreateSampler");
  return {device, sampler, vkDestroySampler};
}

Unique<VkDescriptorSetLayout> CreateDescriptorSetLayout(
    VkDevice device, std::span<const VkDescriptorSetLayoutBinding> bindings) {
  const VkDescriptorSetLayoutCreateInfo info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<std::uint32_t>(bindings.size()),
      .pBindings = bindings.data(),
  };
  VkDescriptorSetLayout layout = VK_NULL_HANDLE;
  Check(vkCreateDescriptorSetLayout(device, &info, nullptr, &layout),
        "vkCreateDescriptorSetLayout");
  return {device, layout, vkDestroyDescriptorSetLayout};
}

Unique<VkDescriptorPool> CreateDescriptorPool(VkDevice device, std::uint32_t maxSets,
                                              std::span<const VkDescriptorPoolSize> sizes) {
  const VkDescriptorPoolCreateInfo info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = maxSets,
      .poolSizeCount = static_cast<std::uint32_t>(sizes.size()),
      .pPoolSizes = sizes.data(),
  };
  VkDescriptorPool pool = VK_NULL_HANDLE;
  Check(vkCreateDescriptorPool(device, &info, nullptr, &pool), "vkCreateDescriptorPool");
  return {device, pool, vkDestroyDescriptorPool};
}

VkDescriptorSet AllocateDescriptorSet(VkDevice device, VkDescriptorPool pool,
                                      VkDescriptorSetLayout layout) {
  const VkDescriptorSetAllocateInfo info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &layout,
  };
  VkDescriptorSet set = VK_NULL_HANDLE;
  Check(vkAllocateDescriptorSets(device, &info, &set), "vkAllocateDescriptorSets");
  return set;
}

void WriteCombinedImage(VkDevice device, VkDescriptorSet set, std::uint32_t binding,
                        VkImageView view, VkSampler sampler) {
  const VkDescriptorImageInfo image{
      .sampler = sampler,
      .imageView = view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
  const VkWriteDescriptorSet write{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = set,
      .dstBinding = binding,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &image,
  };
  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void WriteStorageImage(VkDevice device, VkDescriptorSet set, std::uint32_t binding,
                       VkImageView view) {
  const VkDescriptorImageInfo image{
      .imageView = view,
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
  };
  const VkWriteDescriptorSet write{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = set,
      .dstBinding = binding,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      .pImageInfo = &image,
  };
  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
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
