#pragma once

#define GLFW_INCLUDE_VULKAN

#include "../Utils/Utils.h"
#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include <vector>

class DescriptorManager {

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSetLayout quadDescriptorSetLayout;

  VkDescriptorPool descriptorPool;
  VkDescriptorPool quadDescriptorPool;

public:
  void init(VkDevice &device) { createDescriptorSetLayout(device); }

  void createDescriptorSetLayout(VkDevice &device);

  void createDescriptorPool(VkDevice &device);

  void createDescriptorSets(VkDevice &device,
                            std::vector<VkDescriptorSet> &descriptorSets,
                            std::vector<VkDescriptorSet> &quadDescriptorSets,
                            std::vector<VkBuffer> &shaderStorageBuffers,
                            VkBuffer &sphereBuffers, VkBuffer &quadBuffers,
                            std::vector<VkBuffer> &uniformBuffers,
                            std::vector<VkBuffer> &attributesUniformBuffers,
                            std::vector<VkBuffer> &modelUniformBuffers);

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
