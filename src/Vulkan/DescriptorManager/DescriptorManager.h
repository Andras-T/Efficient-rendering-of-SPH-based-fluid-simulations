#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "../SwapchainManager/SwapchainManager.h"
#include "../Utils/Utils.h"
#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include <vector>
#include "../Uniform/Uniform.h"

class DescriptorManager {

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSetLayout quadDescriptorSetLayout;
  VkDescriptorSetLayout blurDescriptorSetLayout;

  VkDescriptorPool descriptorPool;
  VkDescriptorPool quadDescriptorPool;
  VkDescriptorPool blurDescriptorPool;

  VkSampler depthSampler;

  SwapchainManager *swapchainManager;

  std::unordered_map<std::string, Uniform>* uniforms;

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
                            std::unordered_map<std::string, Uniform> &uniforms);

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

  VkDescriptorSetLayout &getBlurDescriptorSetLayout() {
    return blurDescriptorSetLayout;
  }

  VkDescriptorPool &getDescriptorPool() { return descriptorPool; }

  VkDescriptorPool &getQuadDescriptorPool() { return quadDescriptorPool; }

  VkDescriptorPool &getBlurDescriptorPool() { return blurDescriptorPool; }

#pragma endregion
};
