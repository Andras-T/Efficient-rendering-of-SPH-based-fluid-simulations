#pragma once

#include "glm/gtc/matrix_transform.hpp"

struct BlurSettings {
  glm::vec2 blurDir = glm::vec2(1.0, 1.0);
  float sigma = 11.0f;
  float kernelSize = 22.0f;
  float w_sum;
  glm::vec3 pad;
};