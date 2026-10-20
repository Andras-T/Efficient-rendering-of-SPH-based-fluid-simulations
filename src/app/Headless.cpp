#include "app/Headless.hpp"

#include "app/ImageWriter.hpp"
#include "core/Capture.hpp"
#include "core/Log.hpp"
#include "gpu/Buffer.hpp"
#include "gpu/Commands.hpp"
#include "gpu/Context.hpp"
#include "gpu/Image.hpp"
#include "gpu/Timestamps.hpp"
#include "render/Renderer.hpp"
#include "sim/CpuSolver.hpp"
#include "sim/SmokeSolver.hpp"
#include "sim/SphSolver.hpp"
#include "sim/WindSolver.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>

namespace sph {

namespace {

constexpr VkFormat CaptureFormat = VK_FORMAT_R8G8B8A8_SRGB;
constexpr std::uint32_t BytesPerPixel = 4;

void CopyTargetToBuffer(VkCommandBuffer commandBuffer, VkImage image, VkBuffer buffer,
                        VkExtent2D extent) {
  gpu::TransitionImage(commandBuffer,
                       {
                           .image = image,
                           .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           .srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                           .dstStage = VK_PIPELINE_STAGE_2_COPY_BIT,
                           .dstAccess = VK_ACCESS_2_TRANSFER_READ_BIT,
                       });
  const VkBufferImageCopy region{
      .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1},
      .imageExtent = {extent.width, extent.height, 1},
  };
  vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1,
                         &region);
  gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                     VK_PIPELINE_STAGE_2_HOST_BIT, VK_ACCESS_2_HOST_READ_BIT);
}

}  // namespace

std::vector<std::uint8_t> RenderWindOffscreen(const gpu::Context& context,
                                              const OffscreenRequest& request) {
  const VkExtent2D extent = request.extent;
  const gpu::Image target(
      context, {
                   .extent = extent,
                   .format = CaptureFormat,
                   .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
               });
  const VkDeviceSize byteCount = VkDeviceSize{extent.width} * extent.height * BytesPerPixel;
  const gpu::Buffer readback(context,
                             {
                                 .size = byteCount,
                                 .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                                                    VMA_ALLOCATION_CREATE_MAPPED_BIT,
                             });
  sim::WindSolver wind(context);
  render::Camera ownedCamera;
  ownedCamera.setAspect(extent);
  ownedCamera.setTarget({0.02F, 0.12F, 0.0F});
  ownedCamera.setYawPitch(0.95F, 0.32F);
  ownedCamera.setDistance(1.30F);
  const render::Camera& camera = request.camera == nullptr ? ownedCamera : *request.camera;
  render::Renderer renderer(context, CaptureFormat);
  renderer.settings().view = request.view;
  renderer.settings().windCar = request.windCar;
  renderer.settings().windTracers = request.windTracers;
  renderer.resize(extent);
  const render::RenderTarget renderTarget{target.handle(), target.view(), extent};
  if (!request.sequenceDirectory.empty()) {
    std::filesystem::create_directories(request.sequenceDirectory);
  }
  std::vector<std::uint8_t> lastFrame;
  const std::uint32_t frameCount = std::max(request.frames, 1U);
  for (std::uint32_t frame = 0; frame < frameCount; ++frame) {
    const bool needPixels = !request.sequenceDirectory.empty() || (frame + 1 >= frameCount);
    gpu::ImmediateSubmit(context, [&](VkCommandBuffer commandBuffer) {
      if (request.simulate) {
        wind.record(commandBuffer, 1);
      }
      renderer.recordWind(commandBuffer, renderTarget, camera, wind.meshAddress(),
                          wind.meshVertexCount(), wind.tracerAddress(), wind.tracerCount(),
                          wind.domainMin(), wind.domainMax());
      if (needPixels) {
        CopyTargetToBuffer(commandBuffer, target.handle(), readback.handle(), extent);
      }
    });
    if (!needPixels) {
      continue;
    }
    readback.invalidate();
    const auto pixels = readback.mapped<const std::uint8_t>();
    if (!request.sequenceDirectory.empty()) {
      WritePng(SequenceFramePath(request.sequenceDirectory, frame), extent, pixels);
    }
    if (frame + 1 >= frameCount) {
      lastFrame.assign(pixels.begin(), pixels.end());
    }
  }
  return lastFrame;
}

