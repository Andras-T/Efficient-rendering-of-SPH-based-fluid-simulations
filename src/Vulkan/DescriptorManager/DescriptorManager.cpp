#include "DescriptorManager.h"

#include <array>
#include <chrono>
#include <stdexcept>

void DescriptorManager::createDescriptorSetLayout(VkDevice &device) {
  // Simulation layout bindings
  {
    std::array<VkDescriptorSetLayoutBinding, 5> layoutBindings{};
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[0].pImmutableSamplers = nullptr;
    layoutBindings[0].stageFlags =
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

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutInfo.pBindings = layoutBindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                    &descriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error(
          "failed to create simulation descriptor set layout!");
    }
  }

  // Blur layout bindings
  {
    std::array<VkDescriptorSetLayoutBinding, 3> blurLayoutBindings{};
    blurLayoutBindings[0].binding = 0;
    blurLayoutBindings[0].descriptorCount = 1;
    blurLayoutBindings[0].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    blurLayoutBindings[0].pImmutableSamplers = nullptr;
    blurLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    blurLayoutBindings[1].binding = 1;
    blurLayoutBindings[1].descriptorCount = 1;
    blurLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    blurLayoutBindings[1].pImmutableSamplers = nullptr;
    blurLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    blurLayoutBindings[2].binding = 2;
    blurLayoutBindings[2].descriptorCount = 1;
    blurLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    blurLayoutBindings[2].pImmutableSamplers = nullptr;
    blurLayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo blurLayoutInfo{};
    blurLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    blurLayoutInfo.bindingCount =
        static_cast<uint32_t>(blurLayoutBindings.size());
    blurLayoutInfo.pBindings = blurLayoutBindings.data();

    if (vkCreateDescriptorSetLayout(device, &blurLayoutInfo, nullptr,
                                    &blurDescriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create blur descriptor set layout!");
    }
  }

  // Quad layout bindings
  {
    std::array<VkDescriptorSetLayoutBinding, 4> quadLayoutBindings{};
    quadLayoutBindings[0].binding = 0;
    quadLayoutBindings[0].descriptorCount = 1;
    quadLayoutBindings[0].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    quadLayoutBindings[0].pImmutableSamplers = nullptr;
    quadLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    quadLayoutBindings[1].binding = 1;
    quadLayoutBindings[1].descriptorCount = 1;
    quadLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    quadLayoutBindings[1].pImmutableSamplers = nullptr;
    quadLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    quadLayoutBindings[2].binding = 2;
    quadLayoutBindings[2].descriptorCount = 1;
    quadLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    quadLayoutBindings[2].pImmutableSamplers = nullptr;
    quadLayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    quadLayoutBindings[3].binding = 3;
    quadLayoutBindings[3].descriptorCount = 1;
    quadLayoutBindings[3].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    quadLayoutBindings[3].pImmutableSamplers = nullptr;
    quadLayoutBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo quadLayoutInfo{};
    quadLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    quadLayoutInfo.bindingCount =
        static_cast<uint32_t>(quadLayoutBindings.size());
    quadLayoutInfo.pBindings = quadLayoutBindings.data();

    if (vkCreateDescriptorSetLayout(device, &quadLayoutInfo, nullptr,
                                    &quadDescriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create quad descriptor set layout!");
    }
  }
}

void DescriptorManager::createDescriptorPool(VkDevice &device) {
  // Simulation descriptor pool
  {
    std::array<VkDescriptorPoolSize, 5> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount =
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

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1024;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create simulation descriptor pool!");
    }
  }

  // Blur descriptor pool
  {
    std::array<VkDescriptorPoolSize, 3> blurPoolSizes{};
    blurPoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    blurPoolSizes[0].descriptorCount = static_cast<uint32_t>(1);

    blurPoolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    blurPoolSizes[1].descriptorCount = static_cast<uint32_t>(1);

    blurPoolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    blurPoolSizes[2].descriptorCount = static_cast<uint32_t>(1);

    VkDescriptorPoolCreateInfo blurPoolInfo{};
    blurPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    blurPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    blurPoolInfo.poolSizeCount = static_cast<uint32_t>(blurPoolSizes.size());
    blurPoolInfo.pPoolSizes = blurPoolSizes.data();
    blurPoolInfo.maxSets = 1024;

    if (vkCreateDescriptorPool(device, &blurPoolInfo, nullptr,
                               &blurDescriptorPool) != VK_SUCCESS) {
      throw std::runtime_error("failed to create blur descriptor pool!");
    }
  }

  // Quad descriptor pool
  {
    std::array<VkDescriptorPoolSize, 4> quadPoolSizes{};
    quadPoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    quadPoolSizes[0].descriptorCount = static_cast<uint32_t>(1);

    quadPoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    quadPoolSizes[1].descriptorCount = static_cast<uint32_t>(1);

    quadPoolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    quadPoolSizes[2].descriptorCount = static_cast<uint32_t>(1);

    quadPoolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    quadPoolSizes[3].descriptorCount = static_cast<uint32_t>(1);

    VkDescriptorPoolCreateInfo quadPoolInfo{};
    quadPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    quadPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    quadPoolInfo.poolSizeCount = static_cast<uint32_t>(quadPoolSizes.size());
    quadPoolInfo.pPoolSizes = quadPoolSizes.data();
    quadPoolInfo.maxSets = 1024;

    if (vkCreateDescriptorPool(device, &quadPoolInfo, nullptr,
                               &quadDescriptorPool) != VK_SUCCESS) {
      throw std::runtime_error("failed to create quad descriptor pool!");
    }
  }
}

