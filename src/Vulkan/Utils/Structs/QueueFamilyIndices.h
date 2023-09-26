#pragma once

#include <cstdint>
#include <optional>

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsAndComputeFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
  }
};
