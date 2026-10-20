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
#include "sim/CpuSolver.hpp"
#include "sim/Scene.hpp"
#include "sim/SphSolver.hpp"
#include "sim/Tank.hpp"
#include "ui/Hud.hpp"

#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

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
  render::Renderer renderer(context, swapchain.format());
  renderer.settings().view = options.view;
  renderer.settings().walls = render::WallDisplay::Glass;
  renderer.resize(swapchain.extent());
  ImGuiLayer imgui(context, window.handle(), swapchain.format(), VK_FORMAT_UNDEFINED,
                   swapchain.imageCount());
  gpu::GpuProfiler profiler(context, FramesInFlight);
  sim::SimParams startParams;
  startParams.solver = options.solver;
  startParams.tankShape = options.tank;
  sim::Scene scene = sim::BuildScene(options.scene, startParams);
  sim::CalibrateRestDensity(scene.particles, scene.params);
  sim::SphSolver solver(context);
  solver.upload(scene.particles, scene.params);
  render::Camera camera;
  camera.setAspect(swapchain.extent());
  camera.frame(scene.params.domainMin, scene.params.domainMax);
  sim::ScenePreset preset = options.scene;
  bool paused = true;
  float paddleRadius = 0.10F;
  float paddleSwirl = 6.0F;
  bool showBlock = true;
  ui::CursorDrag cursorDrag;
  const glm::vec3 blockHalf = sim::WoodenBlockHalf;
  glm::vec3 blockCenter = sim::WoodenBlockStart(preset, scene.params);
  glm::vec3 blockVelocity{0.0F};
  const auto placeBlock = [&] {
    blockCenter = sim::WoodenBlockStart(preset, scene.params);
    blockVelocity = {};
    if (preset == sim::ScenePreset::DropIn) {
      showBlock = true;
    }
  };
  placeBlock();

  std::array<FrameResources, FramesInFlight> frames{};
  for (auto& frame : frames) {
    frame = CreateFrameResources(context);
  }
  auto renderFinished = CreateRenderFinished(context, swapchain.imageCount());

  const auto recreateSwapchain = [&] {
    context.waitIdle();
    swapchain.recreate(window.framebufferExtent());
    renderFinished = CreateRenderFinished(context, swapchain.imageCount());
    renderer.resize(swapchain.extent());
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
    // Particle buffers are updated in place, so consecutive frames cannot overlap.
    if (frameNumber > 0) {
      const auto previous = static_cast<std::uint32_t>((frameNumber - 1) % FramesInFlight);
      const VkFence previousFence = frames.at(previous).inFlight.get();
      gpu::Check(vkWaitForFences(context.device(), 1, &previousFence, VK_TRUE, NoTimeout),
                 "vkWaitForFences");
    }

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
    ui::ApplyCameraInput(camera, solver.params().domainMin, solver.params().domainMax);
    ui::HudState hud{
        .deviceName = context.deviceName(),
        .extent = swapchain.extent(),
        .camera = &camera,
        .timings = profiler.intervals(),
        .preset = preset,
        .particleCount = solver.count(),
        .paused = paused,
        .timeStep = solver.params().timeStep,
        .substeps = solver.params().substeps,
        .restDensity = solver.params().restDensity,
        .viscosity = solver.params().viscosity,
        .solver = solver.params().solver,
        .tankShape = solver.params().tankShape,
        .view = renderer.settings().view,
        .walls = renderer.settings().walls,
        .paddleRadius = paddleRadius,
        .paddleSwirl = paddleSwirl,
        .showBlock = showBlock,
        .exposure = renderer.settings().exposure,
        .absorption = renderer.settings().absorption,
    };
    const ui::HudResult hudResult = ui::DrawHud(hud);
    paddleRadius = hud.paddleRadius;
    paddleSwirl = hud.paddleSwirl;
    showBlock = hud.showBlock;
    if (hudResult.sceneChanged || hudResult.resetScene) {
      preset = hud.preset;
      sim::SimParams next;
      next.solver = hud.solver;
      next.viscosity = hud.viscosity;
      next.tankShape = hud.tankShape;
      scene = sim::BuildScene(preset, next);
      sim::CalibrateRestDensity(scene.particles, scene.params);
      solver.upload(scene.particles, scene.params);
      cursorDrag = {};
      placeBlock();
      camera.frame(scene.params.domainMin, scene.params.domainMax);
      camera.setAspect(swapchain.extent());
      hud.paused = true;
    } else {
      if (hud.viscosity != solver.params().viscosity) {
        solver.setViscosity(hud.viscosity);
      }
      if (hud.solver != solver.params().solver) {
        solver.setSolver(hud.solver);
      }
    }
    paused = hud.paused;
    renderer.settings().view = hud.view;
    renderer.settings().walls = hud.walls;
    renderer.settings().exposure = hud.exposure;
    renderer.settings().absorption = hud.absorption;
    const ui::CursorForce cursor =
        ui::SamplePaddle(camera, solver.params().domainMin, solver.params().domainMax, paused,
                         cursorDrag, paddleRadius, paddleSwirl);
    solver.setCursor(cursor.position, cursor.stirring ? cursor.force : glm::vec3{},
                     cursor.stirring ? cursor.radius : 0.0F, cursor.stirring ? cursor.swirl : 0.0F,
                     cursor.stirring ? cursor.push : 0.0F);
    if (showBlock && cursor.grabbing && cursorDrag.haveHit) {
      const glm::vec3 closest =
          glm::clamp(cursor.position, blockCenter - blockHalf, blockCenter + blockHalf);
      if (glm::length(cursor.position - closest) < cursor.radius) {
        blockCenter += cursor.force * (1.0F / 1800.0F);
        blockVelocity = {};
      }
    }
    if (!paused && showBlock && preset == sim::ScenePreset::DropIn && !cursor.grabbing) {
      sim::AdvanceWoodenBlock(blockCenter, blockVelocity, solver.params());
    }
    sim::ClampAabbToTank(blockCenter, blockHalf, solver.params());
    solver.setBlock(blockCenter - blockHalf, blockCenter + blockHalf, showBlock);
    renderer.settings().paddleCenter = cursor.position;
    renderer.settings().paddleRadius = cursor.visible ? cursor.radius : 0.0F;
    renderer.settings().blockMin = blockCenter - blockHalf;
    renderer.settings().blockMax = blockCenter + blockHalf;
    renderer.settings().block = showBlock;
    imgui.endFrame();

    const VkCommandBuffer commandBuffer = frame.commandBuffer;
    const VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    gpu::Check(vkBeginCommandBuffer(commandBuffer, &beginInfo), "vkBeginCommandBuffer");
    const VkImage image = swapchain.image(imageIndex);
    profiler.beginFrame(commandBuffer, flight);
    if (!paused) {
      solver.record(commandBuffer, solver.params().substeps);
      gpu::GlobalBarrier(
          commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
          VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
          VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
          VK_ACCESS_2_SHADER_STORAGE_READ_BIT);
      profiler.mark(commandBuffer, "sim");
    }
    renderer.record(commandBuffer, {image, swapchain.view(imageIndex), swapchain.extent()}, camera,
                    solver.particleAddress(), solver.count(), solver.foamAddress(),
                    solver.foamCapacity(), solver.params(),
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
