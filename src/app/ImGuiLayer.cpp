#include "app/ImGuiLayer.hpp"

#include "gpu/Context.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <stdexcept>

namespace sph {

ImGuiLayer::ImGuiLayer(const gpu::Context& context, GLFWwindow* window, VkFormat colorFormat,
                       std::uint32_t imageCount)
    : colorFormat_(colorFormat) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = nullptr;
  ImGui::StyleColorsDark();

  if (!ImGui_ImplGlfw_InitForVulkan(window, true)) {
    ImGui::DestroyContext();
    throw std::runtime_error("ImGui GLFW backend failed to initialize");
  }

  ImGui_ImplVulkan_InitInfo info{};
  info.ApiVersion = VK_API_VERSION_1_3;
  info.Instance = context.instance();
  info.PhysicalDevice = context.physicalDevice();
  info.Device = context.device();
  info.QueueFamily = context.queueFamily();
  info.Queue = context.queue();
  info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE;
  info.MinImageCount = 2;
  info.ImageCount = std::max(imageCount, 8U);
  info.UseDynamicRendering = true;
  info.PipelineInfoMain.PipelineRenderingCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &colorFormat_,
  };
  if (!ImGui_ImplVulkan_Init(&info)) {
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    throw std::runtime_error("ImGui Vulkan backend failed to initialize");
  }
}

ImGuiLayer::~ImGuiLayer() {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiLayer::beginFrame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::endFrame() { ImGui::Render(); }

void ImGuiLayer::record(VkCommandBuffer commandBuffer) {
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

}  // namespace sph
