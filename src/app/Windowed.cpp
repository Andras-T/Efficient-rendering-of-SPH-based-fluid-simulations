#include "app/Windowed.hpp"

#include "app/ImGuiLayer.hpp"
#include "app/Window.hpp"
#include "core/Log.hpp"
#include "gpu/Commands.hpp"
#include "gpu/Context.hpp"
#include "gpu/Handles.hpp"
#include "gpu/Swapchain.hpp"
#include "gpu/Timestamps.hpp"
#include "render/Camera.hpp"
#include "render/Renderer.hpp"
#include "ui/Hud.hpp"

#include <GLFW/glfw3.h>

#include <array>
#include <limits>
#include <vector>

namespace sph {

namespace {

constexpr std::size_t FramesInFlight = 2;
constexpr std::uint64_t NoTimeout = std::numeric_limits<std::uint64_t>::max();

struct FrameResources {
  gpu::Unique<VkCommandPool> commandPool;
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  gpu::Unique<VkSemaphore> imageAcquired;
  gpu::Unique<VkFence> inFlight;
};

FrameResources CreateFrameResources(const gpu::Context& context) {
  FrameResources frame;
  frame.commandPool = gpu::CreateCommandPool(context.device(), context.queueFamily());
  const VkCommandBufferAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = frame.commandPool.get(),
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  gpu::Check(vkAllocateCommandBuffers(context.device(), &allocateInfo, &frame.commandBuffer),
             "vkAllocateCommandBuffers");
  frame.imageAcquired = gpu::CreateBinarySemaphore(context.device());
  frame.inFlight = gpu::CreateFence(context.device(), true);
  return frame;
}

// Signalled by the submit and waited on by present; one per swapchain image so a semaphore
// is never reused while the presentation engine may still hold it.
std::vector<gpu::Unique<VkSemaphore>> CreateRenderFinished(const gpu::Context& context,
                                                           std::uint32_t count) {
  std::vector<gpu::Unique<VkSemaphore>> semaphores;
  semaphores.reserve(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    semaphores.push_back(gpu::CreateBinarySemaphore(context.device()));
  }
  return semaphores;
}

}  // namespace

int RunWindowed(const Options& options) {
  Window window("SPH Fluid", options.width, options.height);
  const gpu::Context context({.validation = options.validation, .window = window.handle()});
  gpu::Swapchain swapchain(context, window.framebufferExtent());
  const render::Renderer renderer(context, swapchain.format());
  ImGuiLayer imgui(context, window.handle(), swapchain.format(), swapchain.imageCount());
  gpu::GpuProfiler profiler(context, FramesInFlight);
  render::Camera camera;
  camera.setAspect(swapchain.extent());

  std::array<FrameResources, FramesInFlight> frames{};
  for (auto& frame : frames) {
    frame = CreateFrameResources(context);
  }
  auto renderFinished = CreateRenderFinished(context, swapchain.imageCount());

  const auto recreateSwapchain = [&] {
    context.waitIdle();
    swapchain.recreate(window.framebufferExtent());
    renderFinished = CreateRenderFinished(context, swapchain.imageCount());
    camera.setAspect(swapchain.extent());
  };

  std::uint64_t frameNumber = 0;
  while (!window.shouldClose() && (options.frames == 0 || frameNumber < options.frames)) {
    glfwPollEvents();
    const VkExtent2D extent = window.framebufferExtent();
    if (extent.width == 0 || extent.height == 0) {
      glfwWaitEvents();
      continue;
    }

    const auto flight = static_cast<std::uint32_t>(frameNumber % FramesInFlight);
    const FrameResources& frame = frames.at(flight);
    const VkFence inFlight = frame.inFlight.get();
    gpu::Check(vkWaitForFences(context.device(), 1, &inFlight, VK_TRUE, NoTimeout),
               "vkWaitForFences");
    profiler.resolve(flight);

    std::uint32_t imageIndex = 0;
    const VkResult acquired =
        vkAcquireNextImageKHR(context.device(), swapchain.handle(), NoTimeout,
                              frame.imageAcquired.get(), VK_NULL_HANDLE, &imageIndex);
    if (acquired == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapchain();
      continue;
    }
    if (acquired != VK_SUBOPTIMAL_KHR) {
      gpu::Check(acquired, "vkAcquireNextImageKHR");
    }

    gpu::Check(vkResetFences(context.device(), 1, &inFlight), "vkResetFences");
    gpu::Check(vkResetCommandPool(context.device(), frame.commandPool.get(), 0),
               "vkResetCommandPool");

    imgui.beginFrame();
    ui::ApplyCameraInput(camera);
    ui::DrawHud(context.deviceName(), swapchain.extent(), camera, profiler.intervals());
    imgui.endFrame();

    const VkCommandBuffer commandBuffer = frame.commandBuffer;
    const VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    gpu::Check(vkBeginCommandBuffer(commandBuffer, &beginInfo), "vkBeginCommandBuffer");
    const VkImage image = swapchain.image(imageIndex);
    profiler.beginFrame(commandBuffer, flight);
    renderer.record(commandBuffer, {image, swapchain.view(imageIndex), swapchain.extent()},
                    [&](VkCommandBuffer overlay) { imgui.record(overlay); });
    profiler.mark(commandBuffer, "render");
    profiler.endFrame(commandBuffer);
    gpu::TransitionImage(commandBuffer,
                         {
                             .image = image,
                             .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                             .srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                             .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                         });
    gpu::Check(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer");

    const VkSemaphore renderDone = renderFinished.at(imageIndex).get();
    const VkSemaphoreSubmitInfo waitInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = frame.imageAcquired.get(),
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    const VkSemaphoreSubmitInfo signalInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = renderDone,
        .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    };
    const VkCommandBufferSubmitInfo commandInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = commandBuffer,
    };
    const VkSubmitInfo2 submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &waitInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &commandInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signalInfo,
    };
    gpu::Check(vkQueueSubmit2(context.queue(), 1, &submitInfo, inFlight), "vkQueueSubmit2");

    const VkSwapchainKHR swapchainHandle = swapchain.handle();
    const VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderDone,
        .swapchainCount = 1,
        .pSwapchains = &swapchainHandle,
        .pImageIndices = &imageIndex,
    };
    const VkResult presented = vkQueuePresentKHR(context.queue(), &presentInfo);
    if (presented == VK_ERROR_OUT_OF_DATE_KHR || presented == VK_SUBOPTIMAL_KHR ||
        window.consumeResize()) {
      recreateSwapchain();
    } else {
      gpu::Check(presented, "vkQueuePresentKHR");
    }
    ++frameNumber;
  }

  context.waitIdle();
  if (context.validationErrorCount() > 0) {
    LogError("{} Vulkan validation error(s)", context.validationErrorCount());
    return 1;
  }
  return 0;
}

}  // namespace sph
