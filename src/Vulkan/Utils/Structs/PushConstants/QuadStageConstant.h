#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <string>

struct QuadStage {
  int stageIndex = 0;

  static std::string getOwner() { return "Quad"; }

  static VkPushConstantRange getPushConstantRange() {
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags =
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(int);
    return pushConstantRange;
  }
};