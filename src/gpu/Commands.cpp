#include "gpu/Commands.hpp"

#include "gpu/Context.hpp"
#include "gpu/Handles.hpp"

#include <cstdint>
#include <limits>

namespace sph::gpu {

void ImmediateSubmit(const Context& context, const std::function<void(VkCommandBuffer)>& record) {
  const VkDevice device = context.device();
  const auto pool =
      CreateCommandPool(device, context.queueFamily(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

  const VkCommandBufferAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = pool.get(),
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  Check(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer),
        "vkAllocateCommandBuffers");

  const VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  Check(vkBeginCommandBuffer(commandBuffer, &beginInfo), "vkBeginCommandBuffer");
  record(commandBuffer);
  Check(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer");

  const auto fence = CreateFence(device, false);
  const VkCommandBufferSubmitInfo commandInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .commandBuffer = commandBuffer,
  };
  const VkSubmitInfo2 submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .commandBufferInfoCount = 1,
      .pCommandBufferInfos = &commandInfo,
  };
  Check(vkQueueSubmit2(context.queue(), 1, &submitInfo, fence.get()), "vkQueueSubmit2");
  const VkFence fenceHandle = fence.get();
  Check(
      vkWaitForFences(device, 1, &fenceHandle, VK_TRUE, std::numeric_limits<std::uint64_t>::max()),
      "vkWaitForFences");
}

void TransitionImage(VkCommandBuffer commandBuffer, const ImageTransition& transition) {
  const VkImageMemoryBarrier2 barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = transition.srcStage,
      .srcAccessMask = transition.srcAccess,
      .dstStageMask = transition.dstStage,
      .dstAccessMask = transition.dstAccess,
      .oldLayout = transition.oldLayout,
      .newLayout = transition.newLayout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = transition.image,
      .subresourceRange = {.aspectMask = transition.aspect, .levelCount = 1, .layerCount = 1},
  };
  const VkDependencyInfo dependency{
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &barrier,
  };
  vkCmdPipelineBarrier2(commandBuffer, &dependency);
}

void GlobalBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 srcStage,
                   VkAccessFlags2 srcAccess, VkPipelineStageFlags2 dstStage,
                   VkAccessFlags2 dstAccess) {
  const VkMemoryBarrier2 barrier{
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
      .srcStageMask = srcStage,
      .srcAccessMask = srcAccess,
      .dstStageMask = dstStage,
      .dstAccessMask = dstAccess,
  };
  const VkDependencyInfo dependency{
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .memoryBarrierCount = 1,
      .pMemoryBarriers = &barrier,
  };
  vkCmdPipelineBarrier2(commandBuffer, &dependency);
}

}  // namespace sph::gpu
