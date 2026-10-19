#pragma once

#include "gpu/Vulkan.hpp"

#include <functional>

namespace sph::gpu {

class Context;

// Records, submits, and waits for a one-off command buffer.
void ImmediateSubmit(const Context& context, const std::function<void(VkCommandBuffer)>& record);

struct ImageTransition {
  VkImage image = VK_NULL_HANDLE;
  VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkPipelineStageFlags2 srcStage = VK_PIPELINE_STAGE_2_NONE;
  VkAccessFlags2 srcAccess = VK_ACCESS_2_NONE;
  VkPipelineStageFlags2 dstStage = VK_PIPELINE_STAGE_2_NONE;
  VkAccessFlags2 dstAccess = VK_ACCESS_2_NONE;
  VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
};

void TransitionImage(VkCommandBuffer commandBuffer, const ImageTransition& transition);

void GlobalBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 srcStage,
                   VkAccessFlags2 srcAccess, VkPipelineStageFlags2 dstStage,
                   VkAccessFlags2 dstAccess);

}  // namespace sph::gpu
