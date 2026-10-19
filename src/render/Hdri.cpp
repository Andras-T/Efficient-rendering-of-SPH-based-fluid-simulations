#include "render/Hdri.hpp"

#include "gpu/Buffer.hpp"
#include "gpu/Commands.hpp"
#include "gpu/Context.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cstdint>
#include <format>
#include <stdexcept>

namespace sph::render {

Hdri::Hdri(const gpu::Context& context, const std::filesystem::path& path) {
  int width = 0;
  int height = 0;
  int channels = 0;
  float* pixels = stbi_loadf(path.string().c_str(), &width, &height, &channels, 4);
  if (pixels == nullptr || width <= 0 || height <= 0) {
    throw std::runtime_error(std::format("could not load HDRI {}", path.string()));
  }

  const VkExtent2D extent{static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
  const VkDeviceSize byteCount = VkDeviceSize{extent.width} * extent.height * 4 * sizeof(float);
  const gpu::Buffer staging(
      context, {
                   .size = byteCount,
                   .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                      VMA_ALLOCATION_CREATE_MAPPED_BIT,
               });
  const auto mapped = staging.mapped<float>();
  std::copy_n(pixels, mapped.size(), mapped.begin());
  staging.flush();
  stbi_image_free(pixels);

  image_ =
      gpu::Image(context, {
                              .extent = extent,
                              .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                              .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                          });
  sampler_ = gpu::CreateSampler(context.device(), VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

  gpu::ImmediateSubmit(context, [&](VkCommandBuffer commandBuffer) {
    gpu::TransitionImage(commandBuffer, {
                                            .image = image_.handle(),
                                            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                            .dstStage = VK_PIPELINE_STAGE_2_COPY_BIT,
                                            .dstAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                        });
    const VkBufferImageCopy region{
        .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1},
        .imageExtent = {extent.width, extent.height, 1},
    };
    vkCmdCopyBufferToImage(commandBuffer, staging.handle(), image_.handle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    gpu::TransitionImage(commandBuffer, {
                                            .image = image_.handle(),
                                            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                            .srcStage = VK_PIPELINE_STAGE_2_COPY_BIT,
                                            .srcAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                            .dstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                            .dstAccess = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                                        });
  });
}

}  // namespace sph::render
