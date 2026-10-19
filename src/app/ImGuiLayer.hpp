#pragma once

#include "gpu/Vulkan.hpp"

#include <cstdint>

struct GLFWwindow;

namespace sph {

namespace gpu {
class Context;
}

// Owns the ImGui context and its GLFW and Vulkan backends.
class ImGuiLayer {
 public:
  ImGuiLayer(const gpu::Context& context, GLFWwindow* window, VkFormat colorFormat,
             std::uint32_t imageCount);
  ~ImGuiLayer();

  ImGuiLayer(const ImGuiLayer&) = delete;
  ImGuiLayer& operator=(const ImGuiLayer&) = delete;
  ImGuiLayer(ImGuiLayer&&) = delete;
  ImGuiLayer& operator=(ImGuiLayer&&) = delete;

  void beginFrame();
  // Finalizes the frame; call once per frame before record().
  void endFrame();
  void record(VkCommandBuffer commandBuffer);

 private:
  VkFormat colorFormat_;
};

}  // namespace sph
