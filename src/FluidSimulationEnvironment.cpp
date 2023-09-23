#include "FluidSimulationEnvironment.h"
#include "Vulkan/Utils/Utils.h"
#include "imgui_impl_glfw.h"
#include "imgui_internal.h"

void FluidSimulationEnvironment::init() {
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

  descriptorManager.init(device);

  pipelineManager.init(vertPath, fragPath, compPath, device,
                       descriptorManager.getDescriptorSetLayout(),
                       vulkanObject.getRenderPass());

  commandPoolManager.init(device, deviceManager.getFamilyIndices());

  swapChainManager.createDepthResources();
  swapChainManager.createFramebuffers(vulkanObject.getRenderPass());

  descriptorManager.createDescriptorPool(device);

  for (auto &instance : instances) {
    instance.InitBuffers(bufferManager, deviceManager,
                         commandPoolManager.getCommandPool(),
                         glm::vec3(x, 0.0f, 0.0f));
    instance.InitDescriptorSets(descriptorManager, device, bufferManager);
    x += offsetX;
  }

  commandPoolManager.createCommandBuffers(device);

  render.init(deviceManager, swapChainManager, commandPoolManager,
              pipelineManager, vulkanObject, window);

  for (auto &instance : instances)
    render.addInstance(instance);

  initImGui();
}

void FluidSimulationEnvironment::initImGui() {
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
  init_info.DescriptorPool = descriptorManager.getDescriptorPool();
  init_info.MinImageCount = swapChainManager.getMinImageCount();
  init_info.ImageCount = swapChainManager.getSwapchainImageCount();
  init_info.Subpass = 0;
  VkPipelineCache m_pipeline_cache{nullptr};
  init_info.PipelineCache = m_pipeline_cache;
  init_info.Allocator = nullptr;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.CheckVkResultFn = check_vk_result;
  ImGui_ImplVulkan_Init(&init_info, vulkanObject.getRenderPass());

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
  if (ImGui::GetIO().KeyMods == ImGuiModFlags_Ctrl &&
      ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
      instances.size() < MAX_SIZE) {
    addNewInstance();
  }
  if (ImGui::GetIO().KeyMods == ImGuiModFlags_Ctrl &&
      ImGui::IsMouseClicked(ImGuiMouseButton_Right) && instances.size() > 1) {
    VkResult err = vkDeviceWaitIdle(device);
    render.removeLastInstance();
    instances[instances.size() - 1].cleanUp(device);
    instances.pop_back();
    x -= offsetX;
  }
}

void FluidSimulationEnvironment::addNewInstance() {
  FluidInstance instance1;
  instances.push_back(instance1);
  instances[instances.size() - 1].InitBuffers(
      bufferManager, deviceManager, commandPoolManager.getCommandPool(),
      glm::vec3(x, 0.0f, 0.0f));
  instances[instances.size() - 1].InitDescriptorSets(descriptorManager, device,
                                                     bufferManager);
  render.addInstance(instances[instances.size() - 1]);
  x += offsetX;
}

void FluidSimulationEnvironment::cleanUp() {
  VkResult err = vkDeviceWaitIdle(device);
  check_vk_result(err);

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  swapChainManager.cleanupSwapChain();

  pipelineManager.cleanup(device);

  vkDestroyRenderPass(device, vulkanObject.getRenderPass(), nullptr);

  descriptorManager.cleanup(device);

  for (auto &instance : instances)
    instance.cleanUp(device);

  bufferManager.cleanUp(device);

  render.cleanUp();

  commandPoolManager.cleanup(device);

  deviceManager.cleanup();

  vulkanObject.cleanup(enableValidationLayers);
}