void DescriptorManager::createDescriptorSets(
    VkDevice &device, std::vector<VkDescriptorSet> &descriptorSets,
    std::vector<VkDescriptorSet> &quadDescriptorSets,
    std::vector<VkDescriptorSet> &blurDescriptorSets,
    std::vector<VkBuffer> &shaderStorageBuffers,
    std::vector<VkBuffer> &uniformBuffers,
    std::vector<VkBuffer> &attributesUniformBuffers,
    std::vector<VkBuffer> &modelUniformBuffers) {
  // Simulation Descriptor sets
  {
    std::vector<VkDescriptorSetLayout> layouts(Utils::MAX_FRAMES_IN_FLIGHT,
                                               descriptorSetLayout);
    this->modelUniformBuffers = &modelUniformBuffers;
    this->uniformBuffers = &uniformBuffers;
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
      std::array<VkWriteDescriptorSet, 5> descriptorWrites{};

      VkDescriptorBufferInfo MVPInfo{};
      MVPInfo.buffer = uniformBuffers[i];
      MVPInfo.offset = 0;
      MVPInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet = descriptorSets[i];
      descriptorWrites[0].dstBinding = 0;
      descriptorWrites[0].dstArrayElement = 0;
      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pBufferInfo = &MVPInfo;

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

      vkUpdateDescriptorSets(device,
                             static_cast<uint32_t>(descriptorWrites.size()),
                             descriptorWrites.data(), 0, nullptr);
    }
  }

  // 2D sampler info
  {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp =
        VK_COMPARE_OP_ALWAYS; // VK_COMPARE_OP_ALWAYS // VK_COMPARE_OP_LESS
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &depthSampler) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create depth sampler!");
    }
  }

  // Blur Descriptor sets
  {
    std::vector<VkDescriptorSetLayout> blurLayouts(Utils::MAX_FRAMES_IN_FLIGHT,
                                                   blurDescriptorSetLayout);

    VkDescriptorSetAllocateInfo blurAllocInfo{};
    blurAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    blurAllocInfo.descriptorPool = blurDescriptorPool;
    blurAllocInfo.descriptorSetCount =
        static_cast<uint32_t>(Utils::MAX_FRAMES_IN_FLIGHT);
    blurAllocInfo.pSetLayouts = blurLayouts.data();

    blurDescriptorSets.resize(Utils::MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &blurAllocInfo,
                                 blurDescriptorSets.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate blur descriptor sets!");
    }

    for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {
      std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

      VkDescriptorImageInfo depthImageInfo = {};
      depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      depthImageInfo.imageView = swapchainManager->getDepthImageView();
      depthImageInfo.sampler = depthSampler;

      descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet = blurDescriptorSets[i];
      descriptorWrites[0].dstBinding = 0;
      descriptorWrites[0].dstArrayElement = 0;
      descriptorWrites[0].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pImageInfo = &depthImageInfo;

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      imageInfo.imageView = swapchainManager->getBlurImageView();
      imageInfo.sampler = VK_NULL_HANDLE;

      descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[1].dstSet = blurDescriptorSets[i];
      descriptorWrites[1].dstBinding = 1;
      descriptorWrites[1].dstArrayElement = 0;
      descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorWrites[1].descriptorCount = 1;
      descriptorWrites[1].pImageInfo = &imageInfo;

      VkDescriptorBufferInfo modelUniformBufferInfo{};
      modelUniformBufferInfo.buffer = modelUniformBuffers[i];
      modelUniformBufferInfo.offset = 0;
      modelUniformBufferInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[2].dstSet = blurDescriptorSets[i];
      descriptorWrites[2].dstBinding = 2;
      descriptorWrites[2].dstArrayElement = 0;
      descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[2].descriptorCount = 1;
      descriptorWrites[2].pBufferInfo = &modelUniformBufferInfo;

      vkUpdateDescriptorSets(device,
                             static_cast<uint32_t>(descriptorWrites.size()),
                             descriptorWrites.data(), 0, nullptr);
    }
  }

  // Quad Descriptor sets
  {
    std::vector<VkDescriptorSetLayout> quadLayouts(Utils::MAX_FRAMES_IN_FLIGHT,
                                                   quadDescriptorSetLayout);

    VkDescriptorSetAllocateInfo quadAllocInfo{};
    quadAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    quadAllocInfo.descriptorPool = quadDescriptorPool;
    quadAllocInfo.descriptorSetCount =
        static_cast<uint32_t>(Utils::MAX_FRAMES_IN_FLIGHT);
    quadAllocInfo.pSetLayouts = quadLayouts.data();

    quadDescriptorSets.resize(Utils::MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &quadAllocInfo,
                                 quadDescriptorSets.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {
      std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

      VkDescriptorImageInfo depthImageInfo = {};
      depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      depthImageInfo.imageView = swapchainManager->getDepthImageView();
      depthImageInfo.sampler = depthSampler;

      descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet = quadDescriptorSets[i];
      descriptorWrites[0].dstBinding = 0;
      descriptorWrites[0].dstArrayElement = 0;
      descriptorWrites[0].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pImageInfo = &depthImageInfo;

      VkDescriptorBufferInfo MVPInfo{};
      MVPInfo.buffer = uniformBuffers[i];
      MVPInfo.offset = 0;
      MVPInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[1].dstSet = quadDescriptorSets[i];
      descriptorWrites[1].dstBinding = 1;
      descriptorWrites[1].dstArrayElement = 0;
      descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[1].descriptorCount = 1;
      descriptorWrites[1].pBufferInfo = &MVPInfo;

      VkDescriptorBufferInfo modelUniformBufferInfo{};
      modelUniformBufferInfo.buffer = modelUniformBuffers[i];
      modelUniformBufferInfo.offset = 0;
      modelUniformBufferInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[2].dstSet = quadDescriptorSets[i];
      descriptorWrites[2].dstBinding = 2;
      descriptorWrites[2].dstArrayElement = 0;
      descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[2].descriptorCount = 1;
      descriptorWrites[2].pBufferInfo = &modelUniformBufferInfo;

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      imageInfo.imageView = swapchainManager->getBlurImageView();
      imageInfo.sampler = depthSampler;

      descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[3].dstSet = quadDescriptorSets[i];
      descriptorWrites[3].dstBinding = 3;
      descriptorWrites[3].dstArrayElement = 0;
      descriptorWrites[3].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[3].descriptorCount = 1;
      descriptorWrites[3].pImageInfo = &imageInfo;

      vkUpdateDescriptorSets(device,
                             static_cast<uint32_t>(descriptorWrites.size()),
                             descriptorWrites.data(), 0, nullptr);
    }
  }
}

