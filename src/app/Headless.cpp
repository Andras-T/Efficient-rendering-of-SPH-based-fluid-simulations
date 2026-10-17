#include "app/Headless.hpp"

#include "app/ImageWriter.hpp"
#include "core/Log.hpp"
#include "gpu/Buffer.hpp"
#include "gpu/Commands.hpp"
#include "gpu/Context.hpp"
#include "gpu/Image.hpp"
#include "render/Renderer.hpp"

#include <algorithm>
#include <chrono>

namespace sph {

namespace {

constexpr VkFormat CaptureFormat = VK_FORMAT_R8G8B8A8_SRGB;
constexpr std::uint32_t BytesPerPixel = 4;

}  // namespace

std::vector<std::uint8_t> RenderOffscreen(const gpu::Context& context, VkExtent2D extent,
                                          std::uint32_t frames) {
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
  const render::Renderer renderer(context, CaptureFormat);
  const render::RenderTarget renderTarget{target.handle(), target.view(), extent};

  for (std::uint32_t frame = 0; frame < std::max(frames, 1U); ++frame) {
    const bool last = frame + 1 >= frames;
    gpu::ImmediateSubmit(context, [&](VkCommandBuffer commandBuffer) {
      renderer.record(commandBuffer, renderTarget);
      if (!last) {
        return;
      }
      gpu::TransitionImage(commandBuffer,
                           {
                               .image = target.handle(),
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
      vkCmdCopyImageToBuffer(commandBuffer, target.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             readback.handle(), 1, &region);
      gpu::GlobalBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COPY_BIT,
                         VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_HOST_BIT,
                         VK_ACCESS_2_HOST_READ_BIT);
    });
  }

  readback.invalidate();
  const auto pixels = readback.mapped<const std::uint8_t>();
  return {pixels.begin(), pixels.end()};
}

int RunHeadless(const Options& options) {
  const gpu::Context context({.validation = options.validation});
  const VkExtent2D extent{options.width, options.height};

  const auto start = std::chrono::steady_clock::now();
  const auto pixels = RenderOffscreen(context, extent, options.frames);
  const std::chrono::duration<double, std::milli> elapsed =
      std::chrono::steady_clock::now() - start;
  LogInfo("Rendered {} frame(s) at {}x{} in {:.1f} ms", options.frames, extent.width, extent.height,
          elapsed.count());

  if (!options.capturePath.empty()) {
    WritePng(options.capturePath, extent, pixels);
    LogInfo("Wrote {}", options.capturePath.string());
  }

  if (context.validationErrorCount() > 0) {
    LogError("{} Vulkan validation error(s)", context.validationErrorCount());
    return 1;
  }
  return 0;
}

}  // namespace sph
