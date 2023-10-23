#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "../SwapchainManager/SwapchainManager.h"
#include "../Utils/Utils.h"
#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include <vector>

class DescriptorManager {

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSetLayout quadDescriptorSetLayout;
  VkDescriptorSetLayout blurDescriptorSetLayout;

  VkDescriptorPool descriptorPool;
  VkDescriptorPool quadDescriptorPool;
  VkDescriptorPool blurDescriptorPool;

  VkSampler depthSampler;

  SwapchainManager *swapchainManager;

  std::vector<VkBuffer> *modelUniformBuffers;
  std::vector<VkBuffer> *uniformBuffers;

public:
  void init(VkDevice &device, SwapchainManager &swapchainManager) {
    createDescriptorSetLayout(device);
    this->swapchainManager = &swapchainManager;
  }

  void createDescriptorSetLayout(VkDevice &device);

  void createDescriptorPool(VkDevice &device);

  void createDescriptorSets(VkDevice &device,
                            std::vector<VkDescriptorSet> &descriptorSets,
                            std::vector<VkDescriptorSet> &quadDescriptorSets,
                            std::vector<VkDescriptorSet> &blurDescriptorSets,
                            std::vector<VkBuffer> &shaderStorageBuffers,
                            /*VkBuffer &sphereBuffers, VkBuffer& quadBuffers,*/
                            std::vector<VkBuffer> &uniformBuffers,
                            std::vector<VkBuffer> &attributesUniformBuffers,
                            std::vector<VkBuffer> &modelUniformBuffers);

  void recreateDescriptorSets(VkDevice &device,
                              std::vector<VkDescriptorSet> &quadDescriptorSets,
                              std::vector<VkDescriptorSet> &blurDescriptorSets);

  void cleanup(VkDevice &device);

#pragma region getterFunctions

  VkDescriptorSetLayout &getDescriptorSetLayout() {
    return descriptorSetLayout;
  }

  VkDescriptorSetLayout &getQuadDescriptorSetLayout() {
    return quadDescriptorSetLayout;
  }

  VkDescriptorPool &getDescriptorPool() { return descriptorPool; }

  VkDescriptorPool &getQuadDescriptorPool() { return quadDescriptorPool; }

#pragma endregion
};
