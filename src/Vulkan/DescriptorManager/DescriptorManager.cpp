#include "DescriptorManager.h"

#include <array>
#include <chrono>
#include <stdexcept>

void DescriptorManager::createDescriptorSetLayout(VkDevice &device) {
  std::array<VkDescriptorSetLayoutBinding, 6> layoutBindings{};
  layoutBindings[5].binding = 5;
  layoutBindings[5].descriptorCount = 1;
  layoutBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  layoutBindings[5].pImmutableSamplers = nullptr;
  layoutBindings[5].stageFlags =
      VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;

  layoutBindings[1].binding = 1;
  layoutBindings[1].descriptorCount = 1;
  layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  layoutBindings[1].pImmutableSamplers = nullptr;
  layoutBindings[1].stageFlags =
      VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;

  layoutBindings[2].binding = 2;
  layoutBindings[2].descriptorCount = 1;
  layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  layoutBindings[2].pImmutableSamplers = nullptr;
  layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  layoutBindings[3].binding = 3;
  layoutBindings[3].descriptorCount = 1;
  layoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  layoutBindings[3].pImmutableSamplers = nullptr;
  layoutBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  layoutBindings[4].binding = 4;
  layoutBindings[4].descriptorCount = 1;
  layoutBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  layoutBindings[4].pImmutableSamplers = nullptr;
  layoutBindings[4].stageFlags =
      VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

  layoutBindings[0].binding = 0;
  layoutBindings[0].descriptorCount = 1;
  layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  layoutBindings[0].pImmutableSamplers = nullptr;
  layoutBindings[0].stageFlags =
      VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
  layoutInfo.pBindings = layoutBindings.data();

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void DescriptorManager::createDescriptorPool(VkDevice &device) {
  std::array<VkDescriptorPoolSize, 6> poolSizes{};
  poolSizes[5].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[5].descriptorCount =
      static_cast<uint32_t>(Utils::MAX_FRAMES_IN_FLIGHT);

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[1].descriptorCount =
      static_cast<uint32_t>(Utils::MAX_FRAMES_IN_FLIGHT) * 2;

  poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[2].descriptorCount =
      static_cast<uint32_t>(Utils::MAX_FRAMES_IN_FLIGHT);

  poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[3].descriptorCount =
      static_cast<uint32_t>(Utils::MAX_FRAMES_IN_FLIGHT);

  poolSizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[4].descriptorCount =
      static_cast<uint32_t>(Utils::MAX_FRAMES_IN_FLIGHT);

  poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(1);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = 1024;

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void DescriptorManager::createDescriptorSets(
    VkDevice &device, std::vector<VkDescriptorSet> &descriptorSets,
    std::vector<VkBuffer> &shaderStorageBuffers, VkBuffer &sphereBuffers,
    std::vector<VkBuffer> &uniformBuffers,
    std::vector<VkBuffer> &attributesUniformBuffers,
    std::vector<VkBuffer> &modelUniformBuffers) {
  std::vector<VkDescriptorSetLayout> layouts(Utils::MAX_FRAMES_IN_FLIGHT,
                                             descriptorSetLayout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount =
      static_cast<uint32_t>(Utils::MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(Utils::MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.buffer = uniformBuffers[i];
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 6> descriptorWrites{};
    descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[5].dstSet = descriptorSets[i];
    descriptorWrites[5].dstBinding = 5;
    descriptorWrites[5].dstArrayElement = 0;
    descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[5].descriptorCount = 1;
    descriptorWrites[5].pBufferInfo = &uniformBufferInfo;

    VkDescriptorBufferInfo storageBufferInfoLastFrame{};
    storageBufferInfoLastFrame.buffer =
        shaderStorageBuffers[(i - 1) % Utils::MAX_FRAMES_IN_FLIGHT];
    storageBufferInfoLastFrame.offset = 0;
    storageBufferInfoLastFrame.range = VK_WHOLE_SIZE;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &storageBufferInfoLastFrame;

    VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
    storageBufferInfoCurrentFrame.buffer = shaderStorageBuffers[i];
    storageBufferInfoCurrentFrame.offset = 0;
    storageBufferInfoCurrentFrame.range = VK_WHOLE_SIZE;

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = descriptorSets[i];
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &storageBufferInfoCurrentFrame;

    VkDescriptorBufferInfo attributesUniformBufferInfo{};
    attributesUniformBufferInfo.buffer = attributesUniformBuffers[i];
    attributesUniformBufferInfo.offset = 0;
    attributesUniformBufferInfo.range = VK_WHOLE_SIZE;

    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = descriptorSets[i];
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pBufferInfo = &attributesUniformBufferInfo;

    VkDescriptorBufferInfo modelUniformBufferInfo{};
    modelUniformBufferInfo.buffer = modelUniformBuffers[i];
    modelUniformBufferInfo.offset = 0;
    modelUniformBufferInfo.range = VK_WHOLE_SIZE;

    descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[4].dstSet = descriptorSets[i];
    descriptorWrites[4].dstBinding = 4;
    descriptorWrites[4].dstArrayElement = 0;
    descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[4].descriptorCount = 1;
    descriptorWrites[4].pBufferInfo = &modelUniformBufferInfo;

    VkDescriptorBufferInfo sphereBufferInfo{};
    sphereBufferInfo.buffer = sphereBuffers;
    sphereBufferInfo.offset = 0;
    sphereBufferInfo.range = VK_WHOLE_SIZE;

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &sphereBufferInfo;

    vkUpdateDescriptorSets(device, 6, descriptorWrites.data(), 0, nullptr);
  }
}

void DescriptorManager::cleanup(VkDevice &device) {
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}