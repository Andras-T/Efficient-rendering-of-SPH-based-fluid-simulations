#pragma once

struct PushConstant {
  int stageIndex = 1;

  static std::string getOwner() { return "Compute"; }

  static VkPushConstantRange getPushConstantRange() {
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(int);
    return pushConstantRange;
  }
};