#pragma once

#include "glm/gtc/matrix_transform.hpp"

struct BlurSettings {
  glm::vec2 blurDir = glm::vec2(1.0, 1.0);
  float blurScale = 0.0001;
  float blurDepthFalloff = 3.5;
  float filterRadius = 12.0;
  float sigma = 11.0f;
  float kernelSize = 32.0f;
  float pad;
};