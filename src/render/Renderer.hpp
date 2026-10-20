#pragma once

#include "gpu/Buffer.hpp"
#include "gpu/Image.hpp"
#include "render/BlockPass.hpp"
#include "render/Camera.hpp"
#include "render/CarPass.hpp"
#include "render/DebugView.hpp"
#include "render/FluidPass.hpp"
#include "render/FoamPass.hpp"
#include "render/Hdri.hpp"
#include "render/PaddlePass.hpp"
#include "render/ParticlePass.hpp"
#include "render/SkyPass.hpp"
#include "render/TankPass.hpp"
#include "render/TonemapPass.hpp"
#include "render/WindTracerPass.hpp"
#include "sim/SimParams.hpp"

#include <glm/vec3.hpp>

#include <cstdint>
#include <functional>

namespace sph::render {

struct RenderTarget {
  VkImage image = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkExtent2D extent{};
};

struct RenderSettings {
  DebugView view = DebugView::Final;
  WallDisplay walls = WallDisplay::Hidden;
  glm::vec3 absorption{2.45F, 0.42F, 0.12F};
  float ior = 1.333F;
  float exposure = 0.85F;
  glm::vec3 paddleCenter{0.0F};
  float paddleRadius = 0.0F;
  glm::vec3 blockMin{0.0F};
  glm::vec3 blockMax{0.0F};
  bool block = false;
};

using OverlayCallback = std::function<void(VkCommandBuffer)>;

class Renderer {
 public:
  static constexpr VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT;
  static constexpr VkFormat HdrFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

  Renderer(const gpu::Context& context, VkFormat colorFormat);

  void resize(VkExtent2D extent);

  [[nodiscard]] RenderSettings& settings() { return settings_; }
  [[nodiscard]] const RenderSettings& settings() const { return settings_; }

  // Draws the frame and leaves `target` in COLOR_ATTACHMENT_OPTIMAL.
  // `overlay` is recorded last, inside a separate rendering scope.
  void record(VkCommandBuffer commandBuffer, const RenderTarget& target, const Camera& camera,
              VkDeviceAddress particles, std::uint32_t particleCount, VkDeviceAddress foam,
              std::uint32_t foamCapacity, const sim::SimParams& params,
              const OverlayCallback& overlay = {});
  void recordWind(VkCommandBuffer commandBuffer, const RenderTarget& target, const Camera& camera,
                  VkDeviceAddress mesh, std::uint32_t meshVertices, VkDeviceAddress tracers,
                  std::uint32_t tracerCount, glm::vec3 domainMin, glm::vec3 domainMax,
                  const OverlayCallback& overlay = {});

 private:
  const gpu::Context* context_ = nullptr;
  Hdri hdri_;
  SkyPass skyPass_;
  ParticlePass particlePass_;
  FluidPass fluidPass_;
  FoamPass foamPass_;
  TankPass tankPass_;
  BlockPass blockPass_;
  PaddlePass paddlePass_;
  CarPass carPass_;
  WindTracerPass windTracerPass_;
  TonemapPass tonemapPass_;
  gpu::Unique<VkSampler> hdrSampler_;
  gpu::Image hdr_;
  gpu::Image depth_;
  gpu::Buffer uniforms_;
  RenderSettings settings_;
};

}  // namespace sph::render
