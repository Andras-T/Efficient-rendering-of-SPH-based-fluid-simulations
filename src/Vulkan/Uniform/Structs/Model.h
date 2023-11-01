#pragma once

#include "glm/vec4.hpp"

struct Model {
  glm::vec4 color = glm::vec4(0.313, 0.65, 1.0, 1.0);
  glm::vec4 wallColor = glm::vec4(0.6, 0.6, 0.6, 1.0);
  glm::vec2 windowSize = glm::vec2(1200, 800);
  int wall = 0;
  float farPlaneDistance;
};
