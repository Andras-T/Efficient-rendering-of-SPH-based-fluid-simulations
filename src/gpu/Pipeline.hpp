#pragma once

#include "gpu/Handles.hpp"

#include <cstdint>
#include <span>

namespace sph::gpu {

Unique<VkPipelineLayout> CreatePipelineLayout(
    VkDevice device, std::span<const VkPushConstantRange> pushConstants,
    std::span<const VkDescriptorSetLayout> setLayouts = {});

struct GraphicsPipelineDesc {
  std::span<const std::uint32_t> spirv;
  const char* vertexEntry = "vsMain";
  const char* fragmentEntry = "fsMain";
  VkPipelineLayout layout = VK_NULL_HANDLE;
  std::span<const VkFormat> colorFormats;
  VkFormat depthFormat = VK_FORMAT_UNDEFINED;
  VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
  bool depthTest = false;
  bool depthWrite = false;
};

// Dynamic rendering pipeline with dynamic viewport and scissor.
Unique<VkPipeline> CreateGraphicsPipeline(VkDevice device, const GraphicsPipelineDesc& desc);

struct ComputePipelineDesc {
  std::span<const std::uint32_t> spirv;
  const char* entry = "csMain";
  VkPipelineLayout layout = VK_NULL_HANDLE;
};

Unique<VkPipeline> CreateComputePipeline(VkDevice device, const ComputePipelineDesc& desc);

}  // namespace sph::gpu
