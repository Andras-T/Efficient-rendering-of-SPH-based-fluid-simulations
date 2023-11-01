#pragma once

#include "glm/gtc/matrix_transform.hpp"

struct BlurSettings {
  float blurScale = 0.0001;
  float blurDepthFalloff = 2.0;
  float filterRadius = 10.0;
  float pad1;
  glm::vec2 blurDir = glm::vec2(1.0, 1.0);
  glm::vec2 pad2;
};