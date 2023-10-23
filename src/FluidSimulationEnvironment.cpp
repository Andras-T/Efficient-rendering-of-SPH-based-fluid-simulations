#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "FluidSimulationEnvironment.h"
#include "Vulkan/Utils/Utils.h"
#include "imgui_impl_glfw.h"
#include "imgui_internal.h"

void FluidSimulationEnvironment::init() {
  logger.LogInfo("Setting up fluid simulations environment");

  window.init();

  vulkanObject.init(*window.get_GLFW_Window(), enableValidationLayers);

  deviceManager.init(vulkanObject.getInstance(), vulkanObject.getSurface(),
                     enableValidationLayers);

  device = deviceManager.getDevice();
  physicalDevice = deviceManager.getPhysicalDevice();

  swapChainManager.init(window, vulkanObject.getSurface(), physicalDevice,
                        device, deviceManager.getFamilyIndices());

  vulkanObject.createRenderPass(device,
                                swapChainManager.getSwapChainImageFormat(),
                                swapChainManager.findDepthFormat());

  descriptorManager.init(device, swapChainManager);

  pipelineManager.init(device, descriptorManager.getDescriptorSetLayout(),
                       descriptorManager.getQuadDescriptorSetLayout(),
                       vulkanObject);

  commandPoolManager.init(device, deviceManager.getFamilyIndices());

  swapChainManager.createDepthResources();
  swapChainManager.createFramebuffers(vulkanObject);

  descriptorManager.createDescriptorPool(device);

  instance.InitBuffers(bufferManager, deviceManager,
                       commandPoolManager.getCommandPool(),
                       glm::vec3(0.0f, 0.0f, 0.0f), window.get_GLFW_Window());
  instance.InitDescriptorSets(descriptorManager, device, bufferManager);

  commandPoolManager.createCommandBuffers(device);

  render.init(deviceManager, swapChainManager, commandPoolManager,
              pipelineManager, vulkanObject, window, bufferManager, instance,
              descriptorManager);

  initImGui();
}

void FluidSimulationEnvironment::initImGui() {
  logger.LogInfo("Setting up ImGui");
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGui_ImplGlfw_InitForVulkan(window.get_GLFW_Window(), true);

  ImGui::GetIO().FontGlobalScale = 1.0f;

  ImGui::StyleColorsDark();

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = vulkanObject.getInstance();
  init_info.Device = device;
  init_info.PhysicalDevice = physicalDevice;
  init_info.QueueFamily = deviceManager.getQueueFamily();
  init_info.Queue = deviceManager.getGraphicsQueue();
  init_info.DescriptorPool = descriptorManager.getQuadDescriptorPool();
  init_info.MinImageCount = swapChainManager.getMinImageCount();
  init_info.ImageCount = swapChainManager.getSwapchainImageCount();
  init_info.Subpass = 0;
  VkPipelineCache m_pipeline_cache{nullptr};
  init_info.PipelineCache = m_pipeline_cache;
  init_info.Allocator = nullptr;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.CheckVkResultFn = check_vk_result;
  ImGui_ImplVulkan_Init(&init_info, vulkanObject.getQuadRenderPass());

  VkCommandBuffer commandBuffer =
      commandPoolManager.beginSingleTimeCommands(device);
  ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
  commandPoolManager.endSingleTimeCommands(
      commandBuffer, deviceManager.getGraphicsQueue(), device);

  vkDeviceWaitIdle(deviceManager.getDevice());
  ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void FluidSimulationEnvironment::run() {
  mainLoop();
  cleanUp();
}

void FluidSimulationEnvironment::mainLoop() {
  logger.LogInfo("Rendering loop called");

  while (!glfwWindowShouldClose(window.get_GLFW_Window())) {
    glfwPollEvents();
    render.drawFrame(lastFrameTime);
    checkInput();
    double currentTime = glfwGetTime();
    lastFrameTime = (currentTime - Window::lastTime) * 1000.0;
    Window::lastTime = currentTime;
  }
}

void FluidSimulationEnvironment::checkInput() {
  glfwPollEvents();
  if (ImGui::GetIO().KeyMods == ImGuiModFlags_Shift &&
          ImGui::IsKeyPressed(ImGuiKey_R, ImGuiKeyOwner_Any) ||
      render.getUniformData().resetRequest) {
    logger.LogInfo("Reset position");
    instance.resetInstance(bufferManager, deviceManager,
                           commandPoolManager.getCommandPool());
    render.getUniformData().attributes.stop = 1;
    render.getUniformData().resetRequest = false;
  }
}

void FluidSimulationEnvironment::cleanUp() {
  VkResult err = vkDeviceWaitIdle(device);
  check_vk_result(err);

  check_vk_result(err);

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  swapChainManager.cleanupSwapChain();

  pipelineManager.cleanup(device);

  vkDestroyRenderPass(device, vulkanObject.getSimulationRenderPass(), nullptr);
  vkDestroyRenderPass(device, vulkanObject.getQuadRenderPass(), nullptr);

  descriptorManager.cleanup(device);

  instance.cleanUp(device);

  bufferManager.cleanUp(device);

  render.cleanUp();

  commandPoolManager.cleanup(device);

  deviceManager.cleanup();

  vulkanObject.cleanup(enableValidationLayers);
}