std::vector<std::uint8_t> RenderSmokeOffscreen(const gpu::Context& context,
                                               const OffscreenRequest& request) {
  const VkExtent2D extent = request.extent;
  const gpu::Image target(
      context, {
                   .extent = extent,
                   .format = CaptureFormat,
                   .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
               });
  const VkDeviceSize byteCount = VkDeviceSize{extent.width} * extent.height * BytesPerPixel;
  const gpu::Buffer readback(context,
                             {
                                 .size = byteCount,
                                 .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                                                    VMA_ALLOCATION_CREATE_MAPPED_BIT,
                             });
  sim::SmokeSolver smoke(context);
  render::Camera ownedCamera;
  ownedCamera.setAspect(extent);
  ownedCamera.setTarget({0.0F, 0.85F, 0.0F});
  ownedCamera.setYawPitch(0.85F, 0.22F);
  ownedCamera.setDistance(3.35F);
  const render::Camera& camera = request.camera == nullptr ? ownedCamera : *request.camera;
  render::Renderer renderer(context, CaptureFormat);
  renderer.settings().view = request.view;
  renderer.resize(extent);
  const render::RenderTarget renderTarget{target.handle(), target.view(), extent};
  if (!request.sequenceDirectory.empty()) {
    std::filesystem::create_directories(request.sequenceDirectory);
  }
  std::vector<std::uint8_t> lastFrame;
  const std::uint32_t frameCount = std::max(request.frames, 1U);
  for (std::uint32_t frame = 0; frame < frameCount; ++frame) {
    const bool needPixels = !request.sequenceDirectory.empty() || (frame + 1 >= frameCount);
    gpu::ImmediateSubmit(context, [&](VkCommandBuffer commandBuffer) {
      if (request.simulate) {
        smoke.record(commandBuffer, 1);
      }
      renderer.recordSmoke(commandBuffer, renderTarget, camera, smoke.scalarAddress(),
                           smoke.velocityAddress(), smoke.domain().grid, smoke.domain().origin,
                           smoke.domain().cellSize, smoke.domainMin(), smoke.domainMax());
      if (needPixels) {
        CopyTargetToBuffer(commandBuffer, target.handle(), readback.handle(), extent);
      }
    });
    if (!needPixels) {
      continue;
    }
    readback.invalidate();
    const auto pixels = readback.mapped<const std::uint8_t>();
    if (!request.sequenceDirectory.empty()) {
      WritePng(SequenceFramePath(request.sequenceDirectory, frame), extent, pixels);
    }
    if (frame + 1 >= frameCount) {
      lastFrame.assign(pixels.begin(), pixels.end());
    }
  }
  return lastFrame;
}

