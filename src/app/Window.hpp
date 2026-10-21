#pragma once

#include "gpu/Vulkan.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct GLFWwindow;

namespace sph {

class Window {
 public:
  Window(const std::string& title, std::uint32_t width, std::uint32_t height);
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;
  Window(Window&&) = delete;
  Window& operator=(Window&&) = delete;

  [[nodiscard]] GLFWwindow* handle() const { return window_; }
  [[nodiscard]] bool shouldClose() const;
  [[nodiscard]] VkExtent2D framebufferExtent() const;

  // Returns whether the framebuffer was resized since the last call.
  bool consumeResize();
  std::vector<std::filesystem::path> consumeDrops();

 private:
  GLFWwindow* window_ = nullptr;
  bool resized_ = false;
  std::vector<std::filesystem::path> dropped_;
};

}  // namespace sph
