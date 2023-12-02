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
        VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

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
    std::array<VkDescriptorSetLayoutBinding, 8> blurLayoutBindings{};
    blurLayoutBindings[0].binding = 0;
    blurLayoutBindings[0].descriptorCount = 1;
    blurLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    blurLayoutBindings[0].pImmutableSamplers = nullptr;
    blurLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    blurLayoutBindings[1].binding = 1;
    blurLayoutBindings[1].descriptorCount = 1;
    blurLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    blurLayoutBindings[1].pImmutableSamplers = nullptr;
    blurLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    blurLayoutBindings[2].binding = 2;
    blurLayoutBindings[2].descriptorCount = 1;
    blurLayoutBindings[2].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    blurLayoutBindings[2].pImmutableSamplers = nullptr;
    blurLayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    blurLayoutBindings[3].binding = 3;
    blurLayoutBindings[3].descriptorCount = 1;
    blurLayoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    blurLayoutBindings[3].pImmutableSamplers = nullptr;
    blurLayoutBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    blurLayoutBindings[4].binding = 4;
    blurLayoutBindings[4].descriptorCount = 1;
    blurLayoutBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    blurLayoutBindings[4].pImmutableSamplers = nullptr;
    blurLayoutBindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    blurLayoutBindings[5].binding = 5;
    blurLayoutBindings[5].descriptorCount = 1;
    blurLayoutBindings[5].descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    blurLayoutBindings[5].pImmutableSamplers = nullptr;
    blurLayoutBindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    blurLayoutBindings[6].binding = 6;
    blurLayoutBindings[6].descriptorCount = 1;
    blurLayoutBindings[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    blurLayoutBindings[6].pImmutableSamplers = nullptr;
    blurLayoutBindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    blurLayoutBindings[7].binding = 7;
    blurLayoutBindings[7].descriptorCount = 1;
    blurLayoutBindings[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    blurLayoutBindings[7].pImmutableSamplers = nullptr;
    blurLayoutBindings[7].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

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
    std::array<VkDescriptorSetLayoutBinding, 8> quadLayoutBindings{};
    quadLayoutBindings[0].binding = 0;
    quadLayoutBindings[0].descriptorCount = 1;
    quadLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    quadLayoutBindings[0].pImmutableSamplers = nullptr;
    quadLayoutBindings[0].stageFlags =
      VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

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

    quadLayoutBindings[4].binding = 4;
    quadLayoutBindings[4].descriptorCount = 1;
    quadLayoutBindings[4].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    quadLayoutBindings[4].pImmutableSamplers = nullptr;
    quadLayoutBindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    quadLayoutBindings[5].binding = 5;
    quadLayoutBindings[5].descriptorCount = 1;
    quadLayoutBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    quadLayoutBindings[5].pImmutableSamplers = nullptr;
    quadLayoutBindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    quadLayoutBindings[6].binding = 6;
    quadLayoutBindings[6].descriptorCount = 1;
    quadLayoutBindings[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    quadLayoutBindings[6].pImmutableSamplers = nullptr;
    quadLayoutBindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    quadLayoutBindings[7].binding = 7;
    quadLayoutBindings[7].descriptorCount = 1;
    quadLayoutBindings[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    quadLayoutBindings[7].pImmutableSamplers = nullptr;
    quadLayoutBindings[7].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

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
    std::array<VkDescriptorPoolSize, 8> blurPoolSizes{};
    blurPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    blurPoolSizes[0].descriptorCount = static_cast<uint32_t>(1);

    blurPoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    blurPoolSizes[1].descriptorCount = static_cast<uint32_t>(1);

    blurPoolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    blurPoolSizes[2].descriptorCount = static_cast<uint32_t>(1);

    blurPoolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    blurPoolSizes[3].descriptorCount = static_cast<uint32_t>(1);

    blurPoolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    blurPoolSizes[4].descriptorCount = static_cast<uint32_t>(1);

    blurPoolSizes[5].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    blurPoolSizes[5].descriptorCount = static_cast<uint32_t>(1);

    blurPoolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    blurPoolSizes[6].descriptorCount = static_cast<uint32_t>(1);

    blurPoolSizes[7].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    blurPoolSizes[7].descriptorCount = static_cast<uint32_t>(1);

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
    std::array<VkDescriptorPoolSize, 8> quadPoolSizes{};
    quadPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    quadPoolSizes[0].descriptorCount = static_cast<uint32_t>(1);

    quadPoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    quadPoolSizes[1].descriptorCount = static_cast<uint32_t>(1);

    quadPoolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    quadPoolSizes[2].descriptorCount = static_cast<uint32_t>(1);

    quadPoolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    quadPoolSizes[3].descriptorCount = static_cast<uint32_t>(1);

    quadPoolSizes[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    quadPoolSizes[4].descriptorCount = static_cast<uint32_t>(1);
    
    quadPoolSizes[5].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    quadPoolSizes[5].descriptorCount = static_cast<uint32_t>(1);

    quadPoolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    quadPoolSizes[6].descriptorCount = static_cast<uint32_t>(1);

    quadPoolSizes[7].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    quadPoolSizes[7].descriptorCount = static_cast<uint32_t>(1);

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

void DescriptorManager::createDescriptorSets(VkDevice &device, 
                                             std::vector<VkDescriptorSet> &descriptorSets,
                                             std::vector<VkDescriptorSet> &quadDescriptorSets,
                                             std::vector<VkDescriptorSet> &blurDescriptorSets,
                                             std::vector<VkBuffer> &shaderStorageBuffers,
                                             std::unordered_map<std::string, Uniform> &uniforms) {
  // Simulation Descriptor sets
  {
    std::vector<VkDescriptorSetLayout> layouts(Utils::MAX_FRAMES_IN_FLIGHT,
                                               descriptorSetLayout);
    this->uniforms = &uniforms;
    this->shaderStorageBuffers = &shaderStorageBuffers;
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
      MVPInfo.buffer = uniforms["MVP"].getBuffer(i);
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
      attributesUniformBufferInfo.buffer = uniforms["Attributes"].getBuffer(i);
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
      modelUniformBufferInfo.buffer = uniforms["Model"].getBuffer(i);
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
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) !=
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
      std::array<VkWriteDescriptorSet, 8> descriptorWrites{};

      VkDescriptorBufferInfo modelUniformBufferInfo{};
      modelUniformBufferInfo.buffer = uniforms["Model"].getBuffer(i);
      modelUniformBufferInfo.offset = 0;
      modelUniformBufferInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet = blurDescriptorSets[i];
      descriptorWrites[0].dstBinding = 0;
      descriptorWrites[0].dstArrayElement = 0;
      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pBufferInfo = &modelUniformBufferInfo;

      VkDescriptorBufferInfo blurSettingsBufferInfo{};
      blurSettingsBufferInfo.buffer = uniforms["BlurSettings"].getBuffer(i);
      blurSettingsBufferInfo.offset = 0;
      blurSettingsBufferInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[1].dstSet = blurDescriptorSets[i];
      descriptorWrites[1].dstBinding = 1;
      descriptorWrites[1].dstArrayElement = 0;
      descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[1].descriptorCount = 1;
      descriptorWrites[1].pBufferInfo = &blurSettingsBufferInfo;

      VkDescriptorImageInfo depthImageInfo = {};
      depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      depthImageInfo.imageView = swapchainManager->getDepthImageView();
      depthImageInfo.sampler = sampler;

      descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[2].dstSet = blurDescriptorSets[i];
      descriptorWrites[2].dstBinding = 2;
      descriptorWrites[2].dstArrayElement = 0;
      descriptorWrites[2].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[2].descriptorCount = 1;
      descriptorWrites[2].pImageInfo = &depthImageInfo;

      VkDescriptorImageInfo blurImageInfo1{};
      blurImageInfo1.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      blurImageInfo1.imageView = swapchainManager->getBlurImageView1(i);
      blurImageInfo1.sampler = VK_NULL_HANDLE;

      descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[3].dstSet = blurDescriptorSets[i];
      descriptorWrites[3].dstBinding = 3;
      descriptorWrites[3].dstArrayElement = 0;
      descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorWrites[3].descriptorCount = 1;
      descriptorWrites[3].pImageInfo = &blurImageInfo1;

      VkDescriptorImageInfo blurImageInfo2{};
      blurImageInfo2.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      blurImageInfo2.imageView = swapchainManager->getBlurImageView2(i);
      blurImageInfo2.sampler = VK_NULL_HANDLE;

      descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[4].dstSet = blurDescriptorSets[i];
      descriptorWrites[4].dstBinding = 4;
      descriptorWrites[4].dstArrayElement = 0;
      descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorWrites[4].descriptorCount = 1;
      descriptorWrites[4].pImageInfo = &blurImageInfo2;

      VkDescriptorImageInfo volumeImageInfo = {};
      volumeImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      volumeImageInfo.imageView = swapchainManager->getVolumeImageView();
      volumeImageInfo.sampler = sampler;

      descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[5].dstSet = blurDescriptorSets[i];
      descriptorWrites[5].dstBinding = 5;
      descriptorWrites[5].dstArrayElement = 0;
      descriptorWrites[5].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[5].descriptorCount = 1;
      descriptorWrites[5].pImageInfo = &volumeImageInfo;

      VkDescriptorImageInfo blurVolumeImageInfo1{};
      blurVolumeImageInfo1.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      blurVolumeImageInfo1.imageView = swapchainManager->getBlurVolumeImageView1(i);
      blurVolumeImageInfo1.sampler = VK_NULL_HANDLE;

      descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[6].dstSet = blurDescriptorSets[i];
      descriptorWrites[6].dstBinding = 6;
      descriptorWrites[6].dstArrayElement = 0;
      descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorWrites[6].descriptorCount = 1;
      descriptorWrites[6].pImageInfo = &blurVolumeImageInfo1;

      VkDescriptorImageInfo blurVolumeImageInfo2{};
      blurVolumeImageInfo2.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      blurVolumeImageInfo2.imageView = swapchainManager->getBlurVolumeImageView2(i);
      blurVolumeImageInfo2.sampler = VK_NULL_HANDLE;

      descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[7].dstSet = blurDescriptorSets[i];
      descriptorWrites[7].dstBinding = 7;
      descriptorWrites[7].dstArrayElement = 0;
      descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorWrites[7].descriptorCount = 1;
      descriptorWrites[7].pImageInfo = &blurVolumeImageInfo2;

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
      std::array<VkWriteDescriptorSet, 8> descriptorWrites{};

      VkDescriptorBufferInfo MVPInfo{};
      MVPInfo.buffer = uniforms["MVP"].getBuffer(i);
      MVPInfo.offset = 0;
      MVPInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet = quadDescriptorSets[i];
      descriptorWrites[0].dstBinding = 0;
      descriptorWrites[0].dstArrayElement = 0;
      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pBufferInfo = &MVPInfo;

      VkDescriptorBufferInfo modelUniformBufferInfo{};
      modelUniformBufferInfo.buffer = uniforms["Model"].getBuffer(i);
      modelUniformBufferInfo.offset = 0;
      modelUniformBufferInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[1].dstSet = quadDescriptorSets[i];
      descriptorWrites[1].dstBinding = 1;
      descriptorWrites[1].dstArrayElement = 0;
      descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[1].descriptorCount = 1;
      descriptorWrites[1].pBufferInfo = &modelUniformBufferInfo;

      VkDescriptorBufferInfo viewModeUniformBufferInfo{};
      viewModeUniformBufferInfo.buffer = uniforms["ViewMode"].getBuffer(i);
      viewModeUniformBufferInfo.offset = 0;
      viewModeUniformBufferInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[2].dstSet = quadDescriptorSets[i];
      descriptorWrites[2].dstBinding = 2;
      descriptorWrites[2].dstArrayElement = 0;
      descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[2].descriptorCount = 1;
      descriptorWrites[2].pBufferInfo = &viewModeUniformBufferInfo;

      VkDescriptorImageInfo texCubeImageInfo = {};
      texCubeImageInfo.imageLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      texCubeImageInfo.imageView = swapchainManager->getTexCubeImageView();
      texCubeImageInfo.sampler = sampler;

      descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[3].dstSet = quadDescriptorSets[i];
      descriptorWrites[3].dstBinding = 3;
      descriptorWrites[3].dstArrayElement = 0;
      descriptorWrites[3].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[3].descriptorCount = 1;
      descriptorWrites[3].pImageInfo = &texCubeImageInfo;

      VkDescriptorImageInfo depthImageInfo = {};
      depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      depthImageInfo.imageView = swapchainManager->getDepthImageView();
      depthImageInfo.sampler = sampler;

      descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[4].dstSet = quadDescriptorSets[i];
      descriptorWrites[4].dstBinding = 4;
      descriptorWrites[4].dstArrayElement = 0;
      descriptorWrites[4].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[4].descriptorCount = 1;
      descriptorWrites[4].pImageInfo = &depthImageInfo;

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      imageInfo.imageView = swapchainManager->getBlurImageView2(i);
      imageInfo.sampler = VK_NULL_HANDLE;

      descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[5].dstSet = quadDescriptorSets[i];
      descriptorWrites[5].dstBinding = 5;
      descriptorWrites[5].dstArrayElement = 0;
      descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorWrites[5].descriptorCount = 1;
      descriptorWrites[5].pImageInfo = &imageInfo;

      VkDescriptorImageInfo bluredVolumeImageInfo{};
      bluredVolumeImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      bluredVolumeImageInfo.imageView = swapchainManager->getBlurVolumeImageView2(i);
      bluredVolumeImageInfo.sampler = VK_NULL_HANDLE;

      descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[6].dstSet = quadDescriptorSets[i];
      descriptorWrites[6].dstBinding = 6;
      descriptorWrites[6].dstArrayElement = 0;
      descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorWrites[6].descriptorCount = 1;
      descriptorWrites[6].pImageInfo = &bluredVolumeImageInfo;

      VkDescriptorImageInfo volumeImageInfo = {};
      volumeImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      volumeImageInfo.imageView = swapchainManager->getVolumeImageView();
      volumeImageInfo.sampler = sampler;

      descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[7].dstSet = quadDescriptorSets[i];
      descriptorWrites[7].dstBinding = 7;
      descriptorWrites[7].dstArrayElement = 0;
      descriptorWrites[7].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[7].descriptorCount = 1;
      descriptorWrites[7].pImageInfo = &volumeImageInfo;

      vkUpdateDescriptorSets(device,
                             static_cast<uint32_t>(descriptorWrites.size()),
                             descriptorWrites.data(), 0, nullptr);
    }
  }
}

void DescriptorManager::recreateDescriptorSets(VkDevice& device,
                                               std::vector<VkDescriptorSet>& descriptorSets,
                                               std::vector<VkDescriptorSet>& quadDescriptorSets,
                                               std::vector<VkDescriptorSet>& blurDescriptorSets) {
  // Simulation Descriptor sets
  {
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
          std::array<VkWriteDescriptorSet, 5> descriptorWrites{};

          VkDescriptorBufferInfo MVPInfo{};
          MVPInfo.buffer = (*uniforms)["MVP"].getBuffer(i);
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
            (*shaderStorageBuffers)[(i - 1) % Utils::MAX_FRAMES_IN_FLIGHT];
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
          storageBufferInfoCurrentFrame.buffer = (*shaderStorageBuffers)[i];
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
          attributesUniformBufferInfo.buffer = (*uniforms)["Attributes"].getBuffer(i);
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
          modelUniformBufferInfo.buffer = (*uniforms)["Model"].getBuffer(i);
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
      std::array<VkWriteDescriptorSet, 8> descriptorWrites{};

      VkDescriptorBufferInfo modelUniformBufferInfo{};
      modelUniformBufferInfo.buffer = (*uniforms)["Model"].getBuffer(i);
      modelUniformBufferInfo.offset = 0;
      modelUniformBufferInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet = blurDescriptorSets[i];
      descriptorWrites[0].dstBinding = 0;
      descriptorWrites[0].dstArrayElement = 0;
      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pBufferInfo = &modelUniformBufferInfo;

      VkDescriptorBufferInfo blurSettingsBufferInfo{};
      blurSettingsBufferInfo.buffer = (*uniforms)["BlurSettings"].getBuffer(i);
      blurSettingsBufferInfo.offset = 0;
      blurSettingsBufferInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[1].dstSet = blurDescriptorSets[i];
      descriptorWrites[1].dstBinding = 1;
      descriptorWrites[1].dstArrayElement = 0;
      descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[1].descriptorCount = 1;
      descriptorWrites[1].pBufferInfo = &blurSettingsBufferInfo;

      VkDescriptorImageInfo depthImageInfo = {};
      depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      depthImageInfo.imageView = swapchainManager->getDepthImageView();
      depthImageInfo.sampler = sampler;

      descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[2].dstSet = blurDescriptorSets[i];
      descriptorWrites[2].dstBinding = 2;
      descriptorWrites[2].dstArrayElement = 0;
      descriptorWrites[2].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[2].descriptorCount = 1;
      descriptorWrites[2].pImageInfo = &depthImageInfo;

      VkDescriptorImageInfo blurImageInfo1{};
      blurImageInfo1.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      blurImageInfo1.imageView = swapchainManager->getBlurImageView1(i);
      blurImageInfo1.sampler = VK_NULL_HANDLE;

      descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[3].dstSet = blurDescriptorSets[i];
      descriptorWrites[3].dstBinding = 3;
      descriptorWrites[3].dstArrayElement = 0;
      descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorWrites[3].descriptorCount = 1;
      descriptorWrites[3].pImageInfo = &blurImageInfo1;

      VkDescriptorImageInfo blurImageInfo2{};
      blurImageInfo2.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      blurImageInfo2.imageView = swapchainManager->getBlurImageView2(i);
      blurImageInfo2.sampler = VK_NULL_HANDLE;

      descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[4].dstSet = blurDescriptorSets[i];
      descriptorWrites[4].dstBinding = 4;
      descriptorWrites[4].dstArrayElement = 0;
      descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorWrites[4].descriptorCount = 1;
      descriptorWrites[4].pImageInfo = &blurImageInfo2;

      VkDescriptorImageInfo volumeImageInfo = {};
      volumeImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      volumeImageInfo.imageView = swapchainManager->getVolumeImageView();
      volumeImageInfo.sampler = sampler;

      descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[5].dstSet = blurDescriptorSets[i];
      descriptorWrites[5].dstBinding = 5;
      descriptorWrites[5].dstArrayElement = 0;
      descriptorWrites[5].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[5].descriptorCount = 1;
      descriptorWrites[5].pImageInfo = &volumeImageInfo;

      VkDescriptorImageInfo blurVolumeImageInfo1{};
      blurVolumeImageInfo1.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      blurVolumeImageInfo1.imageView = swapchainManager->getBlurVolumeImageView1(i);
      blurVolumeImageInfo1.sampler = VK_NULL_HANDLE;

      descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[6].dstSet = blurDescriptorSets[i];
      descriptorWrites[6].dstBinding = 6;
      descriptorWrites[6].dstArrayElement = 0;
      descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorWrites[6].descriptorCount = 1;
      descriptorWrites[6].pImageInfo = &blurVolumeImageInfo1;

      VkDescriptorImageInfo blurVolumeImageInfo2{};
      blurVolumeImageInfo2.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      blurVolumeImageInfo2.imageView = swapchainManager->getBlurVolumeImageView2(i);
      blurVolumeImageInfo2.sampler = VK_NULL_HANDLE;

      descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[7].dstSet = blurDescriptorSets[i];
      descriptorWrites[7].dstBinding = 7;
      descriptorWrites[7].dstArrayElement = 0;
      descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorWrites[7].descriptorCount = 1;
      descriptorWrites[7].pImageInfo = &blurVolumeImageInfo2;

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
      std::array<VkWriteDescriptorSet, 8> descriptorWrites{};

      VkDescriptorBufferInfo MVPInfo{};
      MVPInfo.buffer = (*uniforms)["MVP"].getBuffer(i);
      MVPInfo.offset = 0;
      MVPInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet = quadDescriptorSets[i];
      descriptorWrites[0].dstBinding = 0;
      descriptorWrites[0].dstArrayElement = 0;
      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pBufferInfo = &MVPInfo;

      VkDescriptorBufferInfo modelUniformBufferInfo{};
      modelUniformBufferInfo.buffer = (*uniforms)["Model"].getBuffer(i);
      modelUniformBufferInfo.offset = 0;
      modelUniformBufferInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[1].dstSet = quadDescriptorSets[i];
      descriptorWrites[1].dstBinding = 1;
      descriptorWrites[1].dstArrayElement = 0;
      descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[1].descriptorCount = 1;
      descriptorWrites[1].pBufferInfo = &modelUniformBufferInfo;

      VkDescriptorBufferInfo viewModeUniformBufferInfo{};
      viewModeUniformBufferInfo.buffer = (*uniforms)["ViewMode"].getBuffer(i);
      viewModeUniformBufferInfo.offset = 0;
      viewModeUniformBufferInfo.range = VK_WHOLE_SIZE;

      descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[2].dstSet = quadDescriptorSets[i];
      descriptorWrites[2].dstBinding = 2;
      descriptorWrites[2].dstArrayElement = 0;
      descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[2].descriptorCount = 1;
      descriptorWrites[2].pBufferInfo = &viewModeUniformBufferInfo;

      VkDescriptorImageInfo texCubeImageInfo = {};
      texCubeImageInfo.imageLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      texCubeImageInfo.imageView = swapchainManager->getTexCubeImageView();
      texCubeImageInfo.sampler = sampler;

      descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[3].dstSet = quadDescriptorSets[i];
      descriptorWrites[3].dstBinding = 3;
      descriptorWrites[3].dstArrayElement = 0;
      descriptorWrites[3].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[3].descriptorCount = 1;
      descriptorWrites[3].pImageInfo = &texCubeImageInfo;

      VkDescriptorImageInfo depthImageInfo = {};
      depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      depthImageInfo.imageView = swapchainManager->getDepthImageView();
      depthImageInfo.sampler = sampler;

      descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[4].dstSet = quadDescriptorSets[i];
      descriptorWrites[4].dstBinding = 4;
      descriptorWrites[4].dstArrayElement = 0;
      descriptorWrites[4].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[4].descriptorCount = 1;
      descriptorWrites[4].pImageInfo = &depthImageInfo;

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      imageInfo.imageView = swapchainManager->getBlurImageView2(i);
      imageInfo.sampler = VK_NULL_HANDLE;

      descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[5].dstSet = quadDescriptorSets[i];
      descriptorWrites[5].dstBinding = 5;
      descriptorWrites[5].dstArrayElement = 0;
      descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorWrites[5].descriptorCount = 1;
      descriptorWrites[5].pImageInfo = &imageInfo;

      VkDescriptorImageInfo bluredVolumeImageInfo{};
      bluredVolumeImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      bluredVolumeImageInfo.imageView = swapchainManager->getBlurVolumeImageView2(i);
      bluredVolumeImageInfo.sampler = VK_NULL_HANDLE;

      descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[6].dstSet = quadDescriptorSets[i];
      descriptorWrites[6].dstBinding = 6;
      descriptorWrites[6].dstArrayElement = 0;
      descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      descriptorWrites[6].descriptorCount = 1;
      descriptorWrites[6].pImageInfo = &bluredVolumeImageInfo;

      VkDescriptorImageInfo volumeImageInfo = {};
      volumeImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      volumeImageInfo.imageView = swapchainManager->getVolumeImageView();
      volumeImageInfo.sampler = sampler;

      descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[7].dstSet = quadDescriptorSets[i];
      descriptorWrites[7].dstBinding = 7;
      descriptorWrites[7].dstArrayElement = 0;
      descriptorWrites[7].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[7].descriptorCount = 1;
      descriptorWrites[7].pImageInfo = &volumeImageInfo;

      vkUpdateDescriptorSets(device,
        static_cast<uint32_t>(descriptorWrites.size()),
        descriptorWrites.data(), 0, nullptr);
    }
  }
}

void DescriptorManager::cleanup(VkDevice &device) {
  vkDestroySampler(device, sampler, nullptr);
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  vkDestroyDescriptorPool(device, quadDescriptorPool, nullptr);
  vkDestroyDescriptorPool(device, blurDescriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, quadDescriptorSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, blurDescriptorSetLayout, nullptr);
}