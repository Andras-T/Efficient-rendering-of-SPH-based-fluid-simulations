#ifndef MODEL_H
#define MODEL_H

#include "glm/vec4.hpp"

struct Model {
  glm::vec4 color = glm::vec4(0.15, 0.25, 1.0, 0.8);
  glm::vec4 wallColor = glm::vec4(0.6, 0.6, 0.6, 0.1);
  float size = 22.0f;
  float wallSize = 22.0f;
  int wall = 0;
  float pad;
};

#endif // MODEL_H
