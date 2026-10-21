#include "app/Window.hpp"

#include "core/Log.hpp"

#include <GLFW/glfw3.h>

#include <span>
#include <stdexcept>
#include <utility>

namespace sph {

Window::Window(const std::string& title, std::uint32_t width, std::uint32_t height) {
  gpu::LoadVulkan();
  glfwInitVulkanLoader(vkGetInstanceProcAddr);
  glfwSetErrorCallback(
      [](int code, const char* message) { LogError("[glfw {}] {}", code, message); });
  if (glfwInit() == GLFW_FALSE) {
    throw std::runtime_error("glfwInit failed");
  }
  if (glfwVulkanSupported() == GLFW_FALSE) {
    glfwTerminate();
    throw std::runtime_error("GLFW found no Vulkan support");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(),
                             nullptr, nullptr);
  if (window_ == nullptr) {
    glfwTerminate();
    throw std::runtime_error("glfwCreateWindow failed");
  }
  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* window, int /*width*/, int /*height*/) {
    static_cast<Window*>(glfwGetWindowUserPointer(window))->resized_ = true;
  });
  glfwSetDropCallback(window_, [](GLFWwindow* window, int count, const char** paths) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (count <= 0 || paths == nullptr) {
      return;
    }
    for (const char* path : std::span(paths, static_cast<std::size_t>(count))) {
      if (path != nullptr) {
        self->dropped_.emplace_back(path);
      }
    }
  });
}

Window::~Window() {
  glfwDestroyWindow(window_);
  glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(window_) == GLFW_TRUE; }

VkExtent2D Window::framebufferExtent() const {
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window_, &width, &height);
  return {static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
}

bool Window::consumeResize() { return std::exchange(resized_, false); }

std::vector<std::filesystem::path> Window::consumeDrops() { return std::exchange(dropped_, {}); }

}  // namespace sph
