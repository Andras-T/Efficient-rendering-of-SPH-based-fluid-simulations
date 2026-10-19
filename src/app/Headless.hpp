#pragma once

#include "core/Options.hpp"
#include "gpu/Vulkan.hpp"
#include "render/Camera.hpp"
#include "render/DebugView.hpp"
#include "sim/Scene.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace sph {

namespace gpu {
class Context;
}

struct OffscreenRequest {
  VkExtent2D extent{};
  std::uint32_t frames = 1;
  std::filesystem::path sequenceDirectory;
  sim::ScenePreset preset = sim::ScenePreset::Empty;
  const sim::Scene* scene = nullptr;
  const render::Camera* camera = nullptr;
  bool simulate = false;
  render::DebugView view = render::DebugView::Final;
};

// Renders offscreen and returns the last frame as tightly packed sRGB RGBA8.
// When `sequenceDirectory` is set, each frame is written as DIR/0000.png.
std::vector<std::uint8_t> RenderOffscreen(const gpu::Context& context,
                                          const OffscreenRequest& request);

std::vector<std::uint8_t> RenderOffscreen(const gpu::Context& context, VkExtent2D extent,
                                          std::uint32_t frames,
                                          const std::filesystem::path& sequenceDirectory = {});

int RunHeadless(const Options& options);

}  // namespace sph
