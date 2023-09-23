#ifndef DESCRIPTORMANAGER_H
#define DESCRIPTORMANAGER_H

#define GLFW_INCLUDE_VULKAN

#include "../Utils/Utils.h"
#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include <vector>

class DescriptorManager {

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorPool descriptorPool;

public:
  void init(VkDevice &device) { createDescriptorSetLayout(device); }

  void createDescriptorSetLayout(VkDevice &device);

  void createDescriptorPool(VkDevice &device);

  void createDescriptorSets(VkDevice &device,
                            std::vector<VkDescriptorSet> &descriptorSets,
                            std::vector<VkBuffer> &shaderStorageBuffers,
                            VkBuffer &sphereBuffers,
                            std::vector<VkBuffer> &uniformBuffers,
                            std::vector<VkBuffer> &attributesUniformBuffers,
                            std::vector<VkBuffer> &modelUniformBuffers);

  void cleanup(VkDevice &device);

#pragma region getterFunctions

  VkDescriptorSetLayout &getDescriptorSetLayout() {
    return descriptorSetLayout;
  }

  VkDescriptorPool &getDescriptorPool() { return descriptorPool; }

#pragma endregion
};

#endif // FLUIDSIMULATION_DESCRIPTORMANAGER_H
