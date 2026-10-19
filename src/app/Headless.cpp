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
#include "sim/GpuParticles.hpp"

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

std::vector<std::uint8_t> RenderOffscreen(const gpu::Context& context,
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

  const sim::Scene ownedScene =
      request.scene == nullptr ? sim::BuildScene(request.preset) : sim::Scene{};
  const sim::Scene& scene = request.scene == nullptr ? ownedScene : *request.scene;

  render::Camera ownedCamera;
  ownedCamera.setAspect(extent);
  ownedCamera.frame(scene.params.domainMin, scene.params.domainMax);
  const render::Camera& camera = request.camera == nullptr ? ownedCamera : *request.camera;

  sim::GpuParticles particles(context);
  particles.upload(scene.particles);

  render::Renderer renderer(context, CaptureFormat);
  renderer.resize(extent);
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
      renderer.record(commandBuffer, renderTarget, camera, particles, scene.params);
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
                                                .preset = options.scene});
  const std::chrono::duration<double, std::milli> elapsed =
      std::chrono::steady_clock::now() - start;
  LogInfo("Rendered {} frame(s) of '{}' at {}x{} in {:.1f} ms", options.frames,
          sim::ScenePresetName(options.scene), extent.width, extent.height, elapsed.count());

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