void DescriptorManager::recreateDescriptorSets(
    VkDevice &device, std::vector<VkDescriptorSet> &quadDescriptorSets,
    std::vector<VkDescriptorSet> &blurDescriptorSets) {
  // Recreate depth sampler
  {
    vkDestroySampler(device, depthSampler, nullptr);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS; // VK_COMPARE_OP_LESS
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &depthSampler) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create depth sampler!");
    }
  }

  // Recreate Blur Descriptor sets
  {
    std::vector<VkDescriptorSetLayout> blurLayouts(Utils::MAX_FRAMES_IN_FLIGHT,
                                                   blurDescriptorSetLayout);

    VkDescriptorSetAllocateInfo blurAllocInfo{};
    blurAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    blurAllocInfo.descriptorPool = blurDescriptorPool;
    blurAllocInfo.descriptorSetCount =
        static_cast<uint32_t>(Utils::MAX_FRAMES_IN_FLIGHT);
    blurAllocInfo.pSetLayouts = blurLayouts.data();

    blurDescriptorSets.resize(Utils::MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &blurAllocInfo,
                                 blurDescriptorSets.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate blur descriptor sets!");
    }

    for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {
      std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

      VkDescriptorImageInfo depthImageInfo = {};
      depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      depthImageInfo.imageView = swapchainManager->getDepthImageView();
      depthImageInfo.sampler = depthSampler;

      descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet = blurDescriptorSets[i];
      descriptorWrites[0].dstBinding = 0;
      descriptorWrites[0].dstArrayElement = 0;
      descriptorWrites[0].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pImageInfo = &depthImageInfo;

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      imageInfo.imageView = swapchainManager->getBlurImageView();
      imageInfo.sampler = VK_NULL_HANDLE;

      descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[1].dstSet = blurDescriptorSets[i];
      descriptorWrites[1].dstBinding = 1;
      descriptorWrites[1].dstArrayElement = 0;
      descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorWrites[1].descriptorCount = 1;
      descriptorWrites[1].pImageInfo = &imageInfo;

      VkDescriptorBufferInfo modelUniformBufferInfo{};
      modelUniformBufferInfo.buffer = (*modelUniformBuffers)[i];
      modelUniformBufferInfo.offset = 0;
      modelUniformBufferInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[2].dstSet = blurDescriptorSets[i];
      descriptorWrites[2].dstBinding = 2;
      descriptorWrites[2].dstArrayElement = 0;
      descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[2].descriptorCount = 1;
      descriptorWrites[2].pBufferInfo = &modelUniformBufferInfo;

      vkUpdateDescriptorSets(device,
                             static_cast<uint32_t>(descriptorWrites.size()),
                             descriptorWrites.data(), 0, nullptr);
    }
  }

  // Recreate Quad Descriptor sets
  {
    std::vector<VkDescriptorSetLayout> quadLayouts(Utils::MAX_FRAMES_IN_FLIGHT,
                                                   quadDescriptorSetLayout);

    VkDescriptorSetAllocateInfo quadAllocInfo{};
    quadAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    quadAllocInfo.descriptorPool = quadDescriptorPool;
    quadAllocInfo.descriptorSetCount =
        static_cast<uint32_t>(Utils::MAX_FRAMES_IN_FLIGHT);
    quadAllocInfo.pSetLayouts = quadLayouts.data();

    quadDescriptorSets.resize(Utils::MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &quadAllocInfo,
                                 quadDescriptorSets.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {
      std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

      VkDescriptorImageInfo depthImageInfo = {};
      depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      depthImageInfo.imageView = swapchainManager->getDepthImageView();
      depthImageInfo.sampler = depthSampler;

      descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet = quadDescriptorSets[i];
      descriptorWrites[0].dstBinding = 0;
      descriptorWrites[0].dstArrayElement = 0;
      descriptorWrites[0].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pImageInfo = &depthImageInfo;

      VkDescriptorBufferInfo MVPInfo{};
      MVPInfo.buffer = (*uniformBuffers)[i];
      MVPInfo.offset = 0;
      MVPInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[1].dstSet = quadDescriptorSets[i];
      descriptorWrites[1].dstBinding = 1;
      descriptorWrites[1].dstArrayElement = 0;
      descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[1].descriptorCount = 1;
      descriptorWrites[1].pBufferInfo = &MVPInfo;

      VkDescriptorBufferInfo modelUniformBufferInfo{};
      modelUniformBufferInfo.buffer = (*modelUniformBuffers)[i];
      modelUniformBufferInfo.offset = 0;
      modelUniformBufferInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[2].dstSet = quadDescriptorSets[i];
      descriptorWrites[2].dstBinding = 2;
      descriptorWrites[2].dstArrayElement = 0;
      descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[2].descriptorCount = 1;
      descriptorWrites[2].pBufferInfo = &modelUniformBufferInfo;

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      imageInfo.imageView = swapchainManager->getBlurImageView();
      imageInfo.sampler = depthSampler;

      descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[3].dstSet = quadDescriptorSets[i];
      descriptorWrites[3].dstBinding = 3;
      descriptorWrites[3].dstArrayElement = 0;
      descriptorWrites[3].descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[3].descriptorCount = 1;
      descriptorWrites[3].pImageInfo = &imageInfo;

      vkUpdateDescriptorSets(device,
                             static_cast<uint32_t>(descriptorWrites.size()),
                             descriptorWrites.data(), 0, nullptr);
    }
  }
}

void DescriptorManager::cleanup(VkDevice &device) {
  vkDestroySampler(device, depthSampler, nullptr);
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  vkDestroyDescriptorPool(device, quadDescriptorPool, nullptr);
  vkDestroyDescriptorPool(device, blurDescriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, quadDescriptorSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, blurDescriptorSetLayout, nullptr);
}