std::vector<std::uint8_t> RenderOffscreen(const gpu::Context& context,
                                          const OffscreenRequest& request) {
  if (request.simulation == SimulationKind::WindTunnel) {
    return RenderWindOffscreen(context, request);
  }
  if (request.simulation == SimulationKind::Smoke) {
    return RenderSmokeOffscreen(context, request);
  }
  const VkExtent2D extent = request.extent;
  const gpu::Image target(
      context, {
                   .extent = extent,
                   .format = CaptureFormat,
                   .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
               });
  const VkDeviceSize byteCount = VkDeviceSize{extent.width} * extent.height * BytesPerPixel;
  const gpu::Buffer readback(context,
                             {
                                 .size = byteCount,
                                 .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                                                    VMA_ALLOCATION_CREATE_MAPPED_BIT,
                             });

  sim::SimParams ownedParams;
  ownedParams.solver = request.solver;
  ownedParams.tankShape = request.tankShape;
  const sim::Scene ownedScene =
      request.scene == nullptr ? sim::BuildScene(request.preset, ownedParams) : sim::Scene{};
  const sim::Scene& scene = request.scene == nullptr ? ownedScene : *request.scene;

  render::Camera ownedCamera;
  ownedCamera.setAspect(extent);
  ownedCamera.frame(scene.params.domainMin, scene.params.domainMax);
  const render::Camera& camera = request.camera == nullptr ? ownedCamera : *request.camera;

  sim::Scene simulated = scene;
  simulated.params.solver = request.solver;
  if (request.simulate && !simulated.particles.empty()) {
    sim::CalibrateRestDensity(simulated.particles, simulated.params);
  }
  sim::SphSolver solver(context);
  solver.upload(simulated.particles, simulated.params);

  render::Renderer renderer(context, CaptureFormat);
  renderer.settings().view = request.view;
  renderer.settings().walls = request.walls;
  renderer.settings().paddleCenter = request.paddleCenter;
  renderer.settings().paddleRadius = request.paddleRadius;
  renderer.settings().blockMin = request.blockMin;
  renderer.settings().blockMax = request.blockMax;
  renderer.settings().block = request.block;
  renderer.resize(extent);
  solver.setCursor(request.paddleCenter, request.paddleForce, request.paddleRadius, 0.0F,
                   request.paddlePush);
  const bool fallingBlock = request.preset == sim::ScenePreset::DropIn;
  glm::vec3 blockCenter = 0.5F * (request.blockMin + request.blockMax);
  glm::vec3 blockHalf = 0.5F * (request.blockMax - request.blockMin);
  glm::vec3 blockVelocity{0.0F};
  bool showBlock = request.block;
  if (fallingBlock) {
    showBlock = true;
    blockHalf = sim::WoodenBlockHalf;
    blockCenter = sim::WoodenBlockStart(sim::ScenePreset::DropIn, solver.params());
  }
  solver.setBlock(blockCenter - blockHalf, blockCenter + blockHalf, showBlock);
  renderer.settings().blockMin = blockCenter - blockHalf;
  renderer.settings().blockMax = blockCenter + blockHalf;
  renderer.settings().block = showBlock;
  const render::RenderTarget renderTarget{target.handle(), target.view(), extent};
  gpu::GpuProfiler profiler(context, 1);

  if (!request.sequenceDirectory.empty()) {
    std::filesystem::create_directories(request.sequenceDirectory);
  }

  std::vector<std::uint8_t> lastFrame;
  const std::uint32_t frameCount = std::max(request.frames, 1U);
  for (std::uint32_t frame = 0; frame < frameCount; ++frame) {
    const bool needPixels = !request.sequenceDirectory.empty() || (frame + 1 >= frameCount);
    gpu::ImmediateSubmit(context, [&](VkCommandBuffer commandBuffer) {
      profiler.beginFrame(commandBuffer, 0);
      if (request.simulate) {
        if (showBlock && frame > 0 && (request.buoyant || fallingBlock)) {
          sim::AdvanceWoodenBlock(blockCenter, blockVelocity, solver.params(),
                                  solver.blockWaterSurface(), request.buoyant);
          solver.setBlock(blockCenter - blockHalf, blockCenter + blockHalf, true);
          renderer.settings().blockMin = blockCenter - blockHalf;
          renderer.settings().blockMax = blockCenter + blockHalf;
          renderer.settings().block = true;
        }
        solver.record(commandBuffer, solver.params().substeps);
        gpu::GlobalBarrier(
            commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_SHADER_STORAGE_READ_BIT);
        profiler.mark(commandBuffer, "sim");
      }
      renderer.record(commandBuffer, renderTarget, camera, solver.particleAddress(), solver.count(),
                      solver.foamAddress(), solver.foamCapacity(), solver.params());
      profiler.mark(commandBuffer, "render");
      profiler.endFrame(commandBuffer);
      if (needPixels) {
        CopyTargetToBuffer(commandBuffer, target.handle(), readback.handle(), extent);
      }
    });
    profiler.resolve(0);

    if (!needPixels) {
      continue;
    }
    readback.invalidate();
    const auto pixels = readback.mapped<const std::uint8_t>();
    if (!request.sequenceDirectory.empty()) {
      WritePng(SequenceFramePath(request.sequenceDirectory, frame), extent, pixels);
    }
    if (frame + 1 >= frameCount) {
      lastFrame.assign(pixels.begin(), pixels.end());
    }
  }

  if (profiler.supported()) {
    for (const gpu::GpuProfiler::Interval& interval : profiler.intervals()) {
      LogInfo("GPU {}: {:.3f} ms", interval.name, interval.milliseconds);
    }
  }

  return lastFrame;
}

std::vector<std::uint8_t> RenderOffscreen(const gpu::Context& context, VkExtent2D extent,
                                          std::uint32_t frames,
                                          const std::filesystem::path& sequenceDirectory) {
  return RenderOffscreen(
      context, {.extent = extent, .frames = frames, .sequenceDirectory = sequenceDirectory});
}

int RunHeadless(const Options& options) {
  const gpu::Context context({.validation = options.validation});
  const VkExtent2D extent{options.width, options.height};

  const auto start = std::chrono::steady_clock::now();
  const auto pixels = RenderOffscreen(context, {.extent = extent,
                                                .frames = options.frames,
                                                .sequenceDirectory = options.sequenceDirectory,
                                                .preset = options.scene,
                                                .simulate = true,
                                                .view = options.view,
                                                .walls = render::WallDisplay::Glass,
                                                .solver = options.solver,
                                                .tankShape = options.tank,
                                                .simulation = options.simulation});
  const std::chrono::duration<double, std::milli> elapsed =
      std::chrono::steady_clock::now() - start;
  LogInfo("Rendered {} frame(s) of '{}' at {}x{} in {:.1f} ms", options.frames,
          SimulationKindName(options.simulation), extent.width, extent.height, elapsed.count());

  if (!options.capturePath.empty()) {
    WritePng(options.capturePath, extent, pixels);
    LogInfo("Wrote {}", options.capturePath.string());
  }
  if (!options.sequenceDirectory.empty()) {
    LogInfo("Wrote {} sequence frame(s) under {}", options.frames,
            options.sequenceDirectory.string());
  }

  if (context.validationErrorCount() > 0) {
    LogError("{} Vulkan validation error(s)", context.validationErrorCount());
    return 1;
  }
  return 0;
}

}  // namespace sph
