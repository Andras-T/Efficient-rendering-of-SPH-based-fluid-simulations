#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <string>

struct BlurStage {
  int stageIndex = 0;

  static std::string getOwner() { return "Blur"; }

  static VkPushConstantRange getPushConstantRange() {
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(int);
    return pushConstantRange;
  }
};