#pragma once

#include "gpu/Handles.hpp"
#include "gpu/Image.hpp"

#include <filesystem>

namespace sph {
namespace gpu {
class Context;
}
namespace render {

// Equirectangular HDRI uploaded as a sampled RGBA32F image.
class Hdri {
 public:
  Hdri(const gpu::Context& context, const std::filesystem::path& path);

  [[nodiscard]] VkImageView view() const { return image_.view(); }
  [[nodiscard]] VkSampler sampler() const { return sampler_.get(); }

 private:
  gpu::Image image_;
  gpu::Unique<VkSampler> sampler_;
};

}  // namespace render
}  // namespace sph
